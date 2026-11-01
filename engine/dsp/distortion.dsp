// FXBox — Distortion (wet signal only; Mix in the engine allows parallel distortion)
// Runs at twice the sample rate: the engine oversamples this effect 2x
// (half-band filters) to keep aliasing down at high drive.
// Modes crossfade in ~20 ms: 0 Overdrive, 1 Distortion, 2 Fuzz, 3 Bitcrush, 4 Wavefolder.
declare name "fxbox_distortion";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

// Page 1
drive = hslider("drive", 0.5, 0, 1, 0.0001) : si.smoo;        // 0..40 dB gain
tone  = hslider("tone", 0.5, 0, 1, 0.0001) : si.smoo;         // dark .. bright
level = hslider("level", -6, -24, 6, 0.01) : si.smoo : ba.db2linear;
// Page 2
mode  = hslider("mode", 0, 0, 4, 1);
crush = hslider("crush", 0.5, 0, 1, 0.0001) : si.smoo;        // bitcrush: bits and rate reduction
gate  = hslider("gate", 0, 0, 1, 0.0001) : si.smoo;           // noise gate threshold (all modes)

w(i) = (mode == i) : si.smooth(ba.tau2pole(0.02));

gain = ba.db2linear(drive * 40);
bias = 0.2 * drive;

overdrive(x) = ma.tanh(x * gain + bias) - ma.tanh(bias);
hard(x) = (x * gain + bias : max(-0.7) : min(0.7)) - (bias : max(-0.7) : min(0.7));
fuzz(x) = s * (1 - exp(0 - abs(v))) with { v = x * gain * 2 + 0.35 * drive; s = ba.if(v >= 0, 1, -1); };
bits = 16 - crush * 12;                                       // 16 .. 4 bits
q = pow(2, bits - 1);
bitcrush(x) = x * (1 + drive * 4) : ba.downSample(ma.SR / (1 + crush * 31)) : *(q) : floor : /(q) : max(-1) : min(1);
fold(x) = x * (1 + drive * 8) : ef.wavefold(0.3 + 0.7 * drive) : *(0.8);

shaper(x) = w(0) * overdrive(x) + w(1) * hard(x) + w(2) * fuzz(x) + w(3) * bitcrush(x) + w(4) * fold(x);

// Noise gate on the input level (useful with fuzz).
gthr = ba.db2linear(-80 + gate * 50);                          // -80 .. -30 dBFS
gated(x) = x * (x : an.amp_follower_ar(0.001, 0.08) > gthr : si.smooth(ba.tau2pole(0.01)) : max(gate == 0));

toneLp = fi.lowpass(2, 700 * pow(14, tone));                  // 700 Hz .. 9.8 kHz

chan = fi.highpass(1, 120) : gated : shaper : fi.dcblocker : toneLp : *(level);
process = chan, chan;
