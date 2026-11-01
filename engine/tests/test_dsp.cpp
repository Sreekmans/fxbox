// Offline tests for the audio core: stability, timing, transitions, metering.
// Runs without JACK or hardware. Exit code 0 = all passed.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "engine_core.hpp"

using namespace fxbox;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
                              else { std::printf("  ok:   "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

constexpr int SR = 48000;
constexpr int BS = 128;

struct Run {
    std::vector<float> l, r;
};

using Gen = std::function<void(int /*sampleIndex*/, float& /*l*/, float& /*r*/)>;

static Run render(EngineCore& c, double seconds, const Gen& gen, int startIndex = 0,
                  const std::function<void(int)>& atBlock = nullptr) {
    const int total = int(seconds * SR);
    Run out;
    out.l.resize(total);
    out.r.resize(total);
    std::vector<float> il(BS), ir(BS);
    for (int pos = 0, b = 0; pos < total; pos += BS, ++b) {
        if (atBlock) atBlock(b);
        const int n = std::min(BS, total - pos);
        for (int i = 0; i < n; ++i) gen(startIndex + pos + i, il[i], ir[i]);
        c.process(il.data(), ir.data(), out.l.data() + pos, out.r.data() + pos, n);
    }
    return out;
}

static void init(EngineCore& c) {
    if (!c.init(SR, 4096)) { std::printf("init failed\n"); std::exit(2); }
}

static double rms(const std::vector<float>& v, int a, int b) {
    double s = 0; for (int i = a; i < b; ++i) s += double(v[i]) * v[i];
    return std::sqrt(s / std::max(1, b - a));
}
static bool allFinite(const Run& r) {
    for (float x : r.l) if (!std::isfinite(x)) return false;
    for (float x : r.r) if (!std::isfinite(x)) return false;
    return true;
}
// Largest sample-to-sample jump; for a click test against a smooth signal.
static float maxStep(const std::vector<float>& v, int a, int b) {
    float m = 0; for (int i = std::max(1, a); i < b; ++i) m = std::max(m, std::fabs(v[i] - v[i - 1])); return m;
}

static void setFx(EngineCore& c, int fx, const float p[4]) {
    for (int i = 0; i < 4; ++i) {
        if (kFxSpecs[fx].params[i].kind == ParamKind::Mix) c.push({Command::SetMix, uint8_t(fx), 0, p[i]});
        else c.push({Command::SetZone, uint8_t(fx), uint8_t(i), p[i]});
    }
    // Only this effect on in the chain.
    for (int f = 0; f < kNumFx; ++f) c.push({Command::SetEnabled, uint8_t(f), 0, f == fx ? 1.f : 0.f});
}

static double goertzel(const std::vector<float>& v, int a, int b, double freq) {
    const double w = 2 * M_PI * freq / SR, cw = 2 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (int i = a; i < b; ++i) { const double s0 = v[i] + cw * s1 - s2; s2 = s1; s1 = s0; }
    return std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - cw * s1 * s2)) / (b - a);
}

static void testStabilityWorstCase() {
    std::printf("[stability] max feedback, loud noise, 20 s per effect\n");
    const float worst[kNumFx][4] = {
        {2000.f, 0.95f, 1.f, 1.f}, {15.f, 1.f, 1.f, 250.f}, {10.f, 1.f, 1.f, 0.9f}, {10.f, 1.f, 1.f, 0.9f},
        {1.f, 1.f, 1.f, 6.f}, {1.f, 1.f, 1.f, 6.f}, {16000.f, 1.f, 10.f, 1.f}, {-40.f, 20.f, 0.1f, 24.f}};
    for (int fx = 0; fx < kNumFx; ++fx) {
        EngineCore c; init(c);
        setFx(c, fx, worst[fx]);
        std::mt19937 rng(1);
        std::uniform_real_distribution<float> d(-0.7f, 0.7f);
        Run r = render(c, 20.0, [&](int, float& l, float& rr) { l = d(rng); rr = d(rng); });
        MeterReadout m; c.readMeters(m);
        CHECK(allFinite(r) && m.dspFaults == 0, "%s: output finite, no DSP faults", kFxSpecs[fx].id);
        // After the input stops the output must not grow (feedback is bounded).
        Run tail = render(c, 6.0, [](int, float& l, float& rr) { l = rr = 0.f; });
        const double first = rms(tail.l, 0, SR), last = rms(tail.l, 5 * SR, 6 * SR);
        CHECK(last <= first * 1.05 + 1e-9, "%s: tail does not grow (rms %.4f -> %.4f)", kFxSpecs[fx].id, first, last);
    }
}

