// FXBox — Reverb (wet signal only)
// Core: zita_rev1_stereo from reverbs.lib (Faust port of Fons Adriaensen's zita-rev1).
declare name "fxbox_reverb";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

decay    = hslider("decay", 2.5, 0.2, 15, 0.001) : si.smoo : max(0.2); // RT60 (s); max(): smoother starts at 0
tone     = hslider("tone", 0.5, 0, 1, 0.0001) : si.smoo;        // 0 = dark, 1 = bright
predelay = hslider("predelay", 20, 0, 250, 0.01) : si.smoo;    // ms

PDMAX = 16384; // 341 ms at 48 kHz

// Tone controls both the reverb's high-frequency damping crossover and a
// gentle output low-pass, mapped exponentially for even perceptual steps.
f2    = 1500 * pow(8, tone);           // 1.5 kHz .. 12 kHz
lpout = 3000 * pow(6, tone);           // 3 kHz .. 18 kHz

pre = de.delay(PDMAX, predelay * ma.SR / 1000.0);

// zita_rev1_stereo(rdel, f1, f2, t60dc, t60m, fsmax). rdel=0: predelay is ours.
process = pre, pre
        : re.zita_rev1_stereo(0, 200, f2, decay * 1.15, decay, 48000)
        : fi.lowpass(1, lpout), fi.lowpass(1, lpout);
