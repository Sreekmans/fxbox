# Test report

**English** · [Nederlands](TEST_REPORT.nl.md)

Status: 01-11-2026, version 1.2.0 (filter types and page 2, distortion types with 2× oversampling, compressor/limiter at the end of the chain).

**On the real hardware, installation, hardware detection, screen, controls and a first DSP measurement have now been done (see the bottom of this report); the remaining items are still open.** All tests below ran on a development machine (x86-64, Ubuntu 24.04) and, for ARM64, under emulation (qemu). Beyond the measurements listed at the bottom, no hardware tests have been done, and working audio, measured latency and boot time on the device have not been established yet. The install script also runs the engine and interface tests on the Pi itself; their output goes to `install/test_dsp_on_pi.txt`.

## Tests performed

### Audio engine, offline (`engine/tests/test_dsp.cpp`)

Run on x86-64 and, compiled for Cortex-A72 (`-mcpu=cortex-a72`), under qemu-aarch64. All passed.

| Test | Result |
|---|---|
| Stability: maximum feedback, loud noise, 20 s per effect | no NaN or DSP errors; the tail does not grow after the input stops |
| Echo time 100 / 375 / 1000 ms | first repeat at 100.44 / 374.79 / 999.58 ms |
| Bypass with tails off | output = input (max. deviation 0) |
| Click-free: bypass, effect switch to Reverb and to Phaser, mix 0→1, input +12 dB | largest sample step stays within 2× that of the clean sine (level-adjusted) |
| Mono input | L and R output identical |
| VU calibration (sine −18 dBFS RMS) | −18.01 dBFS; sample peak −14.99 dBFS |
| VU rise time | 98.9 % after 300 ms |
| Clip detection | detected |

On its first run this test found a real bug: the Reverb produced NaN right after start-up, because the smoothing of the decay parameter started from 0 (division by zero). This has been fixed (`max(0.2)` after the smoothing) and the stability test now covers it.

Processing time for 30 s of audio, **for information only, not a Pi measurement**:

- x86: 78× to 272× faster than realtime.
- ARM64 under emulation: 3.9× to 16× faster than realtime. Emulation is much slower than a real Pi 4, so this figure says nothing about the load on the device either.

### Version 1.1: chain and new effects (`engine/tests/test_dsp.cpp`)

Passed on x86-64 and under ARM64 emulation:

| Test | Result |
|---|---|
| Stability Distortion, Saturation, Filter (maximum settings, loud noise, 20 s) | no NaN or DSP errors, tail does not grow |
| Whole chain (all 7 effects on), 20 s loud noise | no NaN or DSP errors, tail decays |
| Filter (cutoff 300 Hz) | 100 Hz: −1.8 dB; 5 kHz: −99 dB |
| Distortion (full drive) | 3rd harmonic −9.4 dB relative to the fundamental |
| Saturation | small signals −0.1 dB (almost unity gain); loud signals get harmonics |
| Click-free: effect on/off (Chorus off + Reverb on, Phaser on, Filter on) | within the limit of the clean sine |
| Automatic mono input | silent IN R → R output = L output; signal on IN R → stereo again |

Processing time of the whole chain (all 7 effects on), for information: 29× realtime on x86, 1.4× realtime under ARM64 emulation. Emulation is much slower than a real Pi 4; **the DSP load on the device has not been measured yet** (see *Instellingen → Info*).

Interface: 22 unit tests, including the chain screen, K2 short/long, saving and reloading a preset with several effects, and reading a state from version 1.0.

### Version 1.2: filter types, distortion types, compressor (`engine/tests/test_dsp.cpp`)

Passed on x86-64 and, compiled for Cortex-A72, under ARM64 emulation (qemu).

