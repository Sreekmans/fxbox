// FXBox — Saturation (wet signal only)
// Gentle tape/tube-style saturation: biased tanh normalised to unity small-signal
// gain, so turning Drive up adds harmonics and compression rather than volume.
declare name "fxbox_saturation";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

drive  = hslider("drive", 0.4, 0, 1, 0.0001) : si.smoo;       // 0..18 dB into the curve
warmth = hslider("warmth", 0.3, 0, 1, 0.0001) : si.smoo;      // bias (even harmonics) + top roll-off
level  = hslider("level", 0, -12, 6, 0.01) : si.smoo : ba.db2linear;

g = ba.db2linear(drive * 18);
b = warmth * 0.6;
tb = ma.tanh(b);
slope = g * (1 - tb * tb);                                    // small-signal gain of the curve
sat(x) = (ma.tanh(x * g + b) - tb) / slope;
air = fi.lowpass(1, 20000 - warmth * 12000);                  // 20 kHz .. 8 kHz

chan = sat : fi.dcblocker : air : *(level);
process = chan, chan;
