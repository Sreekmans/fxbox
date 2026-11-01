# Testrapport

Stand: 01-11-2026, versie 1.2.0 (filtertypes en pagina 2, distortiontypes met 2× oversampling, compressor/limiter aan het einde van de keten).

**Op de echte hardware zijn inmiddels de installatie, hardwareherkenning, scherm, bediening en een eerste DSP-meting gedaan (zie onderaan); de overige punten staan nog open.** Alle tests hieronder draaiden op een ontwikkelmachine (x86-64, Ubuntu 24.04) en voor ARM64 onder emulatie (qemu). Er zijn dus nog geen hardwaretests gedaan en er is geen werkende audio, gemeten latentie of opstarttijd op het toestel vastgesteld. Het installatiescript voert de engine- en interfacetests ook op de Pi zelf uit. De uitvoer daarvan komt in `install/test_dsp_on_pi.txt`.

## Uitgevoerde tests

### Audio-engine, offline (`engine/tests/test_dsp.cpp`)

Uitgevoerd op x86-64 en, gecompileerd voor Cortex-A72 (`-mcpu=cortex-a72`), onder qemu-aarch64. Alles geslaagd.

| Test | Resultaat |
|---|---|
| Stabiliteit: maximale feedback, luide ruis, 20 s per effect | geen NaN of DSP-fouten; de staart groeit niet na het stoppen van de ingang |
| Echotijd 100 / 375 / 1000 ms | eerste herhaling op 100,44 / 374,79 / 999,58 ms |
| Bypass met staarten uit | uitgang = ingang (max. afwijking 0) |
| Klikvrij: bypass, effectwissel naar Reverb en naar Phaser, mix 0→1, ingang +12 dB | grootste sample-stap blijft binnen 2× die van de schone sinus (rekening houdend met het niveau) |
| Mono-ingang | L en R uitgang identiek |
| VU-kalibratie (sinus −18 dBFS RMS) | −18,01 dBFS; samplepiek −14,99 dBFS |
| VU-stijgtijd | 98,9 % na 300 ms |
| Clipdetectie | gedetecteerd |

Bij de eerste uitvoering vond deze test een echte fout: de Reverb gaf direct na het opstarten NaN, omdat de afvlakking van de decay-parameter vanaf 0 begon (deling door nul). Dat is opgelost (`max(0.2)` na de afvlakking) en de stabiliteitstest dekt het nu af.

Rekentijd voor 30 s audio, **ter informatie en geen Pi-meting**:

- x86: 78× tot 272× sneller dan realtime.
- ARM64 onder emulatie: 3,9× tot 16× sneller dan realtime. Emulatie is veel trager dan een echte Pi 4, dus ook dit cijfer zegt niets over de belasting op het toestel.

### Versie 1.1: keten en nieuwe effecten (`engine/tests/test_dsp.cpp`)

Geslaagd op x86-64 en onder ARM64-emulatie:

| Test | Resultaat |
|---|---|
| Stabiliteit Distortion, Saturation, Filter (maximale instellingen, luide ruis, 20 s) | geen NaN of DSP-fouten, staart groeit niet |
| Hele keten (alle 7 effecten aan), 20 s luide ruis | geen NaN of DSP-fouten, staart neemt af |
| Filter (cutoff 300 Hz) | 100 Hz: −1,8 dB; 5 kHz: −99 dB |
| Distortion (volle drive) | 3e harmonische −9,4 dB t.o.v. de grondtoon |
| Saturation | kleine signalen −0,1 dB (vrijwel eenheidsversterking); luide signalen krijgen boventonen |
| Klikvrij: effect aan/uit (Chorus uit + Reverb aan, Phaser aan, Filter aan) | binnen de grens van de schone sinus |
| Automatische mono-ingang | stille IN R → R-uitgang = L-uitgang; signaal op IN R → weer stereo |

Rekentijd van de hele keten (alle 7 effecten aan), ter informatie: 29× realtime op x86, 1,4× realtime onder ARM64-emulatie. Emulatie is veel trager dan een echte Pi 4; **de DSP-belasting op het toestel is nog niet gemeten** (zie *Instellingen → Info*).

