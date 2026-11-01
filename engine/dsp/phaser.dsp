// FXBox — Phaser (wet signal only: the all-pass chain)
// Same structure as vibrato2_mono in phaflangers.lib (second-order resonant
// all-pass sections built with fi.tf2), with a resettable LFO so the sweep can
// be aligned to MIDI Start. Mixing with the dry signal (in the engine)
// creates the notches; Mix = 50 % gives the deepest notches.
declare name "fxbox_phaser";
declare license "GPL-3.0-or-later";
import("stdfaust.lib");

rate     = hslider("rate", 0.4, 0.02, 10, 0.0001) : si.smoo;   // Hz
depth    = hslider("depth", 0.7, 0, 1, 0.0001) : si.smoo;      // sweep width
feedback = hslider("feedback", 0.3, 0, 0.9, 0.0001) : si.smoo;
lforeset = button("lforeset");

SECTIONS = 4;
WIDTH    = 1000;  // Hz, notch width
FRATIO   = 1.5;
FRQMIN   = 120;

frqmax = FRQMIN + 40 + depth * 3000;

phase = os.hs_phasor(1, rate, lforeset);
lfo(off) = 0.5 - 0.5 * cos(2 * ma.PI * ma.frac(phase + off));

chain(off) = (+ : seq(i, SECTIONS, ap2p(R, th(i)))) ~ *(feedback)
with {
    ap2p(R, t) = fi.tf2(R^2, -2*R*cos(t), 1, -2*R*cos(t), R^2);
    R     = exp(0.0 - ma.PI * WIDTH / ma.SR);
    thmin = 2 * ma.PI * FRQMIN / ma.SR;
    thmax = 2 * ma.PI * frqmax / ma.SR;
    th1   = thmin + (thmax - thmin) * lfo(off);
    th(i) = (FRATIO^(i+1)) * th1;
};

process = chain(0.0), chain(0.25);
