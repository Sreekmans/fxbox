// Tests for MIDI clock tracking and tap tempo with synthetic, jittery input.
#include <cmath>
#include <cstdio>
#include <random>

#include "tempo.hpp"

using namespace fxbox;

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
                              else { std::printf("  ok:   "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

// Feeds ticks for `beats` quarter notes at `bpm`, with gaussian jitter (seconds).
static double feed(ClockTracker& tr, double t, double bpm, double beats, double jitter, std::mt19937& rng,
                   double* maxDev = nullptr, double refBpm = 0, TempoDeadband* db = nullptr, int* dbUpdates = nullptr) {
    std::normal_distribution<double> n(0.0, jitter);
    const double period = 60.0 / bpm / 24.0;
    const int ticks = int(beats * 24);
    for (int i = 0; i < ticks; ++i) {
        t += period;
        tr.tick(t + n(rng));
        if (maxDev && tr.hasTempo()) *maxDev = std::max(*maxDev, std::fabs(tr.bpm() - refBpm));
        if (db && tr.hasTempo() && db->update(tr.bpm()) && dbUpdates) ++*dbUpdates;
    }
    return t;
}

int main() {
    std::mt19937 rng(42);

    std::printf("[clock] steady 174 BPM, 1.5 ms jitter (USB-like)\n");
    {
        ClockTracker tr;
        double t = feed(tr, 0.0, 174.0, 2.0, 0.0015, rng);
        CHECK(tr.hasTempo() && std::fabs(tr.bpm() - 174.0) < 1.0, "after 2 beats: %.2f BPM", tr.bpm());
        double dev = 0;
        TempoDeadband db; int updates = 0;
        db.force(tr.bpm());
        feed(tr, t, 174.0, 64.0, 0.0015, rng, &dev, 174.0, &db, &updates);
        CHECK(dev < 0.35, "over 64 beats max deviation %.3f BPM", dev);
        CHECK(updates <= 4, "applied tempo changed %d times in 64 beats (deadband)", updates);
    }

    std::printf("[clock] 3 ms jitter (worst-case UART buffering)\n");
    {
        ClockTracker tr;
        double t = feed(tr, 0.0, 120.0, 4.0, 0.003, rng);
        double dev = 0;
        feed(tr, t, 120.0, 32.0, 0.003, rng, &dev, 120.0);
        CHECK(dev < 0.6, "max deviation %.3f BPM", dev);
    }

    std::printf("[clock] tempo change 120 -> 140 BPM\n");
    {
        ClockTracker tr;
        double t = feed(tr, 0.0, 120.0, 16.0, 0.001, rng);
        t = feed(tr, t, 140.0, 3.0, 0.001, rng);
        CHECK(std::fabs(tr.bpm() - 140.0) < 1.5, "3 beats after change: %.2f BPM", tr.bpm());
        t = feed(tr, t, 140.0, 4.0, 0.001, rng);
        CHECK(std::fabs(tr.bpm() - 140.0) < 0.3, "7 beats after change: %.2f BPM", tr.bpm());
    }

    std::printf("[clock] loss and recovery\n");
    {
        ClockTracker tr;
        double t = feed(tr, 0.0, 100.0, 8.0, 0.0, rng);
        CHECK(tr.receiving(t + 0.1), "receiving shortly after last tick");
        CHECK(!tr.receiving(t + 0.6), "lost after 0.5 s without ticks");
        const double held = tr.bpm();
        CHECK(std::fabs(held - 100.0) < 0.2, "tempo held at %.2f BPM", held);
        t = feed(tr, t + 5.0, 90.0, 2.0, 0.0, rng);
        CHECK(std::fabs(tr.bpm() - 90.0) < 0.5, "new clock picked up quickly: %.2f BPM", tr.bpm());
    }

    std::printf("[clock] start message marks the next tick\n");
    {
        ClockTracker tr;
        double t = feed(tr, 0.0, 120.0, 2.0, 0.0, rng);
        tr.start();
        CHECK(!tr.takeStartedFlag(), "no flag before next tick");
        tr.tick(t + 0.02);
        CHECK(tr.takeStartedFlag(), "flag after first tick following Start");
        CHECK(!tr.takeStartedFlag(), "flag consumed once");
    }

    std::printf("[tap] tap tempo\n");
    {
        TapTempo tap;
        CHECK(!tap.tap(10.0), "first tap gives no tempo yet");
        bool ok = tap.tap(10.5);
        CHECK(ok && std::fabs(tap.bpm() - 120.0) < 0.01, "two taps 0.5 s apart: %.2f BPM", tap.bpm());
        tap.tap(11.0); tap.tap(11.52);
        CHECK(std::fabs(tap.bpm() - 118.6) < 1.0, "averaged over taps: %.2f BPM", tap.bpm());
        tap.tap(20.0);
        tap.tap(20.4);
        CHECK(std::fabs(tap.bpm() - 150.0) < 0.01, "pause > 2 s starts a new series: %.2f BPM", tap.bpm());
    }

    std::printf("[divisions]\n");
    CHECK(std::fabs(divisionToMs(0.75, 120.0) - 375.0) < 1e-9, "1/8D at 120 BPM = 375 ms");
    CHECK(std::fabs(divisionToMs(1.0 / 3.0, 120.0) - 166.6667) < 1e-3, "1/8T at 120 BPM = 166.67 ms");
    CHECK(std::fabs(divisionToHz(4.0, 120.0) - 0.5) < 1e-9, "1 bar per cycle at 120 BPM = 0.5 Hz");

    std::printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
