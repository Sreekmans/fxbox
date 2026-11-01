# Hardwareconfiguratie en bronnen

[English](HARDWARE.md) · **Nederlands**

Alle waarden komen uit Zynthians eigen configuratie voor de V4-kit. Niets is geraden; wat niet uit de bronnen vast te stellen is, staat onderaan bij "Te bevestigen op het toestel". Hardware-instellingen staan los van de code in `/etc/fxbox/hardware.toml` en `/etc/fxbox/audio.conf`.

Bronnen (geraadpleegd 31-10-2026):

- **zynthian-sys** (branch `vangelis`, commit `c2eea576`): `config/zynthian_envars_V4.sh`, `sbin/zynthian_autoconfig.py`, `boot/config.txt`, `custom/display/ZynScreen 3.5 (v1)/boot/overlays/*.dts`, `sbin/fix_alsamixer_settings.sh`, `etc/systemd/mod-ttymidi.service` — <https://github.com/zynthian/zynthian-sys>
- **zynthian-webconf**: `lib/kit_config_handler.py`, `lib/wiring_config_handler.py`, `lib/display_config_handler.py`, `lib/audio_config_handler.py` — <https://github.com/zynthian/zynthian-webconf>
- **zyncoder**: `zyncontrol_vx.c` (omzetting wiringPi → BCM), `zynmcp23017.c` — <https://github.com/zynthian/zyncoder>
- **Raspberry Pi kernel** (`rpi-6.12.y`): `arch/arm/boot/dts/overlays/README`, `sound/soc/codecs/pcm186x.c` — <https://github.com/raspberrypi/linux>
- Zynthian-wiki, *Adding Encoders* (volgorde van de encoders) — <https://wiki.zynthian.org/index.php/Adding_Encoders>

## Herkenning van de kit

Zynthians autoconfig herkent een V4 aan deze I²C-chips:

| Bord | Chips | Betekenis |
|---|---|---|
| ZynScreen | MCP23017 @ 0x20 + geen KMS-scherm | scherm + bediening |
| ZynADAC | PCM1863 @ 0x4A + PCM5242 @ 0x4D | ADC + DAC |

ZynADAC + ZynScreen = V4. Alleen PCM5242 (0x4D) + ZynScreen = een V2-kit **zonder audio-ingang**. `sudo fxbox-hwtest check` toont welke chips aanwezig zijn.

## Scherm — ZynScreen 3.5 (v1)

| Instelling | Waarde |
|---|---|
| Controller | ILI9486 via fbtft (`compatible = "ilitek,ili9486"`) |
| Bus | SPI0, CE0, 64 MHz |
| D/C | GPIO 24 |
| Reset | GPIO 25 (actief laag) |
| Achtergrondlicht | GPIO 22 (`gpio-backlight`, standaard aan) |
| Overlays | `piscreen2r-notouch,rotate=270` + `piscreen2r-backlight` |
| Beeld | 480 × 320, 16 bit RGB565, framebuffer `/dev/fb1` (met HDMI als `fb0`) |
| Touch | ADS7846 op CE1, IRQ GPIO 17 — **niet gebruikt** |

De standaard Pi-kernel bevat alleen `piscreen2r` (mét touch). FXBox compileert Zynthians `piscreen2r-notouch.dts` en `piscreen2r-backlight.dts` (meegeleverd in `system/overlays/`, ongewijzigd). De gecompileerde `piscreen2r-notouch.dtbo` is bit-voor-bit gecontroleerd tegen de door Zynthian meegeleverde binary.

## Bediening — MCP23017

| Instelling | Waarde |
|---|---|
| I²C | bus 1, adres 0x20 |
| INTA | BCM GPIO 27 (Zynthian: wiringPi 2) |
| INTB | BCM GPIO 4 (Zynthian: wiringPi 7) |
| Registers | ingangen met pull-up, interrupt bij verandering, INTPOL actief hoog (zoals zyncoder) |

Zynthian nummert MCP-pinnen als 100–115 (100 = GPA0 … 108 = GPB0). FXBox gebruikt 0–15.

| | A | B | Drukknop | Plaats (Zynthian-volgorde) |
|---|---|---|---|---|
| K1 | GPA2 | GPA1 | GPA0 | linksboven |
| K2 | GPA5 | GPA4 | GPA3 | linksonder |
| K3 | GPB2 | GPB1 | GPB0 | rechtsboven |
| K4 | GPB5 | GPB4 | GPB3 | rechtsonder |
| S1–S4 | GPA6, GPA7, GPB6, GPB7 | | | volgorde te bevestigen |

## Audio — ZynADAC / HiFiBerry DAC+ ADC Pro

| Instelling | Waarde |
|---|---|
| Overlay | `dtoverlay=hifiberry-dacplusadcpro` + `force_eeprom_read=0` |
| ALSA-kaart | `sndrpihifiberry` |
| Ingangskeuze ADC | `ADC Left Input = VINL2[SE] + VINL1[SE]`, `ADC Right Input = VINR2[SE] + VINR1[SE]` (zoals Zynthian) |
| JACK | 48 kHz, 2 in / 2 uit, periode 128 × 2 (instelbaar; Zynthian gebruikt 256 × 2) |
| Ingebouwde Pi-audio | uit (`dtparam=audio=off`) |

## DIN-MIDI

| Instelling | Waarde |
|---|---|
| Apparaat | `/dev/ttyAMA0` (PL011 UART op GPIO 14/15) |
| Overlays | `miniuart-bt` (Bluetooth naar de mini-UART), `midi-uart0` (UART-klok aangepast) |
| Baudrate | tty op 38400 = 31250 baud op de kabel (zelfde methode als Zynthian/ttymidi) |
| Seriële console | uit (anders zou de UART niet vrij zijn) |

## GPIO-overzicht (geen conflicten)

| GPIO | Functie |
|---|---|
| 2, 3 | I²C1 (MCP23017, ADC/DAC-besturing) |
| 4 | MCP23017 INTB |
| 7–11 | SPI0 (scherm, touch) |
| 14, 15 | UART0 (DIN-MIDI) |
| 17 | touch-IRQ (niet gebruikt) |
| 18–21 | I²S (audio) |
| 22 | achtergrondlicht |
| 24, 25 | scherm D/C, reset |
| 27 | MCP23017 INTA |

## Te bevestigen op het toestel

1. **Volgorde S1–S4.** De pinnen zijn bekend, de volgorde niet gedocumenteerd. Bevestig ze met `sudo fxbox-hwtest controls --write`.
2. **Draairichting** per encoder (vlag `reverse` in `hardware.toml`).
3. **Plaats van K1–K4** op jouw front (de tegels op het scherm volgen de Zynthian-volgorde).
4. **Welke audiokaart** er echt op zit (ZynADAC of HiFiBerry DAC+ ADC Pro): zelfde overlay, maar de namen van de mixerregelaars kunnen per kernel verschillen. `fxbox-mixer` schrijft de gevonden regelaars naar `/var/lib/fxbox/mixer-controls.txt`.
5. **Schermrevisie.** "ZynScreen v1.5" is in Zynthians configuratie niet apart vermeld; de V4-configuratie gebruikt "ZynScreen 3.5 (v1)". Toont `fxbox-hwtest screen` geen goed beeld, stuur dan een foto van de printplaat en de uitvoer van `dmesg | grep -i -E "fb|ili"`.
