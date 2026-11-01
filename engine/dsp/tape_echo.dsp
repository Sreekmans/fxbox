// FXBox — Tape Echo (wet signal only; dry/wet mix happens in the engine)
// Built from Faust standard library blocks (delays.lib, filters.lib, oscillators.lib).
declare name "fxbox_tape_echo";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

// Controls (physical units). The engine clamps values before writing them.
time_ms  = hslider("time", 375, 10, 2000, 0.01);
feedback = hslider("feedback", 0.35, 0, 0.95, 0.0001) : si.smoo;
wow      = hslider("wow", 0.15, 0, 1, 0.0001) : si.smoo;

MAXD = 131072; // 2.73 s at 48 kHz — headroom for 2000 ms + wow modulation

// Delay time glides like a tape transport (~250 ms), so tempo and preset
// changes produce a short pitch slide instead of a click.
dsamp = time_ms * ma.SR / 1000.0 : si.smooth(ba.tau2pole(0.25));

// Wow (slow) + flutter (fast) + slow drift. Depth in samples scales with the
// wow control; maximum about 3 ms of wow and 0.3 ms of flutter.
wowmod(ph) = wow * ma.SR * (0.0030 * os.oscp(0.55, ph)
                          + 0.0003 * os.oscp(6.3, ph * 1.7)
                          + 0.0008 * (no.lfnoise(0.3) : si.smoo));

// Feedback loop: tape-like band limiting and soft saturation keep repeats
// darker and bounded. tanh(x*k)/k has unity small-signal gain.
sat(x) = ma.tanh(x * 1.2) / 1.2;
tapeTone = fi.highpass(1, 90) : fi.lowpass(2, 3600);
loop = tapeTone : sat : *(feedback);

echo(ph) = (+ : de.fdelay3(MAXD, max(2, dsamp + wowmod(ph)))) ~ loop;

// Channel 0/1 use different modulation phases for a subtle stereo image.
process = echo(0), echo(1.9);