static void testEchoTiming() {
    std::printf("[timing] tape echo first repeat\n");
    for (float ms : {100.f, 375.f, 1000.f}) {
        EngineCore c; init(c);
        const float p[4] = {ms, 0.f, 1.f, 0.f};
        setFx(c, FX_TAPE, p);
        render(c, 1.5, [](int, float& l, float& r) { l = r = 0.f; });   // let the time glide settle
        Run r = render(c, 1.5, [](int i, float& l, float& rr) { l = rr = (i == 0) ? 1.f : 0.f; });
        int arg = 0; float best = 0;
        for (int i = 0; i < int(r.l.size()); ++i) if (std::fabs(r.l[i]) > best) { best = std::fabs(r.l[i]); arg = i; }
        const double got = arg * 1000.0 / SR;
        CHECK(std::fabs(got - ms) < 1.0, "time %.0f ms -> first repeat at %.2f ms", ms, got);
    }
}

static void testBypassTransparent() {
    std::printf("[bypass] dry path is unity, no tails when trails are off\n");
    EngineCore c; init(c);
    const float p[4] = {300.f, 0.5f, 0.5f, 0.2f};
    setFx(c, FX_TAPE, p);
    c.push({Command::Trails, 0, 0, 0.f});
    c.push({Command::Bypass, 0, 0, 1.f});
    render(c, 0.5, [](int, float& l, float& r) { l = r = 0.f; });
    auto sine = [](int i, float& l, float& r) { l = r = 0.5f * std::sin(2 * M_PI * 440.0 * i / SR); };
    Run r = render(c, 1.0, sine);
    double err = 0;
    for (int i = 0; i < int(r.l.size()); ++i) { float x, y; sine(i, x, y); err = std::max(err, double(std::fabs(r.l[i] - x))); }
    CHECK(err < 1e-4, "bypass output equals input (max error %.2e)", err);
}

static void testClickFree() {
    std::printf("[transitions] no clicks on bypass, effect switch, mix jump\n");
    const double f = 220.0, a = 0.5;
    const float natural = float(2 * M_PI * f / SR * a);   // largest step of the clean sine
    auto sine = [&](int i, float& l, float& r) { l = r = float(a * std::sin(2 * M_PI * f * i / SR)); };
    struct Case { const char* name; std::function<void(EngineCore&)> act; float level = 1.f; };
    std::vector<Case> cases = {
        {"bypass on",   [](EngineCore& c) { c.push({Command::Bypass, 0, 0, 1.f}); }},
        {"chorus off, reverb on", [](EngineCore& c) { c.push({Command::SetEnabled, FX_CHORUS, 0, 0.f});
                                                       c.push({Command::SetEnabled, FX_REVERB, 0, 1.f}); }},
        {"phaser on", [](EngineCore& c) { c.push({Command::SetEnabled, FX_PHASER, 0, 1.f}); }},
        {"filter on (cutoff above the tone)", [](EngineCore& c) { c.push({Command::SetZone, FX_FILTER, 0, 8000.f});
                                                                   c.push({Command::SetEnabled, FX_FILTER, 0, 1.f}); }},
        {"mix 0 -> 1",  [](EngineCore& c) { c.push({Command::SetMix, FX_CHORUS, 0, 1.f}); }},
        {"input gain +12 dB", [](EngineCore& c) { c.push({Command::InputGainDb, 0, 0, 12.f}); }, 3.98f},
    };
    for (auto& cs : cases) {
        EngineCore c; init(c);
        const float p[4] = {0.5f, 0.3f, 0.f, 0.f};
        setFx(c, FX_CHORUS, p);   // chorus at mix 0 = clean sine path before the change
        Run warm = render(c, 1.0, sine);
        Run r = render(c, 0.5, sine, int(warm.l.size()), [&](int b) { if (b == 10) cs.act(c); });
        const int a0 = 10 * BS - 64, a1 = 10 * BS + SR / 20;   // around the change, 50 ms
        const float step = maxStep(r.l, a0, a1);
        // A smooth transition may add new spectral content (the effect), but no
        // discontinuity: the step must stay within a small multiple of the sine's.
        // (level: the signal itself gets louder by this factor in the gain case)
        CHECK(step < natural * cs.level * 2.0f, "%s: max step %.4f (sine alone %.4f, x%.1f level)", cs.name, step, natural, cs.level);
    }
}

