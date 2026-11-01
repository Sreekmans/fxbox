# Architectuur en technische keuzes

[English](ARCHITECTURE.md) · **Nederlands**

## Overzicht

```
 instrument ─► ADC ─► JACK ─► fxbox-engine ─► JACK ─► DAC ─► uitgang L/R
                              │  (C++, realtime)
       DIN-MIDI (UART) ──────►│  MIDI-clock, tap tempo, sync
       USB-MIDI (ALSA seq) ──►│
                              │ UNIX-socket (tekstregels)
                              ▼
                         fxbox-ui (Python)
              MCP23017 ──► encoders/knoppen ─┐
                                             ├─► controller ─► cairo ─► /dev/fb1 (ZynScreen)
                       presets/state (JSON) ─┘
```

Drie systemd-diensten, elk met een eigen verantwoordelijkheid:

| Dienst | Taak | Prioriteit / kernen |
|---|---|---|
| `fxbox-jack` | JACK2-server op de ZynADAC/HiFiBerry | realtime prio 70, kernen 2–3 |
| `fxbox-engine` | effecten, meters, MIDI-clock, tempo | JACK-thread realtime; kernen 2–3 |
| `fxbox-ui` | scherm, encoders, knoppen, presets | normaal (`nice 5`), kernen 0–1 |

Valt de interface weg of herstart ze, dan loopt de audio door. Herstart de engine, dan zet de interface automatisch de volledige toestand terug.

## Besturingssysteem

| Optie | Beoordeling |
|---|---|
| **Raspberry Pi OS Lite 64-bit, Bookworm** ✔ | Dezelfde basis waarop Zynthian zijn V4-ondersteuning bouwt (`setup_system_raspioslite_64bit_bookworm.sh`). De ZynScreen-overlays, de `hifiberry-dacplusadcpro`-overlay en `midi-uart0` zijn daarop beproefd. Geen desktop. |
| Raspberry Pi OS Trixie | Zynthians Trixie-script is nog in ontwikkeling; installatie geeft een waarschuwing maar werkt waarschijnlijk. |
| Zynthian-image | Uitgesloten door de opdracht. |
| Buildroot / Yocto | Kleiner, maar veel meer werk en minder onderhoudbaar voor één toestel. |

## Audio-aanpak

| Optie | Voor | Tegen |
|---|---|---|
| **Eigen C++-engine op JACK, DSP uit Faust-bibliotheken** ✔ | Volledige controle over overgangen, bypass, staarten, metering en clock in één realtime-pad; offline testbaar; Faust-bibliotheken (GRAME; reverb = zita-rev1 van Fons Adriaensen) zijn bewezen en onderhouden; geen runtime-afhankelijkheid van Faust (gegenereerde C++ staat in de repo). | Nieuwe effecten vragen een Faust-bestand en een hercompilatie. |
| mod-host + LV2-plugins | Groot aanbod plugins. | Bypass/crossfades/staarten/meters moeten rond externe plugins gebouwd worden; tempo-sync en parameter-smoothing verschillen per plugin; meer bewegende delen bij het opstarten. |
| Pure Data / SuperCollider | Snel prototypen. | Moeilijker robuust te maken als apparaat; zwaardere runtime. |
| Rechtstreeks ALSA zonder JACK | Iets minder lagen. | JACK is op deze hardware in Zynthian beproefd, levert xrun-rapportage en `jack_iodelay` voor latentiemeting. |

**Licenties.** Faust-bibliotheken: LGPL met uitzondering die het verspreiden van gegenereerde code onder een eigen licentie toestaat. De ZynScreen-overlays komen uit zynthian-sys (GPL-3.0). FXBox zelf: GPL-3.0-or-later.

**Effectketen.** Acht effecten in vaste volgorde (`kChainOrder` in `effects.hpp`): Filter → Saturation → Distortion → Chorus → Phaser → Tape Echo → Reverb → Compressor. Per slot: invoerversterking (naar het effect), droge doorvoer en natte uitvoer, elk per sample afgevlakt. Een slot dat uit staat en stil is, wordt overgeslagen: het signaal gaat er onveranderd doorheen en het kost geen rekentijd.

**Parameters en pagina's.** Elk effect heeft 8 parameterplaatsen (indices 0–7 in protocol, presets en engine); 0–3 vormen pagina 1, 4–7 pagina 2. Ongebruikte plaatsen hebben het type `None` en worden overgeslagen. Keuzeparameters (filtertype, LFO-vorm, distortiontype, limiter) zijn gewone Faust-sliders met stap 1; in de Faust-code worden ze omgezet in afgevlakte één-uit-n-gewichten, zodat een wissel in ~20 ms overvloeit in plaats van te klikken. Nadeel: alle varianten worden steeds berekend.

