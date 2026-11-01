// Effect registry: the Faust-generated classes, how the engine's parameter
// indices map onto their controls, and the fixed chain order.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../dsp/generated/tape_echo.hpp"
#include "../dsp/generated/reverb.hpp"
#include "../dsp/generated/chorus.hpp"
#include "../dsp/generated/phaser.hpp"
#include "../dsp/generated/distortion.hpp"
#include "../dsp/generated/saturation.hpp"
#include "../dsp/generated/filter.hpp"
#include "../dsp/generated/compressor.hpp"

namespace fxbox {

constexpr int kNumFx = 8;
constexpr int kNumParams = 8;   // two pages of four on the screen
// Effect ids are stable (used in the protocol, presets and state files).
enum FxId { FX_TAPE = 0, FX_REVERB = 1, FX_CHORUS = 2, FX_PHASER = 3,
            FX_DISTORTION = 4, FX_SATURATION = 5, FX_FILTER = 6, FX_COMPRESSOR = 7 };

// Fixed signal order of the chain, like a pedalboard.
inline constexpr int kChainOrder[kNumFx] = {FX_FILTER, FX_SATURATION, FX_DISTORTION, FX_CHORUS,
                                            FX_PHASER, FX_TAPE, FX_REVERB, FX_COMPRESSOR};

enum class ParamKind {
    Zone,       // written to a Faust control
    Mix,        // dry/wet balance, handled by the engine
    None,       // unused slot
};
// Whether a parameter follows the tempo when sync is on.
enum class SyncKind { None, Time, Rate };

struct ParamSpec {
    const char* name;     // stable id used in the protocol and tests
    ParamKind kind;
    const char* zone;     // Faust control label (Zone only)
    float minV, maxV;     // safe range enforced by the engine
    SyncKind sync;
};

struct FxSpec {
    const char* id;
    ParamSpec params[kNumParams];
    int oversample = 1;          // 2: the engine runs this effect at twice the sample rate
};

#define FX_NONE {"", ParamKind::None, nullptr, 0.f, 0.f, SyncKind::None}

// Safety limits live here, independent of what the UI sends.
inline const FxSpec kFxSpecs[kNumFx] = {
    {"tape",   {{"time",     ParamKind::Zone, "time",     10.f, 2000.f, SyncKind::Time},
                {"feedback", ParamKind::Zone, "feedback", 0.f,  0.95f,  SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"wow",      ParamKind::Zone, "wow",      0.f,  1.f,    SyncKind::None},
                FX_NONE, FX_NONE, FX_NONE, FX_NONE}},
    {"reverb", {{"decay",    ParamKind::Zone, "decay",    0.2f, 15.f,   SyncKind::None},
                {"tone",     ParamKind::Zone, "tone",     0.f,  1.f,    SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"predelay", ParamKind::Zone, "predelay", 0.f,  250.f,  SyncKind::None},
                FX_NONE, FX_NONE, FX_NONE, FX_NONE}},
    {"chorus", {{"rate",     ParamKind::Zone, "rate",     0.02f, 10.f,  SyncKind::Rate},
                {"depth",    ParamKind::Zone, "depth",    0.f,  1.f,    SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"feedback", ParamKind::Zone, "feedback", 0.f,  0.9f,   SyncKind::None},
                FX_NONE, FX_NONE, FX_NONE, FX_NONE}},
    {"phaser", {{"rate",     ParamKind::Zone, "rate",     0.02f, 10.f,  SyncKind::Rate},
                {"depth",    ParamKind::Zone, "depth",    0.f,  1.f,    SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"feedback", ParamKind::Zone, "feedback", 0.f,  0.9f,   SyncKind::None},
                FX_NONE, FX_NONE, FX_NONE, FX_NONE}},
    {"distortion", {{"drive", ParamKind::Zone, "drive",   0.f,  1.f,    SyncKind::None},
                {"tone",     ParamKind::Zone, "tone",     0.f,  1.f,    SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"level",    ParamKind::Zone, "level",    -24.f, 6.f,   SyncKind::None},
                {"mode",     ParamKind::Zone, "mode",     0.f,  4.f,    SyncKind::None},
                {"crush",    ParamKind::Zone, "crush",    0.f,  1.f,    SyncKind::None},
                {"gate",     ParamKind::Zone, "gate",     0.f,  1.f,    SyncKind::None},
                FX_NONE}, 2},
    {"saturation", {{"drive", ParamKind::Zone, "drive",   0.f,  1.f,    SyncKind::None},
                {"warmth",   ParamKind::Zone, "warmth",   0.f,  1.f,    SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"level",    ParamKind::Zone, "level",    -12.f, 6.f,   SyncKind::None},
                FX_NONE, FX_NONE, FX_NONE, FX_NONE}},
    {"filter", {{"cutoff",   ParamKind::Zone, "cutoff",   40.f, 16000.f, SyncKind::None},
                {"reso",     ParamKind::Zone, "reso",     0.f,  1.f,    SyncKind::None},
                {"rate",     ParamKind::Zone, "rate",     0.02f, 10.f,  SyncKind::Rate},
                {"depth",    ParamKind::Zone, "depth",    0.f,  1.f,    SyncKind::None},
                {"mode",     ParamKind::Zone, "mode",     0.f,  4.f,    SyncKind::None},
                {"shape",    ParamKind::Zone, "shape",    0.f,  3.f,    SyncKind::None},
                {"env",      ParamKind::Zone, "env",      0.f,  1.f,    SyncKind::None},
                {"drive",    ParamKind::Zone, "drive",    0.f,  1.f,    SyncKind::None}}},
    {"compressor", {{"threshold", ParamKind::Zone, "threshold", -40.f, 0.f, SyncKind::None},
                {"ratio",    ParamKind::Zone, "ratio",    1.f,  20.f,   SyncKind::None},
                {"attack",   ParamKind::Zone, "attack",   0.1f, 100.f,  SyncKind::None},
                {"makeup",   ParamKind::Zone, "makeup",   0.f,  24.f,   SyncKind::None},
                {"release",  ParamKind::Zone, "release",  20.f, 1000.f, SyncKind::None},
                {"knee",     ParamKind::Zone, "knee",     0.f,  12.f,   SyncKind::None},
                {"mix",      ParamKind::Mix,  nullptr,    0.f,  1.f,    SyncKind::None},
                {"limiter",  ParamKind::Zone, "limiter",  0.f,  1.f,    SyncKind::None}}},
};

inline bool fxHasMix(int fx) {
    for (const auto& p : kFxSpecs[fx].params)
        if (p.kind == ParamKind::Mix) return true;
    return false;
}

inline std::unique_ptr<fxfaust::dsp> makeFaustDsp(int fx) {
    switch (fx) {
        case FX_TAPE:       return std::make_unique<fxfaust::TapeEchoDsp>();
        case FX_REVERB:     return std::make_unique<fxfaust::ReverbDsp>();
        case FX_CHORUS:     return std::make_unique<fxfaust::ChorusDsp>();
        case FX_PHASER:     return std::make_unique<fxfaust::PhaserDsp>();
        case FX_DISTORTION: return std::make_unique<fxfaust::DistortionDsp>();
        case FX_SATURATION: return std::make_unique<fxfaust::SaturationDsp>();
        case FX_FILTER:     return std::make_unique<fxfaust::FilterDsp>();
        case FX_COMPRESSOR: return std::make_unique<fxfaust::CompressorDsp>();
    }
    return nullptr;
}

// Collects zone pointers by label (non-realtime, at init).
struct ZoneCollector : public fxfaust::UI {
    std::vector<std::pair<std::string, FAUSTFLOAT*>> zones;
    std::vector<std::pair<std::string, FAUSTFLOAT*>> meters;   // bargraphs (outputs)
    void addHorizontalBargraph(const char* l, FAUSTFLOAT* z, FAUSTFLOAT, FAUSTFLOAT) override { meters.emplace_back(l, z); }
    void addVerticalBargraph(const char* l, FAUSTFLOAT* z, FAUSTFLOAT, FAUSTFLOAT) override { meters.emplace_back(l, z); }
    void add(const char* l, FAUSTFLOAT* z) { zones.emplace_back(l, z); }
    void addButton(const char* l, FAUSTFLOAT* z) override { add(l, z); }
    void addCheckButton(const char* l, FAUSTFLOAT* z) override { add(l, z); }
    void addVerticalSlider(const char* l, FAUSTFLOAT* z, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) override { add(l, z); }
    void addHorizontalSlider(const char* l, FAUSTFLOAT* z, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) override { add(l, z); }
    void addNumEntry(const char* l, FAUSTFLOAT* z, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT, FAUSTFLOAT) override { add(l, z); }
    FAUSTFLOAT* find(const std::string& l) const {
        for (auto& p : zones) if (p.first == l) return p.second;
        return nullptr;
    }
};

} // namespace fxbox