static void testMonoInput() {
    std::printf("[input] mono mode feeds left input to both channels\n");
    EngineCore c; init(c);
    c.push({Command::Trails, 0, 0, 0.f});
    c.push({Command::Bypass, 0, 0, 1.f});
    c.push({Command::InputMode, 0, 0, float(INPUT_MONO)});
    render(c, 0.2, [](int, float& l, float& r) { l = r = 0.f; });
    Run r = render(c, 0.5, [](int i, float& l, float& rr) { l = 0.3f * std::sin(i * 0.05f); rr = 0.9f; });
    double err = 0;
    for (int i = 0; i < int(r.l.size()); ++i) err = std::max(err, double(std::fabs(r.l[i] - r.r[i])));
    CHECK(err < 1e-6, "left and right outputs identical (max diff %.2e)", err);
}

static void testAutoMono() {
    std::printf("[input] auto mode: silent right input -> mono, signal on right -> stereo\n");
    EngineCore c; init(c);   // INPUT_AUTO is the default
    c.push({Command::Trails, 0, 0, 0.f});
    c.push({Command::Bypass, 0, 0, 1.f});
    Run r = render(c, 1.0, [](int i, float& l, float& rr) { l = 0.3f * std::sin(i * 0.05f); rr = 0.f; });
    double err = 0;
    for (int i = SR / 2; i < SR; ++i) err = std::max(err, double(std::fabs(r.l[i] - r.r[i])));
    MeterReadout m; c.readMeters(m);
    CHECK(err < 1e-4 && m.monoDetected, "mono source detected, right output = left (max diff %.2e)", err);
    Run r2 = render(c, 0.5, [](int i, float& l, float& rr) { l = 0.3f * std::sin(i * 0.05f); rr = 0.2f * std::sin(i * 0.11f); });
    double diff = 0;
    for (int i = SR / 4; i < SR / 2; ++i) diff = std::max(diff, double(std::fabs(r2.r[i] - 0.2f * std::sin(i * 0.11f))));
    c.readMeters(m);
    CHECK(diff < 1e-3 && !m.monoDetected, "stereo again once the right input carries signal (max diff %.2e)", diff);
}