Interface: 22 unittests, waaronder het ketenscherm, K2 kort/lang, het opslaan en terugladen van een preset met meerdere effecten, en het inlezen van een toestand uit versie 1.0.

### Versie 1.2: filtertypes, distortiontypes, compressor (`engine/tests/test_dsp.cpp`)

Geslaagd op x86-64 en, gecompileerd voor Cortex-A72, onder ARM64-emulatie (qemu).

| Test | Resultaat |
|---|---|
| Filtertypes bij cutoff 1 kHz, reso 0 (100 Hz / 1 kHz / 10 kHz) | Ladder −0,4 / −12,2 / −85,8 dB · LP −0,3 / −6,2 / −43,0 dB · HP −40,3 / −6,2 / −0,3 dB · BP −20,3 / −6,2 / −21,6 dB · Notch −0,4 / −44,8 / −0,3 dB |
| Filter: alle vier LFO-vormen met envelope en drive op maximum, luide ruis | geen NaN of DSP-fouten |
| Filter: typewissel LP → HP tijdens een sinus | grootste sample-stap gelijk aan die van de sinus zelf (geen klik) |
| Distortion: alle vijf types, luide ruis, 5 s | geen NaN of DSP-fouten, hoorbaar signaal |
| Distortion: aliasing, sinus van 7 kHz, drive 50 % (alle niet-harmonische componenten opgeteld, t.o.v. de harmonischen) | Overdrive −32,1 dB (zonder oversampling −20,1) · Distortion −25,6 (−15,5) · Fuzz −27,3 (−17,6) · Folder −22,8 (−16,1). Bitcrush is bewust niet gemeten: die aliast expres. |
| Distortion op 50 % mix: frequentiegang 0,5–8 kHz | 2,5 dB spreiding (geen kamfiltering: het droge pad wordt 23 samples vertraagd, gelijk aan de oversampler) |
| Distortion aan/uit tijdens een sinus | grootste stap 0,025 tegen 0,017 voor de sinus zelf (binnen de grens van 2×) |
| Compressor: sinus −6 dBFS piek, drempel −20 dB, ratio 4, harde knie | uitgang −16,1 dBFS piek (theorie −16,5); gain reduction gemeld: 10,0 dB |
| Limiter: +24 dB make-up op een luid signaal | piek −0,76 dBFS, geen clipping |
| Compressor uit | gain reduction 0 gemeld |
| Hele keten, alle 8 effecten aan, luide ruis 20 s | geen NaN of DSP-fouten, staart neemt af |

Rekentijd, ter informatie en **geen Pi-meting**:

- x86: hele keten 18× realtime; Distortion 48× (met oversampling het zwaarste effect), de andere effecten 100× tot 265×.
- ARM64 onder emulatie: hele keten 0,9× realtime; Distortion 2,6×, Reverb 4,1×, de andere 8,6× tot 16×. Emulatie is veel trager dan een echte Pi 4.

De Distortion is door de oversampling ongeveer twee keer zo zwaar als in versie 1.1. Hoe zwaar op het toestel zelf, moet nog gemeten worden (zie hieronder, punt 15b).

Interface: 30 unittests (8 nieuw), onder andere:

- K4 vasthouden wisselt de pagina en de knoppen volgen;
- keuzeparameters gaan één stap per klik, ook bij snel draaien;
- een lege plaats op pagina 2 doet niets;
- een toestand uit versie 1.1 (4 waarden per effect) krijgt standaardwaarden voor pagina 2;
- pagina-2-waarden blijven bewaard in een gebruikerspreset;
- de gain reduction wordt uit het engine-protocol gelezen, ook van een oudere engine zonder dat veld.

### Tempo en MIDI-clock (`engine/tests/test_tempo.cpp`, `engine/tests/din_clock_sim.py`)

