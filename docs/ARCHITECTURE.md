# Architecture and technical choices

**English** · [Nederlands](ARCHITECTURE.nl.md)

## Overview

```
instrument ─► ADC ─► JACK ─► fxbox-engine ─► JACK ─► DAC ─► output L/R
                                  │ (C++, realtime)
DIN-MIDI (UART) ─────────────────►│ MIDI clock, tap tempo, sync
USB-MIDI (ALSA seq) ─────────────►│
                                  │ UNIX socket (text lines)
                                  ▼
                           fxbox-ui (Python)
MCP23017 ──► encoders/buttons ─┐
                               ├─► controller ─► cairo ─► /dev/fb1 (ZynScreen)
presets/state (JSON) ──────────┘
```

Three systemd services, each with its own responsibility:

| Service | Task | Priority / cores |
|---|---|---|
| `fxbox-jack` | JACK2 server on the ZynADAC/HiFiBerry | realtime prio 70, cores 2–3 |
| `fxbox-engine` | effects, meters, MIDI clock, tempo | JACK thread realtime; cores 2–3 |
| `fxbox-ui` | screen, encoders, buttons, presets | normal (`nice 5`), cores 0–1 |

If the interface fails or restarts, the audio keeps running. If the engine restarts, the interface automatically restores the full state.

## Operating system

| Option | Assessment |
|---|---|
| **Raspberry Pi OS Lite 64-bit, Bookworm** ✔ | The same base Zynthian builds its V4 support on (`setup_system_raspioslite_64bit_bookworm.sh`). The ZynScreen overlays, the `hifiberry-dacplusadcpro` overlay and `midi-uart0` are proven on it. No desktop. |
| Raspberry Pi OS Trixie | Zynthian's Trixie script is still in development; installation shows a warning but will probably work. |
| Zynthian image | Excluded by the brief. |
| Buildroot / Yocto | Smaller, but much more work and less maintainable for a single device. |

## Audio approach

| Option | Pros | Cons |
|---|---|---|
| **Own C++ engine on JACK, DSP from the Faust libraries** ✔ | Full control over transitions, bypass, tails, metering and clock in one realtime path; testable offline; the Faust libraries (GRAME; reverb = zita-rev1 by Fons Adriaensen) are proven and maintained; no runtime dependency on Faust (the generated C++ is in the repo). | New effects need a Faust file and a recompile. |
| mod-host + LV2 plugins | Large choice of plugins. | Bypass, crossfades, tails and meters have to be built around external plugins; tempo sync and parameter smoothing differ per plugin; more moving parts at boot. |
| Pure Data / SuperCollider | Fast prototyping. | Harder to make robust as an appliance; heavier runtime. |
| ALSA directly, without JACK | Slightly fewer layers. | JACK is proven on this hardware in Zynthian, and provides xrun reporting and `jack_iodelay` for latency measurement. |

**Licences.** Faust libraries: LGPL with an exception allowing generated code to be distributed under its own licence. The ZynScreen overlays come from zynthian-sys (GPL-3.0). FXBox itself: GPL-3.0-or-later.

**Effect chain.** Eight effects in a fixed order (`kChainOrder` in `effects.hpp`): Filter → Saturation → Distortion → Chorus → Phaser → Tape Echo → Reverb → Compressor. Per slot: input gain (into the effect), dry pass-through and wet output, each smoothed per sample. A slot that is off and silent is skipped: the signal passes through unchanged and costs no processing time.

**Parameters and pages.** Each effect has 8 parameter slots (indices 0–7 in the protocol, presets and engine); 0–3 form page 1, 4–7 page 2. Unused slots have type `None` and are skipped. Choice parameters (filter type, LFO shape, distortion type, limiter) are ordinary Faust sliders with step 1; in the Faust code they are turned into smoothed one-of-n weights, so a switch crossfades in ~20 ms instead of clicking. Drawback: all variants are always computed.

**Oversampling (Distortion).** The Distortion runs at 2× the sample rate. Up- and downsampling use a linear-phase 47-tap half-band FIR (Kaiser window, β = 6; `oversampler.hpp`), skipping the zero taps. The total delay is exactly 23 samples at the normal sample rate. The dry path of that slot gets the same delay, so parallel mixing causes no comb filtering. That delay is faded in and out over 10 ms when switching on and off, so without a click, and adds no latency while the Distortion is off. Measured aliasing reduction: 6 to 12 dB depending on the type (see the test report). At extreme drive, aliasing remains audible; 4× would help more but costs twice the processing time.

**Compressor.** Feed-forward, stereo-linked, with soft knee (`co.FFcompressor_N_chan` from the Faust library), followed by a fixed fast limiter (threshold −1 dB, attack 0.5 ms) that is faded in and out with *Limiter*. The gain reduction is exposed as Faust bargraph `gr`; the engine sends the maximum of both channels as the 11th field of the `m` line. The interface also reads an older `m` line without that field.

**Extensibility.** An effect consists of a `.dsp` file (`engine/dsp/`), a line in `engine/src/effects.hpp` (safe limits, sync type, position in the chain) and an `EffectDef` in `ui/fxbox/model.py`. An LV2 host (lilv) can later be added as an extra slot type.

## Realtime rules in the engine

- The JACK callback only calls `EngineCore::process`: no memory allocation, no locks, no system calls, no file access.
- Control data arrives through a lock-free single-producer/single-consumer queue (`spsc.hpp`); meters go back through atomics.
- All buffers are allocated at start-up. Effects are "warmed up" at start-up so their parameter smoothers sit at the right value.
- Denormals are flushed to zero on the audio thread (ARM64 FPCR.FZ; x86 MXCSR).
- Every block is checked for NaN/Inf; an effect that goes off the rails is cleared without stopping the engine (message on screen).