static void testNewEffects() {
    std::printf("[effects] filter, distortion, saturation\n");
    {   // Ladder filter: low-pass at 300 Hz, no resonance.
        EngineCore c; init(c);
        const float p[4] = {300.f, 0.f, 0.5f, 0.f};
        setFx(c, FX_FILTER, p);
        render(c, 0.5, [](int, float& l, float& r) { l = r = 0.f; });
        Run lo = render(c, 1.0, [](int i, float& l, float& r) { l = r = 0.3f * std::sin(2 * M_PI * 100.0 * i / SR); });
        Run hi = render(c, 1.0, [](int i, float& l, float& r) { l = r = 0.3f * std::sin(2 * M_PI * 5000.0 * i / SR); });
        const double gLo = 20 * std::log10(rms(lo.l, SR / 2, SR) / (0.3 / std::sqrt(2.0)));
        const double gHi = 20 * std::log10(rms(hi.l, SR / 2, SR) / (0.3 / std::sqrt(2.0)) + 1e-12);
        CHECK(gLo > -4.0, "filter passes 100 Hz (%.1f dB)", gLo);
        CHECK(gHi < -40.0, "filter cuts 5 kHz at 300 Hz cutoff (%.1f dB)", gHi);
    }
    {   // Distortion: strong 3rd harmonic at full drive.
        EngineCore c; init(c);
        const float p[4] = {1.f, 1.f, 1.f, -6.f};
        setFx(c, FX_DISTORTION, p);
        Run r = render(c, 1.0, [](int i, float& l, float& rr) { l = rr = 0.3f * std::sin(2 * M_PI * 200.0 * i / SR); });
        const double h1 = goertzel(r.l, SR / 2, SR, 200.0), h3 = goertzel(r.l, SR / 2, SR, 600.0);
        CHECK(20 * std::log10(h3 / h1) > -20.0, "distortion 3rd harmonic %.1f dB relative to fundamental", 20 * std::log10(h3 / h1));
    }
    {   // Saturation: about unity gain for quiet signals, harmonics for loud ones.
        EngineCore c; init(c);
        const float p[4] = {0.5f, 0.3f, 1.f, 0.f};
        setFx(c, FX_SATURATION, p);
        Run q = render(c, 1.0, [](int i, float& l, float& r) { l = r = 0.02f * std::sin(2 * M_PI * 200.0 * i / SR); });
        const double g = 20 * std::log10(rms(q.l, SR / 2, SR) / (0.02 / std::sqrt(2.0)));
        CHECK(std::fabs(g) < 1.5, "saturation small-signal gain %.2f dB", g);
        Run l = render(c, 1.0, [](int i, float& a, float& b) { a = b = 0.7f * std::sin(2 * M_PI * 200.0 * i / SR); });
        const double h1 = goertzel(l.l, SR / 2, SR, 200.0), h2 = goertzel(l.l, SR / 2, SR, 400.0), h3 = goertzel(l.l, SR / 2, SR, 600.0);
        CHECK(20 * std::log10((h2 + h3) / h1) > -40.0, "saturation adds harmonics on loud signals (%.1f dB)", 20 * std::log10((h2 + h3) / h1));
    }
}

static void testFullChain() {
    std::printf("[chain] all eight effects on at once, loud noise, 20 s\n");
    EngineCore c; init(c);
    const float vals[kNumFx][4] = {{500.f, 0.8f, 0.4f, 0.5f}, {6.f, 0.6f, 0.4f, 40.f}, {0.6f, 0.6f, 0.5f, 0.5f},
                                   {0.3f, 0.8f, 0.5f, 0.7f}, {0.8f, 0.5f, 1.f, -6.f}, {0.7f, 0.5f, 1.f, 0.f},
                                   {2000.f, 0.8f, 0.5f, 0.6f}, {-24.f, 6.f, 5.f, 6.f}};
    for (int f = 0; f < kNumFx; ++f) {
        for (int i = 0; i < 4; ++i) {
            if (kFxSpecs[f].params[i].kind == ParamKind::Mix) c.push({Command::SetMix, uint8_t(f), 0, vals[f][i]});
            else c.push({Command::SetZone, uint8_t(f), uint8_t(i), vals[f][i]});
        }
        c.push({Command::SetEnabled, uint8_t(f), 0, 1.f});
    }
    std::mt19937 rng(3);
    std::uniform_real_distribution<float> d(-0.7f, 0.7f);
    auto t0 = std::chrono::steady_clock::now();
    Run r = render(c, 20.0, [&](int, float& l, float& rr) { l = d(rng); rr = d(rng); });
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    MeterReadout m; c.readMeters(m);
    CHECK(allFinite(r) && m.dspFaults == 0, "output finite, no DSP faults");
    std::printf("  info: full chain 20 s audio in %.3f s (%.1fx realtime, this machine)\n", sec, 20.0 / sec);
    Run tail = render(c, 8.0, [](int, float& l, float& rr) { l = rr = 0.f; });
    const double first = rms(tail.l, 0, SR), last = rms(tail.l, 7 * SR, 8 * SR);
    CHECK(last <= first * 1.05 + 1e-9, "chain tail does not grow (rms %.4f -> %.4f)", first, last);
}

