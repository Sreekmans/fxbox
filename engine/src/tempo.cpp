#include "tempo.hpp"

#include <cmath>

namespace fxbox {

void ClockTracker::reset() {
    count_ = 0;
    head_ = 0;
    last_ = -1.0;
    divergent_ = 0;
    hasTempo_ = false;
}

bool ClockTracker::receiving(double now) const {
    return last_ >= 0.0 && (now - last_) < kLostAfter;
}

double ClockTracker::slopeOverLast(size_t n) const {
    // Least-squares fit of tick time against tick index over the last n ticks.
    // Robust against per-tick jitter (USB polling, UART buffering).
    if (n < 2) return 0.0;
    const size_t start = (head_ + kWindow - n) % kWindow;
    const double t0 = t_[start];
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (size_t i = 0; i < n; ++i) {
        const double x = double(i);
        const double y = t_[(start + i) % kWindow] - t0;
        sx += x; sy += y; sxx += x * x; sxy += x * y;
    }
    const double dn = double(n);
    const double den = dn * sxx - sx * sx;
    if (den == 0.0) return 0.0;
    return (dn * sxy - sx * sy) / den;
}

void ClockTracker::tick(double t) {
    if (last_ >= 0.0 && (t - last_) >= kLostAfter) reset();   // clock came back
    last_ = t;
    t_[head_] = t;
    head_ = (head_ + 1) % kWindow;
    if (count_ < kWindow) ++count_;

    if (pendingStart_) { pendingStart_ = false; started_ = true; }

    if (count_ < 12) return;   // half a beat before the first estimate

    // Detect a real tempo change: the last 24 ticks disagree with the long
    // window for a while -> restart the window so we follow quickly.
    const size_t shortN = count_ < 24 ? count_ : 24;
    const double shortP = slopeOverLast(shortN);
    const double longP = slopeOverLast(count_);
    if (shortP <= 0 || longP <= 0) return;
    if (std::fabs(shortP - longP) / longP > 0.03) {
        if (++divergent_ >= 12) {
            // Keep only the recent ticks.
            std::array<double, kWindow> keep{};
            for (size_t i = 0; i < shortN; ++i) keep[i] = t_[(head_ + kWindow - shortN + i) % kWindow];
            t_ = keep;
            count_ = shortN;
            head_ = shortN % kWindow;
            divergent_ = 0;
            snap_ = true;   // follow the new tempo immediately
        }
    } else {
        divergent_ = 0;
    }

    const double period = slopeOverLast(count_);
    if (period <= 0) return;
    double bpm = 60.0 / (24.0 * period);
    if (bpm < kMinBpm || bpm > kMaxBpm) return;
    // Extra smoothing on top of the regression (time constant ~1 beat) so the
    // displayed tempo does not flicker; snapped after a real tempo change.
    bpm_ = (hasTempo_ && !snap_) ? bpm_ + (bpm - bpm_) * 0.04 : bpm;
    snap_ = false;
    hasTempo_ = true;
}

bool TapTempo::tap(double t) {
    if (n_ > 0 && (t - taps_[n_ - 1]) > kResetAfter) n_ = 0;
    if (n_ == taps_.size()) {
        for (size_t i = 1; i < taps_.size(); ++i) taps_[i - 1] = taps_[i];
        --n_;
    }
    taps_[n_++] = t;
    if (n_ < 2) return false;
    const double avg = (taps_[n_ - 1] - taps_[0]) / double(n_ - 1);
    if (avg <= 0) return false;
    double bpm = 60.0 / avg;
    if (bpm < kMinBpm) bpm = kMinBpm;
    if (bpm > kMaxBpm) bpm = kMaxBpm;
    bpm_ = bpm;
    return true;
}

} // namespace fxbox
