// Realtime audio core: an effect chain in fixed order (see kChainOrder), each
// effect on/off with smooth transitions, global bypass, tails and metering.
// Independent of JACK so it can be tested offline.
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "effects.hpp"
#include "oversampler.hpp"
#include "spsc.hpp"

namespace fxbox {

struct Command {
    enum Type : uint8_t {
        SetZone,     // fx, idx (param index), value
        SetMix,      // fx, value 0..1
        SetEnabled,  // fx, value 0/1: effect on/off in the chain
        Bypass,      // value 0/1: whole chain bypassed
        InputMode,   // value 0 = stereo, 1 = mono (left feeds both), 2 = auto
        InputGainDb, // value dB
        Trails,      // value 0/1 (tails keep ringing when an effect is switched off / bypassed)
        LfoReset,    // all modulation LFOs restart at phase 0 (MIDI Start)
    };
    Type type;
    uint8_t fx = 0;
    uint8_t idx = 0;
    float value = 0.f;
};

enum InputModeValue { INPUT_STEREO = 0, INPUT_MONO = 1, INPUT_AUTO = 2 };

// Values read by the control thread. Levels are linear amplitude.
struct MeterReadout {
    float vu[2] = {0, 0};       // average-responding level, scaled to read RMS on a sine
    float peak[2] = {0, 0};     // sample peak since last read
    float inPeak[2] = {0, 0};
    uint32_t outClips = 0;      // samples at/over full scale since last read
    uint32_t inClips = 0;
    uint32_t dspFaults = 0;     // NaN/Inf recoveries (should stay 0)
    bool monoDetected = false;  // auto input mode is currently treating the input as mono
    float gainReductionDb = 0;  // compressor gain reduction (0 when it is off)
};

class EngineCore {
public:
    // Non-realtime: allocates and warms up all effects.
    bool init(int sampleRate, int maxBlock);
    // Realtime: no allocation, no locks, no system calls.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
    // Control thread.
    bool push(const Command& c) { return queue_.push(c); }
    void readMeters(MeterReadout& r);
    int sampleRate() const { return sr_; }

private:
    struct Slot {
        std::unique_ptr<fxfaust::dsp> dsp;
        FAUSTFLOAT* zones[kNumParams] = {};
        FAUSTFLOAT* lfoReset = nullptr;
        FAUSTFLOAT* grZones[2] = {nullptr, nullptr};   // compressor gain reduction (dB)
        int os = 1;                                   // oversampling factor (1 or 2)
        HalfBand2x up[2], down[2];
        FixedDelay dryDelay[2];
        float dlyMix = 0.f, dlyMixTarget = 0.f;       // 1 = dry path delayed to match the oversampler
        bool hasMix = true;
        bool on = false;
        float mixTarget = 0.3f, mix = 0.3f;
        float gIn = 0.f, gInTarget = 0.f;     // feed into the effect
        float gOut = 0.f, gOutTarget = 0.f;   // effect output level (tails)
        float dry = 1.f, dryTarget = 1.f;     // signal passing around the effect
        int quietSamples = 0;
    };

    void apply(const Command& c);
    void retarget();

    int sr_ = 48000;
    int maxBlock_ = 0;
    std::array<Slot, kNumFx> fx_;
    bool bypass_ = false;
    bool trails_ = true;
    int inputMode_ = INPUT_AUTO;
    int rightSilent_ = 0;
    float monoMix_ = 0.f;   // 0 = stereo, 1 = left copied to right
    float inGainTarget_ = 1.f, inGain_ = 1.f;

    float kFast_ = 0.f;     // ~10 ms  (gains, mix)
    float kVu_ = 0.f;       // VU integrator

    float vuState_[2][2] = {{0, 0}, {0, 0}};

    std::vector<float> cur_[2], fxIn_[2], fxOut_[2], dly_[2];
    std::vector<float> osIn_[2], osOut_[2];   // 2x-rate buffers

    SpscQueue<Command, 1024> queue_;

    std::atomic<float> vuOut_[2]{};
    std::atomic<float> peakOut_[2]{};
    std::atomic<float> peakIn_[2]{};
    std::atomic<uint32_t> outClips_{0}, inClips_{0}, faults_{0};
    std::atomic<bool> monoDetected_{false};
    std::atomic<float> grDb_{0.f};
};

} // namespace fxbox