static void testMeters() {
    std::printf("[meters] calibration: sine at -18 dBFS RMS in bypass\n");
    EngineCore c; init(c);
    c.push({Command::Trails, 0, 0, 0.f});
    c.push({Command::Bypass, 0, 0, 1.f});
    const double amp = std::pow(10.0, -18.0 / 20.0) * std::sqrt(2.0);   // peak of a -18 dBFS RMS sine
    render(c, 1.0, [&](int i, float& l, float& r) { l = r = float(amp * std::sin(2 * M_PI * 1000.0 * i / SR)); });
    MeterReadout m; c.readMeters(m);
    const double vuDb = 20 * std::log10(m.vu[0]);
    const double pkDb = 20 * std::log10(m.peak[0]);
    CHECK(std::fabs(vuDb + 18.0) < 0.3, "VU level %.2f dBFS (expected -18.0 +/- 0.3)", vuDb);
    CHECK(std::fabs(pkDb + 15.0) < 0.1, "sample peak %.2f dBFS (expected -14.99)", pkDb);
    CHECK(m.outClips == 0 && m.inClips == 0, "no clip flagged");

    // VU rise time: from silence to a step, ~99 % after 300 ms.
    EngineCore c2; init(c2);
    c2.push({Command::Trails, 0, 0, 0.f});
    c2.push({Command::Bypass, 0, 0, 1.f});
    render(c2, 0.5, [](int, float& l, float& r) { l = r = 0.f; });
    std::vector<float> vu;
    render(c2, 0.6, [&](int i, float& l, float& r) { l = r = float(amp * std::sin(2 * M_PI * 1000.0 * i / SR)); },
           0, [&](int) { MeterReadout mm; c2.readMeters(mm); vu.push_back(mm.vu[0]); });
    const int at300 = int(0.300 * SR / BS);
    const double frac = vu[at300] / (amp / std::sqrt(2.0));
    CHECK(frac > 0.97 && frac < 1.03, "VU after 300 ms: %.1f %% of final value", frac * 100.0);

    // Clip detection.
    EngineCore c3; init(c3);
    c3.push({Command::Trails, 0, 0, 0.f});
    c3.push({Command::Bypass, 0, 0, 1.f});
    c3.push({Command::InputGainDb, 0, 0, 12.f});
    render(c3, 0.5, [](int i, float& l, float& r) { l = r = 0.9f * std::sin(i * 0.01f); });
    MeterReadout m3; c3.readMeters(m3);
    CHECK(m3.outClips > 0, "output clipping detected (%u samples)", m3.outClips);
}

static void testPerformance() {
    std::printf("[performance] this machine (NOT a Raspberry Pi measurement)\n");
    for (int fx = 0; fx < kNumFx; ++fx) {
        EngineCore c; init(c);
        const float p[4] = {0.5f, 0.5f, 0.5f, 0.5f};
        float pp[4];
        for (int i = 0; i < 4; ++i) pp[i] = std::max(kFxSpecs[fx].params[i].minV, std::min(kFxSpecs[fx].params[i].maxV, p[i]));
        if (fx == FX_TAPE) pp[0] = 400.f;
        if (fx == FX_REVERB) pp[0] = 3.f;
        if (fx == FX_FILTER) pp[0] = 1000.f;
        setFx(c, fx, pp);
        std::mt19937 rng(2);
        std::uniform_real_distribution<float> d(-0.3f, 0.3f);
        auto t0 = std::chrono::steady_clock::now();
        render(c, 30.0, [&](int, float& l, float& r) { l = d(rng); r = l; });
        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("  info: %-10s 30 s audio in %.3f s  (%.1fx realtime)\n", kFxSpecs[fx].id, sec, 30.0 / sec);
    }
}