| Test | Resultaat |
|---|---|
| 174 BPM met 1,5 ms jitter | na 2 tellen binnen 1 BPM; daarna max. 0,15 BPM afwijking over 64 tellen; 0 tempo-updates naar de DSP door de dode band |
| 120 BPM met 3 ms jitter | max. 0,12 BPM afwijking |
| Tempowissel 120 → 140 | na 3 tellen 139,4; na 7 tellen 140,0 |
| Clock weg / terug | verloren na 0,5 s, tempo vastgehouden, nieuwe clock snel opgepikt |
| Start-bericht | de eerste puls daarna wordt gemarkeerd (LFO-reset) |
| Tap tempo, verdelingen | correct (o.a. 1/8D bij 120 BPM = 375 ms) |
| **Integratie:** gesimuleerde DIN-clock via een pseudo-terminal in de echte engine | 'din' herkend, 173,9–174,1 BPM getoond, verlies gemeld, tempo vastgehouden |

### Interface (`ui/tests/test_ui.py`: 19 tests)

- **Encoders en knoppen:** encoderdecoder (rechtsom, linksom, contactdender, halve draai, omkeren) en ontdendering van de knoppen.
- **Pinkoppeling:** MCP-pinkoppeling met de V4-standaardwaarden.
- **Bediening:**
  - parameter draaien;
  - draaien wordt genegeerd tijdens en vlak na indrukken;
  - bypass en tap reageren bij indrukken;
  - effectkeuze;
  - lang indrukken om op te slaan;
  - navigeren door presets en banken (met omloop);
  - sync-verdelingen;
  - tap zonder sync.
- **Opslag:** vertraagd en atomisch opslaan (geen schrijfacties tijdens het draaien) en herstel na een herstart; een beschadigd toestandsbestand wordt opzijgezet.
- **Engine-koppeling:** de volledige toestand wordt opnieuw verstuurd bij het (her)verbinden, foutmeldingen verschijnen op het scherm en de protocol-parser is getest.

### Integratie interface + engine (`ui/tests/integration_engine.py`)

De echte `fxbox.main` draaide tegen de echte `fxbox-engine` op een dummy-JACK-server, met een framebuffer in het geheugen.

| Gecontroleerd | Resultaat |
|---|---|
| Beelden getekend | 166 in ongeveer 5,5 s (30 fps) |
| Geschreven schermrijen | 613 rijen in totaal (alleen gewijzigde rijen) |
| Bewaard | sync, bankwissel en de bewerkt-markering |
| Afsluiten | netjes bij SIGTERM |

### Overig

- Desktopdemo start en tekent headless (SDL dummy).
- Engine-protocol tegen dummy-JACK: verbinding met de JACK-poorten gemaakt en statusregels ontvangen.
- `boot-config.sh`: idempotent (één blok na twee keer uitvoeren), seriële console verwijderd, herstel geeft exact het origineel terug.
- `shellcheck` op alle scripts: geen waarschuwingen.
- `systemd-analyze verify` op alle diensten: geen syntaxfouten.
- `build-image.sh` getest met een nagemaakt Raspberry Pi OS-image (twee partities, `--skip-install`):
  - vergroten van partitie 2, plus `resize2fs`;
  - koppelen via offsets, dus zonder udev;
  - controle op de Bookworm-basis;
  - `userconf.txt`, het `ssh`-bestand en de hostnaam;
  - `resolv.conf` teruggezet en `policy-rc.d` verwijderd;
  - uitvoer `.img`, `.xz` en `.sha256`;
  - geen loop-apparaten achtergebleven.
- **Nog niet getest** voor `build-image.sh`: de download van het echte Raspberry Pi OS-image en het uitvoeren van `install.sh` in de ARM64-chroot. Die downloads zijn vanuit de ontwikkelomgeving niet bereikbaar. De FAT-bootpartitie werd in de test vervangen door ext4, omdat de testomgeving geen vfat kan mounten.
- Engine: 0 compilerwaarschuwingen. `main.cpp` en `midi_input.cpp` controleren ook voor aarch64 foutloos (`-fsyntax-only`); het volledige linken gebeurt op de Pi.
- Overlay: `piscreen2r-notouch.dts` gecompileerd is identiek aan Zynthians meegeleverde `.dtbo`.
- Python-code vereist minimaal 3.11 (Bookworm levert 3.11), gecontroleerd met vermin.