| Test | Result |
|---|---|
| Filter types at cutoff 1 kHz, reso 0 (100 Hz / 1 kHz / 10 kHz) | Ladder −0.4 / −12.2 / −85.8 dB · LP −0.3 / −6.2 / −43.0 dB · HP −40.3 / −6.2 / −0.3 dB · BP −20.3 / −6.2 / −21.6 dB · Notch −0.4 / −44.8 / −0.3 dB |
| Filter: all four LFO shapes with envelope and drive at maximum, loud noise | no NaN or DSP errors |
| Filter: type switch LP → HP during a sine | largest sample step equal to that of the sine itself (no click) |
| Distortion: all five types, loud noise, 5 s | no NaN or DSP errors, audible signal |
| Distortion: aliasing, 7 kHz sine, drive 50 % (all non-harmonic components summed, relative to the harmonics) | Overdrive −32.1 dB (without oversampling −20.1) · Distortion −25.6 (−15.5) · Fuzz −27.3 (−17.6) · Folder −22.8 (−16.1). Bitcrush deliberately not measured: it aliases on purpose. |
| Distortion at 50 % mix: frequency response 0.5–8 kHz | 2.5 dB spread (no comb filtering: the dry path is delayed by 23 samples, matching the oversampler) |
| Distortion on/off during a sine | largest step 0.025 vs 0.017 for the sine itself (within the 2× limit) |
| Compressor: sine −6 dBFS peak, threshold −20 dB, ratio 4, hard knee | output −16.1 dBFS peak (theory −16.5); reported gain reduction: 10.0 dB |
| Limiter: +24 dB make-up on a loud signal | peak −0.76 dBFS, no clipping |
| Compressor off | gain reduction 0 reported |
| Whole chain, all 8 effects on, loud noise 20 s | no NaN or DSP errors, tail decays |

Processing time, for information and **not a Pi measurement**:

- x86: whole chain 18× realtime; Distortion 48× (with oversampling the heaviest effect), the other effects 100× to 265×.
- ARM64 under emulation: whole chain 0.9× realtime; Distortion 2.6×, Reverb 4.1×, the others 8.6× to 16×. Emulation is much slower than a real Pi 4.

Because of the oversampling, the Distortion is about twice as heavy as in version 1.1. How heavy on the device itself still has to be measured (see below, item 15b).

Interface: 30 unit tests (8 new), including:

- holding K4 switches the page and the encoders follow;
- choice parameters move one step per click, even when turning fast;
- an empty slot on page 2 does nothing;
- a state from version 1.1 (4 values per effect) gets default values for page 2;
- page 2 values are kept in a user preset;
- the gain reduction is read from the engine protocol, also from an older engine without that field.

### Tempo and MIDI clock (`engine/tests/test_tempo.cpp`, `engine/tests/din_clock_sim.py`)

| Test | Result |
|---|---|
| 174 BPM with 1.5 ms jitter | within 1 BPM after 2 beats; then max. 0.15 BPM deviation over 64 beats; 0 tempo updates to the DSP thanks to the dead band |
| 120 BPM with 3 ms jitter | max. 0.12 BPM deviation |
| Tempo change 120 → 140 | 139.4 after 3 beats; 140.0 after 7 beats |
| Clock lost / back | lost after 0.5 s, tempo held, new clock picked up quickly |
| Start message | the first pulse after it is flagged (LFO reset) |
| Tap tempo, divisions | correct (e.g. 1/8D at 120 BPM = 375 ms) |
| **Integration:** simulated DIN clock via a pseudo-terminal into the real engine | 'din' detected, 173.9–174.1 BPM shown, loss reported, tempo held |

### Interface (`ui/tests/test_ui.py`: 19 tests)

- **Encoders and buttons:** encoder decoder (clockwise, counter-clockwise, contact bounce, half turn, reverse) and button debouncing.
- **Pin mapping:** MCP pin mapping with the V4 defaults.
- **Controls:**
  - turning a parameter;
  - turning is ignored during and just after a press;
  - bypass and tap react on press;
  - effect selection;
  - long press to save;
  - navigating presets and banks (with wrap-around);
  - sync divisions;
  - tap without sync.
- **Storage:** delayed and atomic saving (no writes while turning) and recovery after a restart; a corrupt state file is set aside.
- **Engine link:** the full state is resent on (re)connecting, error messages appear on screen and the protocol parser is tested.

### Integration interface + engine (`ui/tests/integration_engine.py`)

The real `fxbox.main` ran against the real `fxbox-engine` on a dummy JACK server, with an in-memory framebuffer.

| Checked | Result |
|---|---|
| Frames drawn | 166 in about 5.5 s (30 fps) |
| Screen rows written | 613 rows in total (changed rows only) |
| Retained | sync, bank change and the edited marker |
| Shutdown | clean on SIGTERM |

### Other