static void setZone(EngineCore& c, int fx, int idx, float v) { c.push({Command::SetZone, uint8_t(fx), uint8_t(idx), v}); }
static double gainDb(const Run& r, double amp) { return 20 * std::log10(rms(r.l, SR / 2, SR) / (amp / std::sqrt(2.0)) + 1e-12); }
static Run sine(EngineCore& c, double hz, float amp, double sec = 1.0) {
    return render(c, sec, [=](int i, float& l, float& r) { l = r = amp * std::sin(2 * M_PI * hz * i / SR); });
}

static void testFilterModes() {
    std::printf("[v1.2] filter modes (cutoff 1 kHz)\n");
    const char* names[5] = {"ladder", "LP", "HP", "BP", "notch"};
    for (int mode = 0; mode < 5; ++mode) {
        EngineCore c; init(c);
        const float p[4] = {1000.f, 0.f, 0.5f, 0.f};
        setFx(c, FX_FILTER, p);
        setZone(c, FX_FILTER, 4, float(mode));
        sine(c, 100, 0.f, 0.3);
        const double lo = gainDb(sine(c, 100.0, 0.3f), 0.3), mid = gainDb(sine(c, 1000.0, 0.3f), 0.3),
                     hi = gainDb(sine(c, 10000.0, 0.3f), 0.3);
        bool ok = false;
        switch (mode) {
            case 0: case 1: ok = lo > -3 && hi < -30; break;
            case 2: ok = lo < -30 && hi > -3; break;
            case 3: ok = lo < -12 && hi < -12 && mid > -8; break;   // SVF band-pass: -6 dB peak at reso 0
            case 4: ok = lo > -3 && hi > -3 && mid < -20; break;
        }
        CHECK(ok, "%-6s 100 Hz %6.1f dB, 1 kHz %6.1f dB, 10 kHz %6.1f dB", names[mode], lo, mid, hi);
    }
    {   // LFO shapes and envelope stay finite with heavy settings.
        EngineCore c; init(c);
        const float p[4] = {800.f, 0.9f, 8.f, 1.f};
        setFx(c, FX_FILTER, p);
        bool fin = true;
        for (int shape = 0; shape < 4; ++shape) {
            setZone(c, FX_FILTER, 5, float(shape));
            setZone(c, FX_FILTER, 6, 1.f);
            setZone(c, FX_FILTER, 7, 1.f);
            std::mt19937 rng(shape);
            std::uniform_real_distribution<float> d(-0.8f, 0.8f);
            fin = fin && allFinite(render(c, 3.0, [&](int, float& l, float& r) { l = d(rng); r = d(rng); }));
        }
        MeterReadout m; c.readMeters(m);
        CHECK(fin && m.dspFaults == 0, "all LFO shapes + envelope + drive: finite, no faults");
    }
    {   // Mode switch while playing: crossfade, no click.
        EngineCore c; init(c);
        const float p[4] = {2000.f, 0.f, 0.5f, 0.f};
        setFx(c, FX_FILTER, p);
        setZone(c, FX_FILTER, 4, 1.f);
        sine(c, 220, 0.3f, 0.5);
        Run ref = sine(c, 220, 0.3f, 0.2);
        const float natural = maxStep(ref.l, 0, int(ref.l.size()));
        setZone(c, FX_FILTER, 4, 2.f);   // LP -> HP
        Run sw = sine(c, 220, 0.3f, 0.2);
        const float step = maxStep(sw.l, 0, int(sw.l.size()));
        CHECK(step < natural * 2.f, "filter LP->HP switch: max step %.4f (sine alone %.4f)", step, natural);
    }
}

