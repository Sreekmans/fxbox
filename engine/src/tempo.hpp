// Tempo logic: MIDI clock tracking (24 PPQN) with jitter filtering, tap tempo.
// Pure logic with explicit timestamps (seconds, monotonic), so it can be tested.
#pragma once
#include <array>
#include <cstddef>

namespace fxbox {

constexpr double kMinBpm = 30.0;
constexpr double kMaxBpm = 300.0;

class ClockTracker {
public:
    // Call for every MIDI clock (0xF8) with its arrival time.
    void tick(double t);
    // Call regularly; returns true while clock is being received.
    bool receiving(double now) const;
    // Filtered tempo, valid once hasTempo() is true.
    bool hasTempo() const { return hasTempo_; }
    double bpm() const { return bpm_; }
    // Start (0xFA): the next clock is beat 1.
    void start() { pendingStart_ = true; }
    // Returns true once when the first clock after a Start has arrived.
    bool takeStartedFlag() { bool f = started_; started_ = false; return f; }
    void reset();

    // Clock is considered lost after this many seconds without a tick.
    static constexpr double kLostAfter = 0.5;

private:
    static constexpr size_t kWindow = 96;   // 4 quarter notes
    std::array<double, kWindow> t_{};
    size_t count_ = 0;     // ticks in window (<= kWindow)
    size_t head_ = 0;      // next write index
    double last_ = -1.0;
    double bpm_ = 120.0;
    bool hasTempo_ = false;
    int divergent_ = 0;
    bool snap_ = false;
    bool pendingStart_ = false;
    bool started_ = false;

    double slopeOverLast(size_t n) const;   // seconds per tick (least squares)
};

class TapTempo {
public:
    // Returns true when a new tempo is available (from the 2nd tap on).
    bool tap(double t);
    double bpm() const { return bpm_; }
private:
    std::array<double, 5> taps_{};
    size_t n_ = 0;
    double bpm_ = 120.0;
    static constexpr double kResetAfter = 2.0;   // seconds between taps
};

// Applies a deadband so tiny tempo fluctuations do not re-trigger parameter
// updates (each delay-time change bends pitch on a tape echo).
class TempoDeadband {
public:
    // Returns true when the applied tempo changed.
    bool update(double bpm) {
        if (!init_ || bpm <= 0) { applied_ = bpm; init_ = true; return true; }
        const double rel = (bpm - applied_) / applied_;
        if (rel > kRel || rel < -kRel) { applied_ = bpm; return true; }
        return false;
    }
    double applied() const { return applied_; }
    void force(double bpm) { applied_ = bpm; init_ = true; }
private:
    static constexpr double kRel = 0.002;   // 0.2 %
    double applied_ = 120.0;
    bool init_ = false;
};

// Musical divisions are expressed in quarter-note beats (1/4 = 1, 1/8D = 0.75).
inline double divisionToMs(double beats, double bpm) { return beats * 60000.0 / bpm; }
inline double divisionToHz(double beatsPerCycle, double bpm) { return bpm / 60.0 / beatsPerCycle; }

} // namespace fxbox
