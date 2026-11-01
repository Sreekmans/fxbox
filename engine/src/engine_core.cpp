#include "engine_core.hpp"

#include <cmath>
#include <cstring>

#if defined(__aarch64__)
#include <cstdint>
#elif defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace fxbox {

namespace {
constexpr float kHalfPi = 1.57079632679f;
constexpr float kQuietLevel = 1e-5f;     // -100 dBFS
constexpr float kSineRmsCal = 1.11072f;  // pi / (2*sqrt(2)): average -> RMS for sines
constexpr float kMonoSilence = 1e-4f;    // -80 dBFS: right input counts as unused
constexpr float kMonoSignal = 1e-3f;     // -60 dBFS: left input carries signal

inline float coef(float tauSeconds, int sr) {
    return 1.f - std::exp(-1.f / (tauSeconds * float(sr)));
}

// Flush denormals to zero on the calling (audio) thread.
inline void enableFlushToZero() {
#if defined(__aarch64__)
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    asm volatile("msr fpcr, %0" ::"r"(fpcr | (1ULL << 24)));
#elif defined(__x86_64__) || defined(__i386__)
    _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
}
} // namespace

bool EngineCore::init(int sampleRate, int maxBlock) {
    enableFlushToZero();   // the warm-up below runs on this thread
    sr_ = sampleRate;
    maxBlock_ = maxBlock;
    kFast_ = coef(0.010f, sr_);
    kVu_ = coef(0.045f, sr_);   // two cascaded poles: ~99 % after 300 ms (VU rise time)

    for (int c = 0; c < 2; ++c) {
        cur_[c].assign(maxBlock, 0.f);
        fxIn_[c].assign(maxBlock, 0.f);
        fxOut_[c].assign(maxBlock, 0.f);
        dly_[c].assign(maxBlock, 0.f);
        osIn_[c].assign(2 * maxBlock, 0.f);
        osOut_[c].assign(2 * maxBlock, 0.f);
    }
    std::vector<float> silence(maxBlock, 0.f);
    for (int i = 0; i < kNumFx; ++i) {
        Slot& s = fx_[i];
        s.dsp = makeFaustDsp(i);
        if (!s.dsp) return false;
        s.os = kFxSpecs[i].oversample == 2 ? 2 : 1;
        s.dsp->init(sr_ * s.os);
        for (int c = 0; c < 2; ++c) {
            s.up[c].prepare(maxBlock);
            s.down[c].prepare(maxBlock);
            s.dryDelay[c].prepare(maxBlock);
        }
        ZoneCollector zc;
        s.dsp->buildUserInterface(&zc);
        for (int p = 0; p < kNumParams; ++p) {
            const ParamSpec& ps = kFxSpecs[i].params[p];
            if (ps.kind == ParamKind::Zone) {
                s.zones[p] = zc.find(ps.zone);
                if (!s.zones[p]) return false;
            }
        }
        s.lfoReset = zc.find("lforeset");
        {
            int k = 0;
            for (auto& m : zc.meters)
                if (m.first == "gr" && k < 2) s.grZones[k++] = m.second;
        }
        s.hasMix = fxHasMix(i);
        if (!s.hasMix) s.mix = s.mixTarget = 1.f;   // e.g. the filter: always fully wet
        // Warm-up: let the parameter smoothers settle on their defaults, so an
        // effect that is switched on later does not glide in from zero.
        float* ins[2] = {silence.data(), silence.data()};
        float* outs[2] = {osOut_[0].data(), osOut_[1].data()};
        std::vector<float> silence2(2 * maxBlock, 0.f);
        float* ins2[2] = {silence2.data(), silence2.data()};
        for (int done = 0; done < sr_; done += maxBlock)
            s.dsp->compute(maxBlock * s.os, s.os == 2 ? ins2 : ins, outs);
    }
    retarget();
    return true;
}

void EngineCore::retarget() {
    for (auto& s : fx_) {
        const bool eff = s.on && !bypass_;
        s.gInTarget = eff ? 1.f : 0.f;
        s.gOutTarget = (eff || trails_) ? 1.f : 0.f;
        s.dryTarget = eff ? std::cos(s.mixTarget * kHalfPi) : 1.f;
        s.dlyMixTarget = (eff && s.os == 2) ? 1.f : 0.f;
    }
}

