// 2x oversampling with a 47-tap linear-phase half-band FIR (Kaiser window, beta 6).
// Up: zero-stuff + filter; down: filter + decimate. Total latency: exactly
// kLatency base-rate samples, which the engine uses to align the dry path.
// Realtime-safe after prepare(): no allocation in process.
#pragma once
#include <cmath>
#include <cstring>
#include <vector>

namespace fxbox {

class HalfBand2x {
public:
    static constexpr int kTaps = 47;
    static constexpr int kCenter = (kTaps - 1) / 2;   // 23
    static constexpr int kLatency = kCenter;          // base-rate samples (23 up + 23 down at 2x)

    HalfBand2x() {
        // Windowed sinc, cutoff fs/4 at the high rate -> half-band (every other tap zero).
        auto bessel0 = [](double x) {
            double s = 1, t = 1;
            for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; }
            return s;
        };
        const double beta = 6.0;
        double sum = 0;
        double h[kTaps];
        for (int i = 0; i < kTaps; ++i) {
            const int m = i - kCenter;
            const double sinc = m == 0 ? 0.5 : std::sin(M_PI * 0.5 * m) / (M_PI * m);
            const double r = double(m) / kCenter;
            const double w = bessel0(beta * std::sqrt(1 - r * r)) / bessel0(beta);
            h[i] = sinc * w;
            sum += h[i];
        }
        for (int i = 0; i < kTaps; ++i) h_[i] = float(h[i] / sum);   // unity DC gain
        for (int i = 0; i < kHalf; ++i) even_[i] = h_[2 * i];
    }

    void prepare(int maxBlock) {
        upExt_.assign(kHalf - 1 + maxBlock, 0.f);
        downExt_.assign(kTaps - 1 + 2 * maxBlock, 0.f);
    }
    void reset() {
        std::fill(upExt_.begin(), upExt_.end(), 0.f);
        std::fill(downExt_.begin(), downExt_.end(), 0.f);
    }

    // n base-rate samples in -> 2n high-rate samples out.
    void up(const float* x, float* y, int n) {
        constexpr int H = kHalf - 1;   // 23 samples of history
        float* e = upExt_.data();
        std::memcpy(e + H, x, sizeof(float) * n);
        for (int k = 0; k < n; ++k) {
            const float* p = e + H + k;
            float acc = 0.f;
            for (int i = 0; i < kHalf; ++i) acc += even_[i] * p[-i];
            y[2 * k] = 2.f * acc;
            y[2 * k + 1] = 2.f * h_[kCenter] * p[-(kCenter - 1) / 2];   // only odd tap: the centre
        }
        std::memmove(e, e + n, sizeof(float) * H);
    }

    // 2n high-rate samples in -> n base-rate samples out.
    void down(const float* u, float* z, int n) {
        constexpr int H = kTaps - 1;   // 46 samples of history
        float* e = downExt_.data();
        std::memcpy(e + H, u, sizeof(float) * 2 * n);
        for (int k = 0; k < n; ++k) {
            const float* p = e + H + 2 * k;
            float acc = h_[kCenter] * p[-kCenter];
            for (int i = 0; i < kHalf; ++i) acc += even_[i] * p[-2 * i];
            z[k] = acc;
        }
        std::memmove(e, e + 2 * n, sizeof(float) * H);
    }

private:
    static constexpr int kHalf = (kTaps + 1) / 2;   // 24 even taps
    float h_[kTaps];
    float even_[kHalf];
    std::vector<float> upExt_, downExt_;
};

// Fixed delay of HalfBand2x::kLatency samples (keeps the dry path aligned).
class FixedDelay {
public:
    static constexpr int kLen = HalfBand2x::kLatency;
    void prepare(int maxBlock) { ext_.assign(kLen + maxBlock, 0.f); }
    // Writes the delayed version of x into y (may be called every block, even when unused).
    void process(const float* x, float* y, int n) {
        float* e = ext_.data();
        std::memcpy(e + kLen, x, sizeof(float) * n);
        std::memcpy(y, e, sizeof(float) * n);
        std::memmove(e, e + n, sizeof(float) * kLen);
    }
private:
    std::vector<float> ext_;
};

} // namespace fxbox