static void testDistortionModes() {
    std::printf("[v1.2] distortion modes (2x oversampled)\n");
    const char* names[5] = {"overdrive", "distortion", "fuzz", "bitcrush", "wavefold"};
    for (int mode = 0; mode < 5; ++mode) {
        EngineCore c; init(c);
        const float p[4] = {0.8f, 0.6f, 1.f, -6.f};
        setFx(c, FX_DISTORTION, p);
        setZone(c, FX_DISTORTION, 4, float(mode));
        setZone(c, FX_DISTORTION, 5, 0.6f);
        setZone(c, FX_DISTORTION, 6, 0.3f);
        std::mt19937 rng(mode);
        std::uniform_real_distribution<float> d(-0.9f, 0.9f);
        Run r = render(c, 5.0, [&](int, float& l, float& rr) { l = d(rng); rr = d(rng); });
        MeterReadout m; c.readMeters(m);
        Run s = sine(c, 200.0, 0.3f);
        const double out = rms(s.l, SR / 2, SR);
        CHECK(allFinite(r) && m.dspFaults == 0 && out > 1e-3, "%-10s finite, no faults, audible (rms %.3f)", names[mode], out);
    }
    {   // Aliasing: 7 kHz sine at drive 0.5. All harmonics and their aliases (at 48 or
        // 96 kHz) fall on whole kHz; harmonics are multiples of 7 kHz, everything else is
        // aliasing. Compare against the same Faust code run directly at 48 kHz (1x).
        // Bitcrush aliases on purpose (rate reduction) and is skipped.
        auto aliasDb = [](const std::vector<float>& o) {
            double h = 0, a = 0;
            for (int k = 1; k <= 23; ++k) {
                double g = goertzel(o, SR / 2, SR, k * 1000.0); g *= g;
                (k % 7 == 0 ? h : a) += g;
            }
            return 10 * std::log10(a / h);
        };
        for (int mode : {0, 1, 2, 4}) {
            EngineCore c; init(c);
            const float p[4] = {0.5f, 1.f, 1.f, 0.f};
            setFx(c, FX_DISTORTION, p);
            setZone(c, FX_DISTORTION, 4, float(mode));
            Run r2 = sine(c, 7000.0, 0.5f);
            fxfaust::DistortionDsp d; d.init(SR);
            ZoneCollector zc; d.buildUserInterface(&zc);
            *zc.find("drive") = 0.5f; *zc.find("tone") = 1.f; *zc.find("level") = 0.f; *zc.find("mode") = float(mode);
            std::vector<float> in(SR), o1(SR), o1r(SR);
            for (int i = 0; i < SR; ++i) in[i] = 0.5f * std::sin(2 * M_PI * 7000.0 * i / SR);
            for (int pos = 0; pos < SR; pos += BS) {
                float* ins[2] = {&in[pos], &in[pos]};
                float* outs[2] = {&o1[pos], &o1r[pos]};
                d.compute(BS, ins, outs);
            }
            const double a1 = aliasDb(o1), a2 = aliasDb(r2.l);
            CHECK(a2 < a1 - 5.0, "%-10s aliasing %.1f dB (without oversampling %.1f dB)", names[mode], a2, a1);
        }
    }
    {   // Parallel mix: the dry path is delayed to match the oversampler (no comb filter).
        EngineCore c; init(c);
        const float p[4] = {0.f, 1.f, 0.5f, 0.f};   // almost clean, half mix
        setFx(c, FX_DISTORTION, p);
        sine(c, 100, 0.f, 0.3);
        double minG = 1e9, maxG = -1e9;
        for (double hz : {500.0, 1000.0, 2000.0, 4000.0, 8000.0}) {
            Run r = sine(c, hz, 0.1f, 0.5);
            const double g = 20 * std::log10(rms(r.l, SR / 4, SR / 2) / (0.1 / std::sqrt(2.0)));
            minG = std::min(minG, g); maxG = std::max(maxG, g);
        }
        CHECK(maxG - minG < 3.0, "50 %% mix, flat response 0.5-8 kHz (spread %.1f dB)", maxG - minG);
    }
    {   // Switching the oversampled effect on/off while playing: no click.
        EngineCore c; init(c);
        const float p[4] = {0.f, 1.f, 0.3f, 0.f};
        setFx(c, FX_DISTORTION, p);
        c.push({Command::SetEnabled, FX_DISTORTION, 0, 0.f});
        sine(c, 440, 0.3f, 0.3);
        Run ref = sine(c, 440, 0.3f, 0.2);
        const float natural = maxStep(ref.l, 0, int(ref.l.size()));
        c.push({Command::SetEnabled, FX_DISTORTION, 0, 1.f});
        Run on = sine(c, 440, 0.3f, 0.3);
        c.push({Command::SetEnabled, FX_DISTORTION, 0, 0.f});
        Run off = sine(c, 440, 0.3f, 0.3);
        const float step = std::max(maxStep(on.l, 0, int(on.l.size())), maxStep(off.l, 0, int(off.l.size())));
        CHECK(step < natural * 2.f, "distortion on/off: max step %.4f (sine alone %.4f)", step, natural);
    }
}