Gebruikte versies: Faust 2.70.3 (gegenereerde C++ staat in de repo, de Pi heeft Faust niet nodig), pycairo 1.29.2 en pygame 2.6.1 (desktop). De versies van de Debian-pakketten op de Pi worden bij de installatie vastgelegd in `install/installed-package-versions.txt`.

## Nog te controleren op het toestel

| # | Controle | Hoe |
|---|---|---|
| 1 | Installatie op schone Raspberry Pi OS Lite 64-bit Bookworm; engine- en interfacetests slagen op de Pi | `install.sh`-uitvoer, `install/test_dsp_on_pi.txt` |
| 2 | Hardware aanwezig (MCP23017, PCM1863, PCM5242, kaart, framebuffer, UART) | `sudo fxbox-hwtest check` |
| 3 | Scherm: oriëntatie, kleuren (BGR), rand rondom, leesbaarheid | `sudo fxbox-hwtest screen` |
| 4 | Volgorde S1–S4, plaats en draairichting K1–K4 | `sudo fxbox-hwtest controls --write` |
| 5 | Encoders: geen gemiste of dubbele stappen, geen parameterwijziging bij indrukken | met de hand |
| 6 | Opstarten zonder desktop of terminal op het ZynScreen; opstarttijd tot bruikbaar scherm | stopwatch; `systemd-analyze` |
| 7 | Audio in → effect → uit; niveaus; mono- en stereo-ingang; ADC-ingangskeuze klopt met de bussen | instrument + koptelefoon/monitor |
| 8 | Latentie | `sudo ./install/measure-latency.sh` met loopbackkabel |
| 9 | Geen xruns bij `PERIOD=128` gedurende 1 uur met Reverb (zwaarste effect); anders 256 | *Instellingen → Info* |
| 10 | DSP-belasting en CPU-temperatuur | *Info*; `vcgencmd measure_temp` |
| 11 | Meters volgen het signaal; CLIP en IN CLIP bij overstuurd signaal | testtoon |
| 12 | DIN-MIDI-clock van een echt apparaat (timing via de midi-uart0-overlay) | drumcomputer/sequencer via DIN |
| 13 | USB-MIDI-clock en inpluggen tijdens gebruik; bronkeuze bij twee apparaten | twee clockbronnen |
| 14 | Start/Stop/Continue-gedrag | sequencer |
| 15 | Bypass, effect aan/uit, presetwissel klikvrij in de praktijk | luisteren |
| 15b | DSP-belasting met de hele keten aan (vooral Reverb + Tape Echo + Distortion, die nu 2× oversampled is) | *Instellingen → Info* |
| 15c | Filtertypes, distortiontypes en compressor klinken zoals bedoeld; GR-weergave in de keten volgt het signaal | luisteren met een instrument |
| 16 | Stroom eraf direct na een wijziging en ruim daarna: toestand hersteld, geen corrupte bestanden | stekker eruit |
| 17 | Foutmeldingen: geluidskaart los (toestel uit!), engine stoppen | `sudo systemctl stop fxbox-engine` |
| 18 | Automatisch herstarten na een crash | `sudo kill -SEGV $(pidof fxbox-engine)` |

## Metingen op het toestel

| Meting | Waarde | Datum |
|---|---|---|
| Rondgangslatentie (128 × 2 @ 48 kHz) | *nog niet gemeten* | |
| Opstarttijd tot effectinterface | *nog niet gemeten* | |
| DSP-belasting (JACK, 48 kHz, 128 × 2) met Filter, Saturation, Distortion en Phaser aan | 7 % (afgelezen op het toestel, *Instellingen → Info*) | 01-11-2026 |
| DSP-belasting met Reverb + Tape Echo aan | *nog niet gemeten* | |
| DSP-belasting met de hele keten (8 effecten, versie 1.2) | *nog niet gemeten* | |
| Xruns (met Filter, Saturation, Distortion en Phaser aan) | 0 | 01-11-2026 |
| Xruns in 1 uur | *nog niet gemeten* | |
