// FXBox — Chorus (wet signal only)
// Two modulated fractional delay lines (delays.lib) with quadrature LFOs.
// The LFO phase can be reset by the engine (MIDI Start) via "lforeset".
declare name "fxbox_chorus";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

rate     = hslider("rate", 0.8, 0.02, 10, 0.0001) : si.smoo;   // Hz
depth    = hslider("depth", 0.5, 0, 1, 0.0001) : si.smoo;
feedback = hslider("feedback", 0, 0, 0.9, 0.0001) : si.smoo;
lforeset = button("lforeset");

BASE_MS  = 12.0;
DEPTH_MS = 7.0;
MAXD     = 4096;

// Resettable phasor in [0,1).
phase = os.hs_phasor(1, rate, lforeset);
lfo(off) = 0.5 - 0.5 * cos(2 * ma.PI * ma.frac(phase + off));

dsamp(off) = (BASE_MS + DEPTH_MS * depth * lfo(off)) * ma.SR / 1000.0;

voice(off) = (+ : de.fdelay3(MAXD, dsamp(off))) ~ (*(feedback) : fi.lowpass(1, 7000));

process = voice(0.0), voice(0.25);