void EngineCore::apply(const Command& c) {
    switch (c.type) {
        case Command::SetZone:
            if (c.fx < kNumFx && c.idx < kNumParams && fx_[c.fx].zones[c.idx]) {
                const ParamSpec& ps = kFxSpecs[c.fx].params[c.idx];
                float v = c.value;
                if (!(v >= ps.minV)) v = ps.minV;   // also catches NaN
                if (v > ps.maxV) v = ps.maxV;
                *fx_[c.fx].zones[c.idx] = v;
            }
            break;
        case Command::SetMix:
            if (c.fx < kNumFx && fx_[c.fx].hasMix) {
                float v = c.value;
                if (!(v >= 0.f)) v = 0.f;
                if (v > 1.f) v = 1.f;
                fx_[c.fx].mixTarget = v;
                retarget();
            }
            break;
        case Command::SetEnabled:
            if (c.fx < kNumFx) {
                fx_[c.fx].on = c.value > 0.5f;
                retarget();
            }
            break;
        case Command::Bypass:
            bypass_ = c.value > 0.5f;
            retarget();
            break;
        case Command::InputMode: {
            const int m = int(c.value + 0.5f);
            inputMode_ = (m == INPUT_MONO || m == INPUT_AUTO) ? m : INPUT_STEREO;
            break;
        }
        case Command::InputGainDb: {
            float db = c.value;
            if (!(db >= -24.f)) db = -24.f;
            if (db > 24.f) db = 24.f;
            inGainTarget_ = std::pow(10.f, db / 20.f);
            break;
        }
        case Command::Trails:
            trails_ = c.value > 0.5f;
            retarget();
            break;
        case Command::LfoReset:
            for (auto& s : fx_)
                if (s.lfoReset) *s.lfoReset = 1.f;
            break;
    }
}

