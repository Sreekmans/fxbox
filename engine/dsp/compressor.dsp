// FXBox — Compressor at the end of the chain (wet signal only; Mix = parallel compression)
// Stereo-linked feed-forward compressor with soft knee (co.FFcompressor_N_chan,
// compressors.lib) and an optional limiter. Gain reduction is exported as "gr"
// (dB, >= 0) for the meter on screen.
declare name "fxbox_compressor";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

// Page 1
thresh  = hslider("threshold", -18, -40, 0, 0.01) : si.smoo;      // dBFS
ratio   = hslider("ratio", 4, 1, 20, 0.001) : si.smoo;
attack  = hslider("attack", 10, 0.1, 100, 0.001) : si.smoo;       // ms
makeup  = hslider("makeup", 0, 0, 24, 0.01) : si.smoo : ba.db2linear;
// Page 2
release = hslider("release", 150, 20, 1000, 0.01) : si.smoo;      // ms
knee    = hslider("knee", 6, 0, 12, 0.01) : si.smoo;              // dB
limit   = hslider("limiter", 1, 0, 1, 1) : si.smooth(ba.tau2pole(0.02));

strength = 1 - 1 / max(1, ratio);

// Gain reduction meter. The compressor passes its gain in dB (<= 0) through this
// function; it is shown as positive dB. The engine shows the larger of the two channels.
grmeter(g) = g <: _, (*(-1) : max(0) : hbargraph("gr", 0, 60)) : attach;

comp = co.FFcompressor_N_chan(strength, thresh, attack * 0.001, release * 0.001, knee, 0, 1, grmeter, 2);
lim = co.FFcompressor_N_chan(1, -1, 0.0005, 0.06, 1, 0, 1, _, 2);

mixlim(a, b, c, d) = a * (1 - limit) + c * limit, b * (1 - limit) + d * limit;

process = comp : *(makeup), *(makeup) <: (_, _), lim : mixlim;