## Behaviour (specified)

| Situation | Behaviour |
|---|---|
| Parameter change | All Faust parameters glide (`si.smoo`); dry/wet and gains follow per sample with a ~10 ms time constant. |
| Delay time changes | Glides in ~250 ms, like a tape transport: a short pitch bend, no click. |
| Feedback | Limited in the engine (Tape 95 %, Chorus/Phaser 90 %), regardless of what the interface sends; tape saturation inside the Tape Echo loop. Filter resonance stays below self-oscillation. |
| Distortion | 2× oversampled (see above). Type switch crossfades in ~20 ms. |
| Filter type / LFO shape | Switch crossfades in ~20 ms; LFO reset on MIDI Start applies to all shapes. |
| Compressor | Always last in the chain; Mix < 100 % = parallel compression. Limiter on by default. |
| Effect on | Input to the effect and wet output fade in over 10 ms; the dry pass-through goes to cos(mix·π/2). |
| Effect off | Input to the effect fades out over 10 ms, the dry pass-through goes to 1. With "Tails on bypass" on, the tail (echo, reverb) rings out; off = wet output gone within 10 ms. |
| Bypass (whole chain) | Buffered bypass (no hardware true bypass): all effects behave as "effect off", so dry signal at unity gain as long as the software is running, with or without tails. |
| Preset change | Effects that are on in both presets: the tail keeps ringing (spill-over), parameters glide to the new values. Other effects switch on or off as above. |
| Mix | Equal-power per effect: dry = cos(mix·π/2), wet = sin(mix·π/2). The filter has no Mix (always fully wet). |
| Input Automatic | If IN R is quieter than −80 dBFS for 0.25 s while IN L carries signal (> −60 dBFS), L is sent to both channels (10 ms crossfade). As soon as IN R gets signal, back to stereo. |

## MIDI clock and tempo

- 24 clock pulses per quarter note. Tempo = least-squares estimate over the last 96 pulses (4 beats), plus light smoothing. A real tempo change (>3 % deviation for half a beat) restarts the window so it is followed within a few beats.
- Tempo-dependent parameters are only updated on a change of more than 0.2 % (dead band), so jitter does not make the delay time wobble.
- **Clock lost:** after 0.5 s without a pulse the clock counts as lost; the last received tempo is kept (no jump), and the screen shows that no clock is coming in.
- **Start (FA):** the first pulse after Start sets the LFOs of Filter, Chorus and Phaser to phase 0 (with sync on). **Continue (FB):** no phase reset. **Stop (FC):** effects keep running, tempo stays where it is.
- **Sources:** DIN MIDI (UART) and all USB MIDI devices (ALSA sequencer, including when plugged in later). Choice in Settings: Automatic (DIN has priority, then the first USB source with clock), DIN, or one USB device.
- **Tap tempo:** average of the last up to 4 intervals; after a 2 s pause a new series starts. Without sync, tapping also sets the Tape Echo Time to one quarter note. With sync and an incoming clock, the clock has priority.
- **Sync on** shows the delay time as 1/4, 1/8, 1/8D, 1/4T …; without sync in ms. Modulation rates use divisions per LFO cycle (1/16 … 8/1).

## Metering

- **VU (needle):** average rectification with two coupled poles (~300 ms to 99 %), calibrated so a sine shows its RMS value. The scale is **relative**: 0 VU = −18 dBFS (configurable: −12/−14/−18/−20). The reference is shown on screen.
- **PEAK:** digital sample peak in **dBFS**, held for 1.5 s. This is a digital value relative to the converter's full scale, not an analogue level in dBu.
- **CLIP:** output ≥ 0 dBFS (before limiting); **IN CLIP:** input at the ADC's full scale. Both visible for 1.5 s.
- No dBu/dBV values are shown: they depend on the analogue calibration of the card and have not been measured.

## Interface and graphics

| Option | Assessment |
|---|---|
| **Python + pycairo, straight to the fbtft framebuffer** ✔ | Anti-aliased needles and text, small and readable, no X/Wayland needed. Only changed rows are written; fbtft only refreshes the memory pages that were touched, so a static screen costs no SPI time. |
| SDL2/pygame on the framebuffer | SDL2 no longer has an fbdev backend; the screen is not a KMS device. |
| Qt / LVGL | Powerful, but much heavier to build and maintain for a single screen. |

The framebuffer is located by driver name (`ili9486`), because its number (`fb0`/`fb1`) depends on HDMI.

**Encoders:** full-step state machine (Ben Buxton's table) that ignores bounce and invalid transitions; rotation of an encoder is ignored while it is pressed and for 80 ms after release. **Buttons:** the level must be stable for 12 ms. **Reading:** on the MCP23017 interrupt lines (libgpiod), with 20 ms polling as a fallback.

## Storage

- Factory presets: `/opt/fxbox/presets/factory.json` (read-only).
- User presets: `/var/lib/fxbox/user_presets.json` (4 banks × 8 slots).
- Last state: `/var/lib/fxbox/state.json`, written 2 s after the controls go idle (at the latest after 15 s), via temporary file + `fsync` + `rename` (atomic). Corrupt files are set aside (`.bad`) and the device starts with default values.
- Journald logs in RAM only (fewer writes to the SD card).