void EngineCore::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    static thread_local bool ftz = false;
    if (!ftz) { enableFlushToZero(); ftz = true; }

    // Larger blocks than prepared are processed in chunks.
    if (n > maxBlock_) {
        for (int off = 0; off < n; off += maxBlock_) {
            int m = (n - off) < maxBlock_ ? (n - off) : maxBlock_;
            process(inL + off, inR + off, outL + off, outR + off, m);
        }
        return;
    }

    Command cmd;
    while (queue_.pop(cmd)) apply(cmd);

    // ---------------------------------------------------------------- input
    float inPk[2] = {0.f, 0.f};
    uint32_t inClips = 0;
    for (int i = 0; i < n; ++i) {
        const float al = std::fabs(inL[i]), ar = std::fabs(inR[i]);
        if (al > inPk[0]) inPk[0] = al;
        if (ar > inPk[1]) inPk[1] = ar;
        if (al >= 0.9999f || ar >= 0.9999f) ++inClips;
    }
    // Auto mode: a silent right input with signal on the left means a mono source.
    if (inputMode_ == INPUT_AUTO) {
        if (inPk[1] < kMonoSilence) {
            if (inPk[0] > kMonoSignal || rightSilent_ > 0) rightSilent_ += n;
        } else {
            rightSilent_ = 0;
        }
    }
    const bool mono = inputMode_ == INPUT_MONO || (inputMode_ == INPUT_AUTO && rightSilent_ > sr_ / 4);
    const float monoTarget = mono ? 1.f : 0.f;

    float g = inGain_, mm = monoMix_;
    for (int i = 0; i < n; ++i) {
        g += (inGainTarget_ - g) * kFast_;
        mm += (monoTarget - mm) * kFast_;
        const float l = inL[i];
        const float r = inR[i] * (1.f - mm) + inL[i] * mm;
        cur_[0][i] = l * g;
        cur_[1][i] = r * g;
    }
    // One-pole smoothers stall just short of their target in float precision; snap.
    if (std::fabs(g - inGainTarget_) < 1e-5f) g = inGainTarget_;
    if (std::fabs(mm - monoTarget) < 1e-4f) mm = monoTarget;
    inGain_ = g;
    monoMix_ = mm;

    // ---------------------------------------------------------------- chain
    uint32_t faults = 0;
    for (int ci = 0; ci < kNumFx; ++ci) {
        Slot& s = fx_[kChainOrder[ci]];
        const bool feeding = s.gInTarget > 0.f || s.gIn > 1e-6f;
        if (feeding) s.quietSamples = 0;
        const bool audible = s.gOut > 1e-6f && s.gOutTarget > 0.f;
        // Oversampled effects: keep the delayed dry copy current, even while skipped,
        // so switching on can crossfade to the aligned dry path without a jump.
        if (s.os == 2)
            for (int c = 0; c < 2; ++c) s.dryDelay[c].process(cur_[c].data(), dly_[c].data(), n);
        const bool run = feeding || (audible && s.quietSamples < 2 * sr_) ||
                         std::fabs(s.dry - s.dryTarget) > 1e-4f ||
                         std::fabs(s.dlyMix - s.dlyMixTarget) > 1e-4f;
        if (!run) {
            // Off and silent: the signal passes unchanged, the DSP is skipped.
            s.gIn = 0.f;
            s.dry = s.dryTarget;   // == 1 when off
            if (s.gOutTarget == 0.f) s.gOut = 0.f;
            s.mix = s.mixTarget;
            s.dlyMix = s.dlyMixTarget;   // == 0 when off
            if (s.lfoReset) *s.lfoReset = 0.f;
            if (s.grZones[0]) *s.grZones[0] = 0.f;
            if (s.grZones[1]) *s.grZones[1] = 0.f;
            continue;
        }

        float gi = s.gIn;
        for (int i = 0; i < n; ++i) {
            gi += (s.gInTarget - gi) * kFast_;
            fxIn_[0][i] = cur_[0][i] * gi;
            fxIn_[1][i] = cur_[1][i] * gi;
        }
        s.gIn = gi;

        if (s.os == 2) {
            for (int c = 0; c < 2; ++c) s.up[c].up(fxIn_[c].data(), osIn_[c].data(), n);
            float* ins[2] = {osIn_[0].data(), osIn_[1].data()};
            float* outs[2] = {osOut_[0].data(), osOut_[1].data()};
            s.dsp->compute(2 * n, ins, outs);
            // Check at the high rate (a NaN could otherwise be filtered into the history).
            const float o0 = osOut_[0][2 * n - 1], o1 = osOut_[1][2 * n - 1];
            if (!std::isfinite(o0) || !std::isfinite(o1)) {
                std::memset(osOut_[0].data(), 0, sizeof(float) * 2 * n);
                std::memset(osOut_[1].data(), 0, sizeof(float) * 2 * n);
            }
            for (int c = 0; c < 2; ++c) s.down[c].down(osOut_[c].data(), fxOut_[c].data(), n);
        } else {
            float* ins[2] = {fxIn_[0].data(), fxIn_[1].data()};
            float* outs[2] = {fxOut_[0].data(), fxOut_[1].data()};
            s.dsp->compute(n, ins, outs);
        }
        if (s.lfoReset) *s.lfoReset = 0.f;   // one-block pulse

        // Recover from numerical failure without taking the engine down.
        const float t0 = fxOut_[0][n - 1], t1 = fxOut_[1][n - 1];
        if (!std::isfinite(t0) || !std::isfinite(t1) || std::fabs(t0) > 1e4f || std::fabs(t1) > 1e4f) {
            s.dsp->instanceClear();
            for (int c = 0; c < 2; ++c) { s.up[c].reset(); s.down[c].reset(); }
            std::memset(fxOut_[0].data(), 0, sizeof(float) * n);
            std::memset(fxOut_[1].data(), 0, sizeof(float) * n);
            ++faults;
        }

        float go = s.gOut, mx = s.mix, d = s.dry, dm = s.dlyMix, blockMax = 0.f;
        const bool os2 = s.os == 2;
        for (int i = 0; i < n; ++i) {
            go += (s.gOutTarget - go) * kFast_;
            mx += (s.mixTarget - mx) * kFast_;
            d += (s.dryTarget - d) * kFast_;
            const float w = std::sin(mx * kHalfPi) * go;
            const float a = fxOut_[0][i], b = fxOut_[1][i];
            float x0 = cur_[0][i], x1 = cur_[1][i];
            if (os2) {
                dm += (s.dlyMixTarget - dm) * kFast_;
                x0 += (dly_[0][i] - x0) * dm;
                x1 += (dly_[1][i] - x1) * dm;
            }
            cur_[0][i] = x0 * d + a * w;
            cur_[1][i] = x1 * d + b * w;
            const float m = std::fabs(a) > std::fabs(b) ? std::fabs(a) : std::fabs(b);
            if (m > blockMax) blockMax = m;
        }
        if (std::fabs(go - s.gOutTarget) < 1e-5f) go = s.gOutTarget;
        if (std::fabs(gi - s.gInTarget) < 1e-6f) s.gIn = s.gInTarget;
        if (std::fabs(mx - s.mixTarget) < 1e-5f) mx = s.mixTarget;
        if (std::fabs(d - s.dryTarget) < 1e-4f) d = s.dryTarget;
        if (std::fabs(dm - s.dlyMixTarget) < 1e-4f) dm = s.dlyMixTarget;
        s.dlyMix = dm;
        s.gOut = go;
        s.mix = mx;
        s.dry = d;
        if (!feeding && blockMax < kQuietLevel) s.quietSamples += n;
        else if (blockMax >= kQuietLevel) s.quietSamples = 0;
    }

    {
        const Slot& c = fx_[FX_COMPRESSOR];
        float gr = 0.f;
        if (c.on && !bypass_)
            for (auto* z : c.grZones)
                if (z && *z > gr) gr = *z;
        grDb_.store(gr, std::memory_order_relaxed);
    }

    // ---------------------------------------------------------------- output
    float pk[2] = {0.f, 0.f};
    uint32_t clips = 0;
    float* outs[2] = {outL, outR};
    for (int i = 0; i < n; ++i) {
        for (int c = 0; c < 2; ++c) {
            float y = cur_[c][i];
            const float a = std::fabs(y);
            if (a > pk[c]) pk[c] = a;
            if (a >= 1.f) { ++clips; y = y > 0.f ? 1.f : -1.f; }
            float& s1 = vuState_[c][0];
            float& s2 = vuState_[c][1];
            s1 += (a - s1) * kVu_;
            s2 += (s1 - s2) * kVu_;
            outs[c][i] = y;
        }
    }

    for (int c = 0; c < 2; ++c) {
        vuOut_[c].store(vuState_[c][1] * kSineRmsCal, std::memory_order_relaxed);
        if (pk[c] > peakOut_[c].load(std::memory_order_relaxed)) peakOut_[c].store(pk[c], std::memory_order_relaxed);
        if (inPk[c] > peakIn_[c].load(std::memory_order_relaxed)) peakIn_[c].store(inPk[c], std::memory_order_relaxed);
    }
    monoDetected_.store(inputMode_ == INPUT_AUTO && mono, std::memory_order_relaxed);
    if (clips) outClips_.fetch_add(clips, std::memory_order_relaxed);
    if (inClips) inClips_.fetch_add(inClips, std::memory_order_relaxed);
    if (faults) faults_.fetch_add(faults, std::memory_order_relaxed);
}

void EngineCore::readMeters(MeterReadout& r) {
    for (int c = 0; c < 2; ++c) {
        r.vu[c] = vuOut_[c].load(std::memory_order_relaxed);
        r.peak[c] = peakOut_[c].exchange(0.f, std::memory_order_relaxed);
        r.inPeak[c] = peakIn_[c].exchange(0.f, std::memory_order_relaxed);
    }
    r.outClips = outClips_.exchange(0, std::memory_order_relaxed);
    r.inClips = inClips_.exchange(0, std::memory_order_relaxed);
    r.dspFaults = faults_.exchange(0, std::memory_order_relaxed);
    r.monoDetected = monoDetected_.load(std::memory_order_relaxed);
    r.gainReductionDb = grDb_.load(std::memory_order_relaxed);
}

} // namespace fxbox
