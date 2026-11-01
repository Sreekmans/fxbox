#!/bin/bash
# FXBox installer for a fresh Raspberry Pi OS Lite (64-bit, Bookworm) on a
# Raspberry Pi 4 inside a Zynthian Kit V4.
#
# Uitvoeren op de Pi, vanuit de uitgepakte projectmap:
#     sudo ./install/install.sh            (vraagt bevestiging voor de opstartconfiguratie)
#     sudo ./install/install.sh --yes      (zonder vragen)
#
# What it does (idempotent; safe to run again after an update):
#   1. checks the Pi model and OS
#   2. installs packages (JACK, ALSA, build tools, Python cairo/I2C/GPIO, fonts)
#   3. builds the audio engine and runs its offline tests on this Pi
#   4. installs to /opt/fxbox, config to /etc/fxbox, data in /var/lib/fxbox
#   5. compiles the ZynScreen overlays and edits config.txt / cmdline.txt
#      (backups are kept, changes sit between "# >>> fxbox" markers)
#   6. installs and enables the systemd services (autostart)
set -euo pipefail

ASSUME_YES=0
FORCE=0
for a in "$@"; do
    case "$a" in
        --yes|-y) ASSUME_YES=1 ;;
        --force) FORCE=1 ;;
        -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
        *) echo "Onbekende optie: $a"; exit 2 ;;
    esac
done

SRC="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX=/opt/fxbox
ETC=/etc/fxbox
DATA=/var/lib/fxbox
LOG=/var/log/fxbox-install.log

say()  { echo -e "\n==> $*"; }
warn() { echo "WARNING: $*" >&2; }
die()  { echo "ERROR: $*" >&2; exit 1; }
ask()  {
    [ "$ASSUME_YES" = 1 ] && return 0
    read -r -p "$1 [j/N] " r
    [[ "$r" =~ ^[jJyY]$ ]]
}

[ "$(id -u)" = 0 ] || die "start met sudo"
exec > >(tee -a "$LOG") 2>&1
echo "FXBox install $(date -Is) from $SRC"

# ------------------------------------------------------------------ 1. checks
say "Hardware en besturingssysteem controleren"
MODEL="$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || echo unknown)"
echo "Model: $MODEL"
if [[ "$MODEL" != *"Raspberry Pi 4"* ]]; then
    [ "$FORCE" = 1 ] || die "dit installatiescript is voor een Raspberry Pi 4 (gevonden: $MODEL). Gebruik --force om toch door te gaan."
    warn "geen Raspberry Pi 4 — toch verder door --force"
fi
[ "$(uname -m)" = "aarch64" ] || die "64-bit besturingssysteem nodig (gevonden: $(uname -m)). Schrijf 'Raspberry Pi OS Lite (64-bit)' naar de kaart."
. /etc/os-release
echo "OS: $PRETTY_NAME"
case "${VERSION_CODENAME:-}" in
    bookworm) ;;
    trixie) warn "Trixie is niet de geteste basis (de ZynScreen-configuratie van Zynthian is bewezen op Bookworm). Toch verder." ;;
    *) [ "$FORCE" = 1 ] || die "Raspberry Pi OS Bookworm verwacht (gevonden: ${VERSION_CODENAME:-?}). Gebruik --force om toch door te gaan." ;;
esac
if [ -d /boot/firmware ]; then BOOT=/boot/firmware; else BOOT=/boot; fi
[ -f "$BOOT/config.txt" ] || die "$BOOT/config.txt niet gevonden"
if systemctl list-unit-files 2>/dev/null | grep -q '^zynthian'; then
    warn "Er staan Zynthian-diensten op dit systeem. FXBox verwacht een schone Raspberry Pi OS."
    ask "Toch doorgaan?" || exit 1
fi

# ------------------------------------------------------------------ 2. packages
say "Pakketten installeren"
export DEBIAN_FRONTEND=noninteractive
echo "jackd2 jackd/tweak_rt_limits boolean true" | debconf-set-selections
apt-get update
apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config \
    jackd2 libjack-jackd2-dev libasound2-dev alsa-utils \
    python3 python3-cairo python3-smbus python3-libgpiod \
    i2c-tools device-tree-compiler fontconfig fonts-dejavu-core
# Interface fonts (fall back to DejaVu if a package is missing in this release).
apt-get install -y --no-install-recommends fonts-inter || warn "fonts-inter niet beschikbaar, DejaVu Sans wordt gebruikt"
apt-get install -y --no-install-recommends fonts-jetbrains-mono || warn "fonts-jetbrains-mono niet beschikbaar, DejaVu Sans Mono wordt gebruikt"
dpkg-query -W -f='${Package} ${Version}\n' jackd2 libjack-jackd2-0 python3-cairo python3-smbus python3-libgpiod \
    fonts-inter fonts-jetbrains-mono 2>/dev/null > "$SRC/install/installed-package-versions.txt" || true

# ------------------------------------------------------------------ 3. build engine
say "Audio-engine bouwen"
BUILD="$SRC/engine/build-pi"
cmake -S "$SRC/engine" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DFXBOX_TUNE_PI4=ON
cmake --build "$BUILD" -j"$(nproc)"
say "Engine-tests uitvoeren op deze Pi"
"$BUILD/test_tempo"
"$BUILD/test_dsp" | tee "$SRC/install/test_dsp_on_pi.txt"