**Oversampling (Distortion).** De Distortion draait op 2× de samplerate. Op- en neerbemonsteren gebeurt met een lineair-fase half-band-FIR van 47 taps (Kaiser-venster, β = 6; `oversampler.hpp`), waarbij de nultaps worden overgeslagen. De totale vertraging is precies 23 samples op de gewone samplerate. Het droge pad van dat slot krijgt dezelfde vertraging, zodat parallelle mix geen kamfilter geeft. Die vertraging wordt in 10 ms in- en uitgemengd bij aan- en uitzetten, dus zonder klik, en kost geen latentie zolang de Distortion uit staat. Gemeten vermindering van de aliasing: 6 tot 12 dB, afhankelijk van het type (zie testrapport). Bij extreme drive blijft aliasing hoorbaar; 4× zou meer helpen, maar kost twee keer zoveel rekentijd.

**Compressor.** Feed-forward, stereo gekoppeld, met zachte knie (`co.FFcompressor_N_chan` uit de Faust-bibliotheek), gevolgd door een vaste snelle begrenzer (drempel −1 dB, attack 0,5 ms) die met *Limiter* wordt in- en uitgemengd. De gain reduction komt als Faust-bargraph `gr` naar buiten; de engine stuurt het maximum van beide kanalen mee als 11e veld van de `m`-regel. De interface leest ook een oudere `m`-regel zonder dat veld.

**Uitbreidbaarheid.** Een effect bestaat uit een `.dsp`-bestand (`engine/dsp/`), een regel in `engine/src/effects.hpp` (veilige grenzen, sync-soort, plaats in de keten) en een `EffectDef` in `ui/fxbox/model.py`. Een LV2-host (lilv) kan later als extra slottype worden toegevoegd.

## Realtime-regels in de engine

- De JACK-callback roept alleen `EngineCore::process` aan: geen geheugenallocatie, geen locks, geen systeemaanroepen, geen bestandstoegang.
- Bediening komt binnen via een lock-vrije single-producer/single-consumer-wachtrij (`spsc.hpp`); meters gaan terug via atomics.
- Alle buffers worden bij het starten gealloceerd. Effecten worden bij het starten "opgewarmd" zodat hun parameter-smoothers op de juiste waarde staan.
- Denormals worden op de audiothread op nul gezet (ARM64 FPCR.FZ; x86 MXCSR).
- Elk blok wordt op NaN/Inf gecontroleerd; een effect dat ontspoort wordt leeggemaakt zonder dat de engine stopt (melding op het scherm).

## Gedrag (vastgelegd)

| Situatie | Gedrag |
|---|---|
| Parameterwijziging | Alle Faust-parameters glijden (`si.smoo`); dry/wet en versterkingen volgen per sample met ~10 ms tijdconstante. |
| Delaytijd verandert | Glijdt in ~250 ms, zoals een bandtransport: korte toonhoogtebuiging, geen klik. |
| Feedback | Begrensd in de engine (Tape 95 %, Chorus/Phaser 90 %), ongeacht wat de interface stuurt; band-saturatie in de lus van de Tape Echo. Filterresonantie blijft onder zelfoscillatie. |
| Distortion | 2× oversampled (zie hierboven). Typewissel vloeit over in ~20 ms. |
| Filtertype / LFO-vorm | Wissel vloeit over in ~20 ms; LFO-reset bij MIDI Start geldt voor alle vormen. |
| Compressor | Altijd laatste in de keten; Mix < 100 % = parallelle compressie. Limiter standaard aan. |
| Effect aan | Invoer naar het effect en de natte uitvoer faden in 10 ms in; de droge doorvoer gaat naar cos(mix·π/2). |
| Effect uit | Invoer naar het effect fadet in 10 ms weg, de droge doorvoer naar 1. Met "Staarten bij bypass" aan klinkt de staart (echo, galm) uit; uit = natte uitvoer binnen 10 ms weg. |
| Bypass (hele keten) | Gebufferde bypass (geen hardwarematige true bypass): alle effecten zoals "effect uit", dus droog signaal op eenheidsversterking zolang de software draait, met of zonder staarten. |
| Presetwissel | Effecten die in beide presets aan staan: staart blijft klinken (spill-over), parameters glijden naar de nieuwe waarden. Andere effecten gaan aan of uit zoals hierboven. |
| Mix | Equal-power per effect: droog = cos(mix·π/2), nat = sin(mix·π/2). Het filter heeft geen Mix (altijd volledig nat). |
| Ingang Automatisch | Is IN R 0,25 s stiller dan −80 dBFS terwijl IN L signaal draagt (> −60 dBFS), dan wordt L naar beide kanalen gestuurd (crossfade 10 ms). Zodra IN R signaal krijgt, weer stereo. |