static void testCompressor() {
    std::printf("[v1.2] compressor / limiter\n");
    {   // -6 dBFS peak sine, threshold -20, ratio 4, hard knee, limiter off.
        EngineCore c; init(c);
        const float p[4] = {-20.f, 4.f, 5.f, 0.f};
        setFx(c, FX_COMPRESSOR, p);
        c.push({Command::SetMix, FX_COMPRESSOR, 0, 1.f});
        setZone(c, FX_COMPRESSOR, 5, 0.f);
        setZone(c, FX_COMPRESSOR, 7, 0.f);
        const float amp = 0.5f;   // -6 dBFS peak
        Run r = sine(c, 1000.0, amp, 1.0);
        MeterReadout m; c.readMeters(m);
        const double inPk = 20 * std::log10(amp);
        double pk = 0; for (int i = SR / 2; i < SR; ++i) pk = std::max(pk, double(std::fabs(r.l[i])));
        const double outPk = 20 * std::log10(pk);
        const double expected = -20 + (inPk + 20) / 4;   // -16.5 dB
        CHECK(std::fabs(outPk - expected) < 1.5, "out peak %.1f dBFS (expected about %.1f)", outPk, expected);
        CHECK(std::fabs(m.gainReductionDb - (inPk - expected)) < 1.5, "gain reduction reported: %.1f dB", m.gainReductionDb);
    }
    {   // Limiter: +24 dB makeup on a loud signal stays below full scale.
        EngineCore c; init(c);
        const float p[4] = {-10.f, 2.f, 10.f, 24.f};
        setFx(c, FX_COMPRESSOR, p);
        c.push({Command::SetMix, FX_COMPRESSOR, 0, 1.f});
        setZone(c, FX_COMPRESSOR, 7, 1.f);
        Run r = sine(c, 200.0, 0.5f, 1.5);
        double pk = 0; for (int i = SR; i < int(r.l.size()); ++i) pk = std::max(pk, double(std::fabs(r.l[i])));
        MeterReadout m; c.readMeters(m);
        CHECK(20 * std::log10(pk) < -0.3 && m.outClips == 0, "limiter: peak %.2f dBFS, no clipping", 20 * std::log10(pk));
    }
    {   // Off: no gain reduction reported.
        EngineCore c; init(c);
        const float p[4] = {-30.f, 8.f, 5.f, 0.f};
        setFx(c, FX_COMPRESSOR, p);
        c.push({Command::SetEnabled, FX_COMPRESSOR, 0, 0.f});
        sine(c, 1000.0, 0.5f, 0.5);
        MeterReadout m; c.readMeters(m);
        CHECK(m.gainReductionDb == 0.f, "switched off: gain reduction 0");
    }
}

int main() {
    testStabilityWorstCase();
    testEchoTiming();
    testBypassTransparent();
    testClickFree();
    testMonoInput();
    testAutoMono();
    testNewEffects();
    testFilterModes();
    testDistortionModes();
    testCompressor();
    testFullChain();
    testMeters();
    testPerformance();
    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