- Desktop demo starts and draws headless (SDL dummy).
- Engine protocol against dummy JACK: connected to the JACK ports and status lines received.
- `boot-config.sh`: idempotent (one block after running twice), serial console removed, restore returns exactly the original.
- `shellcheck` on all scripts: no warnings.
- `systemd-analyze verify` on all services: no syntax errors.
- `build-image.sh` tested with a mock Raspberry Pi OS image (two partitions, `--skip-install`):
  - growing partition 2, plus `resize2fs`;
  - mounting via offsets, so without udev;
  - check for the Bookworm base;
  - `userconf.txt`, the `ssh` file and the hostname;
  - `resolv.conf` restored and `policy-rc.d` removed;
  - output `.img`, `.xz` and `.sha256`;
  - no loop devices left behind.
- **Not yet tested** for `build-image.sh`: downloading the real Raspberry Pi OS image and running `install.sh` in the ARM64 chroot. Those downloads are not reachable from the development environment. In the test, the FAT boot partition was replaced by ext4, because the test environment cannot mount vfat.
- Engine: 0 compiler warnings. `main.cpp` and `midi_input.cpp` also check cleanly for aarch64 (`-fsyntax-only`); the full link happens on the Pi.
- Overlay: `piscreen2r-notouch.dts` compiled is identical to Zynthian's shipped `.dtbo`.
- Python code requires at least 3.11 (Bookworm ships 3.11), checked with vermin.

Versions used: Faust 2.70.3 (the generated C++ is in the repo; the Pi does not need Faust), pycairo 1.29.2 and pygame 2.6.1 (desktop). The Debian package versions on the Pi are recorded during installation in `install/installed-package-versions.txt`.

## Still to check on the device

| # | Check | How |
|---|---|---|
| 1 | Installation on clean Raspberry Pi OS Lite 64-bit Bookworm; engine and interface tests pass on the Pi | `install.sh` output, `install/test_dsp_on_pi.txt` |
| 2 | Hardware present (MCP23017, PCM1863, PCM5242, card, framebuffer, UART) | `sudo fxbox-hwtest check` |
| 3 | Screen: orientation, colours (BGR), border all round, readability | `sudo fxbox-hwtest screen` |
| 4 | Order of S1–S4, position and direction of K1–K4 | `sudo fxbox-hwtest controls --write` |
| 5 | Encoders: no missed or double steps, no parameter change when pressed | by hand |
| 6 | Boot without desktop or terminal on the ZynScreen; boot time to usable screen | stopwatch; `systemd-analyze` |
| 7 | Audio in → effect → out; levels; mono and stereo input; ADC input selection matches the jacks | instrument + headphones/monitor |
| 8 | Latency | `sudo ./install/measure-latency.sh` with loopback cable |
| 9 | No xruns at `PERIOD=128` for 1 hour with Reverb (heaviest effect); otherwise 256 | *Instellingen → Info* |
| 10 | DSP load and CPU temperature | *Info*; `vcgencmd measure_temp` |
| 11 | Meters follow the signal; CLIP and IN CLIP with an overdriven signal | test tone |
| 12 | DIN MIDI clock from a real device (timing via the midi-uart0 overlay) | drum machine/sequencer via DIN |
| 13 | USB MIDI clock and hot-plugging during use; source selection with two devices | two clock sources |
| 14 | Start/Stop/Continue behaviour | sequencer |
| 15 | Bypass, effect on/off, preset change click-free in practice | listening |
| 15b | DSP load with the whole chain on (especially Reverb + Tape Echo + Distortion, now 2× oversampled) | *Instellingen → Info* |
| 15c | Filter types, distortion types and compressor sound as intended; GR display in the chain follows the signal | listening with an instrument |
| 16 | Power removed right after a change and well after it: state restored, no corrupt files | pull the plug |
| 17 | Error messages: sound card disconnected (device off!), engine stopped | `sudo systemctl stop fxbox-engine` |
| 18 | Automatic restart after a crash | `sudo kill -SEGV $(pidof fxbox-engine)` |

## Measurements on the device

| Measurement | Value | Date |
|---|---|---|
| Round-trip latency (128 × 2 @ 48 kHz) | *not yet measured* | |
| Boot time to effects interface | *not yet measured* | |
| DSP load (JACK, 48 kHz, 128 × 2) with Filter, Saturation, Distortion and Phaser on | 7 % (read on the device, *Instellingen → Info*) | 01-11-2026 |
| DSP load with Reverb + Tape Echo on | *not yet measured* | |
| DSP load with the whole chain (8 effects, version 1.2) | *not yet measured* | |
| Xruns (with Filter, Saturation, Distortion and Phaser on) | 0 | 01-11-2026 |
| Xruns in 1 hour | *not yet measured* | |