# ------------------------------------------------------------------ 4. install files
say "Installeren in $PREFIX"
id fxbox >/dev/null 2>&1 || useradd --system --home-dir "$DATA" --shell /usr/sbin/nologin --user-group fxbox
for g in audio video i2c gpio spi dialout; do
    getent group "$g" >/dev/null && usermod -aG "$g" fxbox
done
install -d "$PREFIX/bin" "$PREFIX/ui" "$PREFIX/presets" "$PREFIX/install" "$ETC"
install -d -o fxbox -g fxbox -m 0750 "$DATA"
install -m 0755 "$BUILD/fxbox-engine" "$PREFIX/bin/fxbox-engine"
rm -rf "$PREFIX/ui/fxbox"
cp -r "$SRC/ui/fxbox" "$PREFIX/ui/fxbox"
find "$PREFIX/ui" -name '__pycache__' -prune -exec rm -rf {} +
install -m 0644 "$SRC/presets/factory.json" "$PREFIX/presets/factory.json"
install -m 0755 "$SRC/install/fxbox-mixer.sh" "$PREFIX/install/fxbox-mixer.sh"
# Configuration is kept if it exists (it may contain measured values).
[ -f "$ETC/hardware.toml" ] || install -m 0644 "$SRC/config/hardware.toml" "$ETC/hardware.toml"
[ -f "$ETC/audio.conf" ] || install -m 0644 "$SRC/config/audio.conf" "$ETC/audio.conf"
cat > /usr/local/bin/fxbox-hwtest <<'EOF'
#!/bin/sh
PYTHONPATH=/opt/fxbox/ui exec python3 -m fxbox.hwtest "$@"
EOF
chmod 0755 /usr/local/bin/fxbox-hwtest
# /dev/i2c-1 for the MCP23017 (dtparam=i2c_arm=on alone does not load i2c-dev).
echo i2c-dev > /etc/modules-load.d/fxbox-i2c.conf
modprobe i2c-dev 2>/dev/null || true
cat > /etc/security/limits.d/95-fxbox.conf <<'EOF'
@audio - rtprio 95
@audio - memlock unlimited
EOF
install -d /etc/systemd/journald.conf.d
cat > /etc/systemd/journald.conf.d/fxbox.conf <<'EOF'
# Keep logs in RAM to spare the SD card (lost at reboot). Remove this file for persistent logs.
[Journal]
Storage=volatile
RuntimeMaxUse=32M
EOF
say "Interfacetests uitvoeren op deze Pi"
(cd "$SRC/ui" && python3 -m unittest discover -s tests) || die "interfacetests mislukt"

# ------------------------------------------------------------------ 5. boot configuration
say "Scherm-overlays"
for o in piscreen2r-notouch piscreen2r-backlight; do
    dtc -@ -q -I dts -O dtb -o "$BOOT/overlays/$o.dtbo" "$SRC/system/overlays/$o.dts"
    echo "  $BOOT/overlays/$o.dtbo"
done

say "Opstartconfiguratie ($BOOT/config.txt, $BOOT/cmdline.txt)"
cat <<EOF
Wordt ingeschakeld: I2C, SPI, I2S, de UART voor DIN-MIDI (midi-uart0; Bluetooth
naar de mini-UART), de ZynADAC/HiFiBerry-overlay en de ZynScreen-overlays.
De ingebouwde audio en de seriële console gaan uit.
Reservekopieën: $BOOT/config.txt.fxbox-backup en $BOOT/cmdline.txt.fxbox-backup
EOF
if ask "Deze opstartwijzigingen nu toepassen?"; then
    "$SRC/install/boot-config.sh" "$BOOT"
    echo "  klaar"
else
    warn "opstartconfiguratie NIET gewijzigd — scherm, bediening en audio werken pas nadat die is toegepast"
fi

# ------------------------------------------------------------------ 6. services
say "Diensten (automatisch starten)"
for u in fxbox-tune fxbox-mixer fxbox-jack fxbox-engine fxbox-ui; do
    install -m 0644 "$SRC/system/systemd/$u.service" "/etc/systemd/system/$u.service"
done
systemctl daemon-reload
systemctl enable fxbox-tune fxbox-mixer fxbox-jack fxbox-engine fxbox-ui
# The UART belongs to DIN MIDI; Bluetooth is not used.
systemctl mask serial-getty@ttyAMA0.service serial-getty@serial0.service >/dev/null 2>&1 || true
systemctl disable --now hciuart.service bluetooth.service >/dev/null 2>&1 || true

say "Installatie voltooid"
cat <<EOF
Volgende stappen:
  1. sudo reboot
  2. Na het herstarten de hardware controleren:   sudo fxbox-hwtest check
  3. De bediening bevestigen (eerst de interface stoppen):
         sudo systemctl stop fxbox-ui
         sudo fxbox-hwtest controls --write
         sudo systemctl start fxbox-ui
  4. De audiovertraging meten (kabel van UIT L naar IN L): sudo ./install/measure-latency.sh
Logboek van deze installatie: $LOG
EOF