## MIDI-clock en tempo

- 24 klokpulsen per kwartnoot. Tempo = kleinste-kwadratenschatting over de laatste 96 pulsen (4 tellen), plus een lichte afvlakking. Een echte tempowissel (>3 % afwijking gedurende een halve tel) start het venster opnieuw zodat binnen enkele tellen wordt gevolgd.
- Tempo-afhankelijke parameters worden pas bijgewerkt bij een verandering van meer dan 0,2 % (dode band), zodat jitter de delaytijd niet laat wiebelen.
- **Clock wegvallen:** na 0,5 s zonder puls telt de clock als verloren; het laatst ontvangen tempo blijft behouden (geen sprong), het scherm toont dat er geen clock binnenkomt.
- **Start (FA):** de eerste puls na Start zet de LFO's van Filter, Chorus en Phaser op fase 0 (bij sync aan). **Continue (FB):** geen fasereset. **Stop (FC):** effecten lopen door, tempo blijft staan.
- **Bronnen:** DIN-MIDI (UART) en alle USB-MIDI-apparaten (ALSA-sequencer, ook bij later aansluiten). Keuze in Instellingen: Automatisch (DIN heeft voorrang, daarna eerste USB-bron met clock), DIN, of één USB-apparaat.
- **Tap tempo:** gemiddelde van de laatste tot 4 intervallen; na 2 s pauze begint een nieuwe reeks. Zonder sync zet tappen ook de Time van de Tape Echo op één kwartnoot. Met sync en een binnenkomende clock heeft de clock voorrang.
- **Sync aan** toont delaytijd als 1/4, 1/8, 1/8D, 1/4T …; zonder sync in ms. Modulatiesnelheden gebruiken verdelingen per LFO-cyclus (1/16 … 8/1).

## Metering

- **VU (naald):** gemiddelde-gelijkrichting met twee gekoppelde polen (~300 ms tot 99 %), gekalibreerd zodat een sinus zijn RMS-waarde toont. De schaal is **relatief**: 0 VU = −18 dBFS (instelbaar: −12/−14/−18/−20). De referentie staat op het scherm.
- **PEAK:** digitale samplepiek in **dBFS**, 1,5 s vasthouden. Dit is een digitale waarde ten opzichte van de volle schaal van de converter, geen analoog niveau in dBu.
- **CLIP:** uitgang ≥ 0 dBFS (vóór de begrenzing); **IN CLIP:** ingang op de volle schaal van de ADC. Beide 1,5 s zichtbaar.
- Er worden geen dBu/dBV-waarden getoond: die hangen af van de analoge kalibratie van de kaart en zijn niet gemeten.

## Interface en grafiek

| Optie | Beoordeling |
|---|---|
| **Python + pycairo, rechtstreeks naar het fbtft-framebuffer** ✔ | Anti-aliased naalden en tekst, klein en leesbaar, geen X/Wayland nodig. Alleen gewijzigde rijen worden geschreven; fbtft ververst alleen de geraakte geheugenpagina's, dus een stil scherm kost geen SPI-tijd. |
| SDL2/pygame op het framebuffer | SDL2 heeft geen fbdev-backend meer; het scherm is geen KMS-apparaat. |
| Qt / LVGL | Krachtig, maar veel zwaarder om te bouwen en te onderhouden voor één scherm. |

Het framebuffer wordt op drivernaam (`ili9486`) gezocht, omdat het nummer (`fb0`/`fb1`) afhangt van HDMI.

**Encoders:** full-step toestandsmachine (tabel van Ben Buxton) die bounce en ongeldige overgangen negeert; draaiingen van een knop worden genegeerd terwijl die knop ingedrukt is en 80 ms na loslaten. **Knoppen:** niveau moet 12 ms stabiel zijn. **Lezen:** op de interruptlijnen van de MCP23017 (libgpiod), met 20 ms polling als vangnet.

## Opslag

- Fabriekspresets: `/opt/fxbox/presets/factory.json` (alleen lezen).
- Gebruikerspresets: `/var/lib/fxbox/user_presets.json` (4 banken × 8 plaatsen).
- Laatste toestand: `/var/lib/fxbox/state.json`, geschreven 2 s nadat de bediening stil is (uiterlijk na 15 s), via tijdelijk bestand + `fsync` + `rename` (atomisch). Beschadigde bestanden worden opzijgezet (`.bad`) en het toestel start met standaardwaarden.
- Journald-logs alleen in RAM (minder schrijfacties naar de SD-kaart).
