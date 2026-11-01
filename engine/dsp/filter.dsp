// FXBox — Filter (full wet; the engine does not mix dry signal in)
// Modes: Moog-style ladder low-pass (ve.moogLadder) and state-variable LP/HP/BP/notch
// (fi.svf). Cutoff modulation: tempo-syncable LFO with four shapes (resettable for
// MIDI Start) plus an envelope follower (auto-wah). Optional drive into the filter.
// Mode and shape changes crossfade in ~20 ms to avoid clicks.
declare name "fxbox_filter";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

// Page 1
cutoff   = hslider("cutoff", 1200, 40, 16000, 0.01) : si.smooth(ba.tau2pole(0.02));
reso     = hslider("reso", 0.3, 0, 1, 0.0001) : si.smoo;
rate     = hslider("rate", 0.5, 0.02, 10, 0.0001) : si.smoo;   // LFO Hz
depth    = hslider("depth", 0, 0, 1, 0.0001) : si.smoo;        // 0..3 octaves
// Page 2
mode     = hslider("mode", 0, 0, 4, 1);                         // 0 Ladder, 1 LP, 2 HP, 3 BP, 4 Notch
shape    = hslider("shape", 0, 0, 3, 1);                        // 0 sine, 1 triangle, 2 square, 3 sample & hold
envamt   = hslider("env", 0, 0, 1, 0.0001) : si.smoo;          // envelope -> up to +4 octaves
drive    = hslider("drive", 0, 0, 1, 0.0001) : si.smoo;        // 0..18 dB into the filter
lforeset = button("lforeset");

// One-hot weights, smoothed: switching modes/shapes crossfades instead of clicking.
w(sel, i) = (sel == i) : si.smooth(ba.tau2pole(0.02));

// LFO
ph = os.hs_phasor(1, rate, lforeset);
wrap = ph < ph';                                               // start of a new cycle
sh = no.noise : ba.sAndH(wrap | (lforeset > 0));
lfo = w(shape, 0) * sin(2 * ma.PI * ph)
    + w(shape, 1) * (1 - 4 * abs(ph - 0.5))
    + w(shape, 2) * ((ph < 0.5) * 2 - 1 : si.smooth(ba.tau2pole(0.003)))
    + w(shape, 3) * (sh : si.smooth(ba.tau2pole(0.003)));

// Envelope follower on the summed input (fast attack, medium release).
env(l, r) = (l + r) * 0.5 : an.amp_follower_ar(0.004, 0.15) : *(4) : min(1);

fc(e) = cutoff * pow(2, depth * 3 * lfo + envamt * 4 * e) : max(20) : min(18000);
Q = 0.707 + reso * 23;             // ladder: below self-oscillation
Qs = 0.5 + reso * 14.5;            // state-variable

g = ba.db2linear(drive * 18);
pre(x) = ma.tanh(x * g) / sqrt(g);  // drive with rough level compensation

chan(e, x) = w(mode, 0) * (pre(x) : ve.moogLadder(fc(e) / (ma.SR / 2), Q) : *(1 + reso * 1.5))
           + w(mode, 1) * (pre(x) : fi.svf.lp(fc(e), Qs))
           + w(mode, 2) * (pre(x) : fi.svf.hp(fc(e), Qs))
           + w(mode, 3) * (pre(x) : fi.svf.bp(fc(e), Qs))
           + w(mode, 4) * (pre(x) : fi.svf.notch(fc(e), Qs));

process(l, r) = chan(e, l), chan(e, r) with { e = env(l, r); };
