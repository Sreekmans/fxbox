# Hardware configuration and sources

**English** · [Nederlands](HARDWARE.nl.md)

All values come from Zynthian's own configuration for the V4 kit. Nothing is guessed; anything that cannot be determined from the sources is listed at the bottom under "To confirm on the device". Hardware settings are kept separate from the code, in `/etc/fxbox/hardware.toml` and `/etc/fxbox/audio.conf`.

Sources (consulted 31-10-2026):

- **zynthian-sys** (branch `vangelis`, commit `c2eea576`): `config/zynthian_envars_V4.sh`, `sbin/zynthian_autoconfig.py`, `boot/config.txt`, `custom/display/ZynScreen 3.5 (v1)/boot/overlays/*.dts`, `sbin/fix_alsamixer_settings.sh`, `etc/systemd/mod-ttymidi.service` — <https://github.com/zynthian/zynthian-sys>
- **zynthian-webconf**: `lib/kit_config_handler.py`, `lib/wiring_config_handler.py`, `lib/display_config_handler.py`, `lib/audio_config_handler.py` — <https://github.com/zynthian/zynthian-webconf>
- **zyncoder**: `zyncontrol_vx.c` (wiringPi → BCM mapping), `zynmcp23017.c` — <https://github.com/zynthian/zyncoder>
- **Raspberry Pi kernel** (`rpi-6.12.y`): `arch/arm/boot/dts/overlays/README`, `sound/soc/codecs/pcm186x.c` — <https://github.com/raspberrypi/linux>
- Zynthian wiki, *Adding Encoders* (encoder order) — <https://wiki.zynthian.org/index.php/Adding_Encoders>

## Kit detection

Zynthian's autoconfig recognises a V4 by these I²C chips:

| Board | Chips | Meaning |
|---|---|---|
| ZynScreen | MCP23017 @ 0x20 + no KMS display | screen + controls |
| ZynADAC | PCM1863 @ 0x4A + PCM5242 @ 0x4D | ADC + DAC |

ZynADAC + ZynScreen = V4. Only PCM5242 (0x4D) + ZynScreen = a V2 kit **without audio input**. `sudo fxbox-hwtest check` shows which chips are present.

## Screen — ZynScreen 3.5 (v1)

| Setting | Value |
|---|---|
| Controller | ILI9486 via fbtft (`compatible = "ilitek,ili9486"`) |
| Bus | SPI0, CE0, 64 MHz |
| D/C | GPIO 24 |
| Reset | GPIO 25 (active low) |
| Backlight | GPIO 22 (`gpio-backlight`, on by default) |
| Overlays | `piscreen2r-notouch,rotate=270` + `piscreen2r-backlight` |
| Image | 480 × 320, 16-bit RGB565, framebuffer `/dev/fb1` (with HDMI as `fb0`) |
| Touch | ADS7846 on CE1, IRQ GPIO 17 — **not used** |

The standard Pi kernel only contains `piscreen2r` (with touch). FXBox compiles Zynthian's `piscreen2r-notouch.dts` and `piscreen2r-backlight.dts` (included unmodified in `system/overlays/`). The compiled `piscreen2r-notouch.dtbo` has been verified bit-for-bit against the binary shipped by Zynthian.

## Controls — MCP23017

| Setting | Value |
|---|---|
| I²C | bus 1, address 0x20 |
| INTA | BCM GPIO 27 (Zynthian: wiringPi 2) |
| INTB | BCM GPIO 4 (Zynthian: wiringPi 7) |
| Registers | inputs with pull-up, interrupt on change, INTPOL active high (as in zyncoder) |

Zynthian numbers MCP pins 100–115 (100 = GPA0 … 108 = GPB0). FXBox uses 0–15.

| | A | B | Push | Position (Zynthian order) |
|---|---|---|---|---|
| K1 | GPA2 | GPA1 | GPA0 | top left |
| K2 | GPA5 | GPA4 | GPA3 | bottom left |
| K3 | GPB2 | GPB1 | GPB0 | top right |
| K4 | GPB5 | GPB4 | GPB3 | bottom right |
| S1–S4 | GPA6, GPA7, GPB6, GPB7 | | | order to be confirmed |

## Audio — ZynADAC / HiFiBerry DAC+ ADC Pro

| Setting | Value |
|---|---|
| Overlay | `dtoverlay=hifiberry-dacplusadcpro` + `force_eeprom_read=0` |
| ALSA card | `sndrpihifiberry` |
| ADC input selection | `ADC Left Input = VINL2[SE] + VINL1[SE]`, `ADC Right Input = VINR2[SE] + VINR1[SE]` (as in Zynthian) |
| JACK | 48 kHz, 2 in / 2 out, period 128 × 2 (configurable; Zynthian uses 256 × 2) |
| Built-in Pi audio | off (`dtparam=audio=off`) |

## DIN MIDI

| Setting | Value |
|---|---|
| Device | `/dev/ttyAMA0` (PL011 UART on GPIO 14/15) |
| Overlays | `miniuart-bt` (Bluetooth moved to the mini UART), `midi-uart0` (adjusted UART clock) |
| Baud rate | tty at 38400 = 31250 baud on the cable (same method as Zynthian/ttymidi) |
| Serial console | off (otherwise the UART would not be free) |

## GPIO overview (no conflicts)

| GPIO | Function |
|---|---|
| 2, 3 | I²C1 (MCP23017, ADC/DAC control) |
| 4 | MCP23017 INTB |
| 7–11 | SPI0 (screen, touch) |
| 14, 15 | UART0 (DIN MIDI) |
| 17 | touch IRQ (not used) |
| 18–21 | I²S (audio) |
| 22 | backlight |
| 24, 25 | screen D/C, reset |
| 27 | MCP23017 INTA |

## To confirm on the device

1. **Order of S1–S4.** The pins are known, the order is not documented. Confirm it with `sudo fxbox-hwtest controls --write`.
2. **Direction of rotation** per encoder (`reverse` flag in `hardware.toml`).
3. **Position of K1–K4** on your front panel (the tiles on the screen follow the Zynthian order).
4. **Which audio card** is actually fitted (ZynADAC or HiFiBerry DAC+ ADC Pro): same overlay, but mixer control names can differ per kernel. `fxbox-mixer` writes the controls it finds to `/var/lib/fxbox/mixer-controls.txt`.
5. **Screen revision.** "ZynScreen v1.5" is not listed separately in Zynthian's configuration; the V4 configuration uses "ZynScreen 3.5 (v1)". If `fxbox-hwtest screen` does not show a correct image, send a photo of the circuit board and the output of `dmesg | grep -i -E "fb|ili"`.
