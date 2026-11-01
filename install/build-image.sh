#!/bin/bash
# Bouwt een flashbaar SD-kaart-image met FXBox voorgeïnstalleerd.
#
# Draait op een Linux-pc (Debian/Ubuntu, x86-64 of arm64) met internet.
# Basis: Raspberry Pi OS Lite 64-bit (Bookworm). Het FXBox-installatiescript
# wordt in een ARM64-chroot uitgevoerd (qemu-user-static), inclusief de tests.
#
#   sudo ./install/build-image.sh --user steven --hostname fxbox
#   sudo ./install/build-image.sh --user steven --base ~/Downloads/2025-xx-xx-raspios-bookworm-arm64-lite.img.xz --xz
#
# Opties:
#   --user NAAM          gebruiker voor SSH-onderhoud (verplicht)
#   --password WACHTW    wachtwoord (anders wordt erom gevraagd)
#   --hostname NAAM      hostnaam (standaard: fxbox)
#   --base BESTAND|URL   basis-image (.img of .img.xz). Standaard: de nieuwste
#                        Raspberry Pi OS Lite 64-bit "oldstable" (= Bookworm)
#   --out MAP            uitvoermap (standaard: ./image-out)
#   --xz                 het resultaat ook comprimeren (.img.xz)
#   --allow-trixie       ook een Trixie-basis accepteren (niet de geteste basis)
#   --skip-install       alleen image voorbereiden (voor foutzoeken)
#
# Het script schrijft NOOIT naar een SD-kaart of ander blokapparaat. Het maakt
# alleen een bestand; dat zet je zelf op de kaart (Raspberry Pi Imager → "Use custom").
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(sed -n 's/^__version__ = "\(.*\)"/\1/p' "$SRC/ui/fxbox/__init__.py")"
DEFAULT_BASE="https://downloads.raspberrypi.com/raspios_oldstable_lite_arm64_latest"
EXTRA_MB=1536

USER_NAME=""; PASSWORD=""; HOSTNAME_NEW="fxbox"; BASE="$DEFAULT_BASE"
OUT="$PWD/image-out"; XZ=0; ALLOW_TRIXIE=0; SKIP_INSTALL=0
while [ $# -gt 0 ]; do
    case "$1" in
        --user) USER_NAME="$2"; shift 2 ;;
        --password) PASSWORD="$2"; shift 2 ;;
        --hostname) HOSTNAME_NEW="$2"; shift 2 ;;
        --base) BASE="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --xz) XZ=1; shift ;;
        --allow-trixie) ALLOW_TRIXIE=1; shift ;;
        --skip-install) SKIP_INSTALL=1; shift ;;
        -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
        *) echo "Onbekende optie: $1"; exit 2 ;;
    esac
done

say()  { echo -e "\n==> $*"; }
die()  { echo "FOUT: $*" >&2; exit 1; }

[ "$(id -u)" = 0 ] || die "start met sudo"
[ -n "$USER_NAME" ] || die "geef een gebruiker op met --user (nodig voor SSH-onderhoud)"
[[ "$USER_NAME" =~ ^[a-z][a-z0-9_-]{0,31}$ ]] || die "ongeldige gebruikersnaam: $USER_NAME"
[[ "$HOSTNAME_NEW" =~ ^[a-zA-Z0-9-]{1,63}$ ]] || die "ongeldige hostnaam: $HOSTNAME_NEW"
case "$OUT" in /dev/*) die "uitvoer moet een map zijn, geen apparaat" ;; esac

# ------------------------------------------------------------- benodigdheden
say "Benodigdheden controleren"
missing=()
for c in curl xz parted losetup e2fsck resize2fs openssl sha256sum rsync; do
    command -v "$c" >/dev/null || missing+=("$c")
done
if [ "$(uname -m)" != "aarch64" ]; then
    if [ ! -e /proc/sys/fs/binfmt_misc/qemu-aarch64 ]; then
        missing+=("qemu-user-static (binfmt voor aarch64)")
    fi
fi
if [ ${#missing[@]} -gt 0 ]; then
    echo "Ontbreekt: ${missing[*]}"
    echo "Debian/Ubuntu: sudo apt install curl xz-utils parted e2fsprogs openssl rsync qemu-user-static binfmt-support"
    exit 1
fi

if [ -z "$PASSWORD" ]; then
    read -r -s -p "Wachtwoord voor $USER_NAME: " PASSWORD; echo
    read -r -s -p "Nogmaals: " p2; echo
    [ "$PASSWORD" = "$p2" ] || die "wachtwoorden verschillen"
fi
[ ${#PASSWORD} -ge 8 ] || die "wachtwoord: minstens 8 tekens"
PWHASH="$(echo "$PASSWORD" | openssl passwd -6 -stdin)"

mkdir -p "$OUT"
WORK="$(mktemp -d "$OUT/.work.XXXXXX")"
MNT="$WORK/root"
LOOP=""
LOOP_BOOT=""
RESOLV_LINK=""

cleanup() {
    set +e
    if mountpoint -q "$MNT" 2>/dev/null; then
        for m in dev/pts dev proc sys run boot/firmware; do
            mountpoint -q "$MNT/$m" && umount -l "$MNT/$m"
        done
        umount -l "$MNT"
    fi
    [ -n "$LOOP" ] && losetup -d "$LOOP"
    [ -n "$LOOP_BOOT" ] && losetup -d "$LOOP_BOOT"
    rm -rf "$WORK"
}
trap cleanup EXIT

avail_mb=$(df -Pm "$OUT" | awk 'NR==2 {print $4}')
[ "$avail_mb" -ge 9000 ] || die "minstens 9 GB vrije ruimte nodig in $OUT (nu ${avail_mb} MB)"

# ------------------------------------------------------------- basis-image
say "Basis-image ophalen"
if [[ "$BASE" =~ ^https?:// ]]; then
    url="$(curl -sSIL -o /dev/null -w '%{url_effective}' "$BASE")"
    echo "Bron: $url"
    curl -fL --retry 3 -o "$WORK/base.img.xz" "$url"
    if curl -fsSL -o "$WORK/base.sha256" "$url.sha256"; then
        want="$(awk '{print $1}' "$WORK/base.sha256")"
        got="$(sha256sum "$WORK/base.img.xz" | awk '{print $1}')"
        [ "$want" = "$got" ] || die "SHA-256 klopt niet voor $url"
        echo "SHA-256 gecontroleerd"
    else
        echo "Let op: geen .sha256 gevonden naast de download; niet gecontroleerd."
    fi
    BASEFILE="$WORK/base.img.xz"
else
    [ -f "$BASE" ] || die "basis-image niet gevonden: $BASE"
    BASEFILE="$BASE"
fi

IMG="$WORK/fxbox.img"
case "$BASEFILE" in
    *.xz) xz -dc "$BASEFILE" > "$IMG" ;;
    *.img) cp "$BASEFILE" "$IMG" ;;
    *) die "basis-image moet .img of .img.xz zijn" ;;
esac

# ------------------------------------------------------------- vergroten en mounten
say "Image vergroten met $EXTRA_MB MB (ruimte voor pakketten en build)"
truncate -s +"${EXTRA_MB}M" "$IMG"
parted -s "$IMG" resizepart 2 100%
# Partities via offsets koppelen: werkt ook zonder udev (containers, WSL).
part_field() {   # part_field NR KOLOM  (bytes uit "parted -m unit B print")
    parted -m -s "$IMG" unit B print | awk -F: -v n="$1" -v c="$2" '$1==n {gsub("B","",$c); print $c}'
}
P1_START="$(part_field 1 2)"; P1_SIZE="$(part_field 1 4)"
P2_START="$(part_field 2 2)"; P2_SIZE="$(part_field 2 4)"
[ -n "$P1_START" ] && [ -n "$P2_START" ] || die "partitietabel van het basis-image niet herkend"
LOOP_BOOT="$(losetup -f --show --offset "$P1_START" --sizelimit "$P1_SIZE" "$IMG")"
LOOP="$(losetup -f --show --offset "$P2_START" --sizelimit "$P2_SIZE" "$IMG")"
e2fsck -fy "$LOOP" >/dev/null || true
resize2fs "$LOOP" >/dev/null

mkdir -p "$MNT"
mount "$LOOP" "$MNT"
mkdir -p "$MNT/boot/firmware"
mount "$LOOP_BOOT" "$MNT/boot/firmware"

. "$MNT/etc/os-release"
echo "Basis: $PRETTY_NAME"
case "$VERSION_CODENAME" in
    bookworm) ;;
    trixie) [ "$ALLOW_TRIXIE" = 1 ] || die "basis is Trixie; FXBox is getest op Bookworm. Gebruik --allow-trixie om toch te bouwen, of geef een Bookworm-image op met --base." ;;
    *) die "onverwachte basis: $VERSION_CODENAME" ;;
esac
[ -f "$MNT/usr/bin/apt-get" ] || die "geen Raspberry Pi OS-rootfs gevonden"

# ------------------------------------------------------------- chroot voorbereiden
say "Chroot voorbereiden"
for m in proc sys dev dev/pts run; do mkdir -p "$MNT/$m"; done
mount -t proc proc "$MNT/proc"
mount -t sysfs sys "$MNT/sys"
mount --bind /dev "$MNT/dev"
mount --bind /dev/pts "$MNT/dev/pts"
mount -t tmpfs tmpfs "$MNT/run"
# DNS van de host gebruiken tijdens de build; het origineel wordt nadien teruggezet.
if [ -L "$MNT/etc/resolv.conf" ]; then
    RESOLV_LINK="$(readlink "$MNT/etc/resolv.conf")"
    rm -f "$MNT/etc/resolv.conf"
elif [ -e "$MNT/etc/resolv.conf" ]; then
    mv "$MNT/etc/resolv.conf" "$MNT/etc/resolv.conf.fxbox-orig"
fi
cp -L /etc/resolv.conf "$MNT/etc/resolv.conf"
# Geen diensten starten tijdens de installatie in de chroot.
mkdir -p "$MNT/usr/sbin"
printf '#!/bin/sh\nexit 101\n' > "$MNT/usr/sbin/policy-rc.d"
chmod +x "$MNT/usr/sbin/policy-rc.d"
[ -f "$MNT/etc/ld.so.preload" ] && mv "$MNT/etc/ld.so.preload" "$MNT/etc/ld.so.preload.fxbox"

# ------------------------------------------------------------- FXBox installeren
say "FXBox-bronnen kopiëren naar /opt/fxbox-src"
rsync -a --delete --exclude 'engine/build*' --exclude '__pycache__' --exclude 'image-out' "$SRC/" "$MNT/opt/fxbox-src/"

if [ "$SKIP_INSTALL" = 0 ]; then
    say "FXBox installeren in de chroot (onder emulatie kan dit lang duren)"
    chroot "$MNT" /bin/bash -c "cd /opt/fxbox-src && ./install/install.sh --yes --force"
    chroot "$MNT" /bin/bash -c "apt-get clean"
    cp "$MNT/var/log/fxbox-install.log" "$OUT/fxbox-install-$VERSION.log" 2>/dev/null || true
fi

# ------------------------------------------------------------- gebruiker, SSH, hostnaam
say "Gebruiker $USER_NAME, SSH en hostnaam $HOSTNAME_NEW"
# userconf.txt: Raspberry Pi OS maakt deze gebruiker bij de eerste start aan.
echo "$USER_NAME:$PWHASH" > "$MNT/boot/firmware/userconf.txt"
touch "$MNT/boot/firmware/ssh"
echo "$HOSTNAME_NEW" > "$MNT/etc/hostname"
sed -i -E "s/^127\.0\.1\.1.*/127.0.1.1\t$HOSTNAME_NEW/" "$MNT/etc/hosts"
grep -q "^127.0.1.1" "$MNT/etc/hosts" || printf '127.0.1.1\t%s\n' "$HOSTNAME_NEW" >> "$MNT/etc/hosts"

# ------------------------------------------------------------- opruimen
say "Opruimen"
rm -f "$MNT/usr/sbin/policy-rc.d"
[ -f "$MNT/etc/ld.so.preload.fxbox" ] && mv "$MNT/etc/ld.so.preload.fxbox" "$MNT/etc/ld.so.preload"
rm -f "$MNT/etc/resolv.conf"
if [ -n "${RESOLV_LINK:-}" ]; then
    ln -s "$RESOLV_LINK" "$MNT/etc/resolv.conf"
elif [ -e "$MNT/etc/resolv.conf.fxbox-orig" ]; then
    mv "$MNT/etc/resolv.conf.fxbox-orig" "$MNT/etc/resolv.conf"
fi
rm -rf "$MNT/opt/fxbox-src/engine/build-pi"
for m in dev/pts dev proc sys run boot/firmware; do umount "$MNT/$m"; done
umount "$MNT"
e2fsck -fy "$LOOP" >/dev/null || true
losetup -d "$LOOP"; LOOP=""
losetup -d "$LOOP_BOOT"; LOOP_BOOT=""

NAME="fxbox-$VERSION-$(date +%Y%m%d)"
mv "$IMG" "$OUT/$NAME.img"
(cd "$OUT" && sha256sum "$NAME.img" > "$NAME.img.sha256")
if [ "$XZ" = 1 ]; then
    say "Comprimeren"
    xz -T0 -6 -k "$OUT/$NAME.img"
    (cd "$OUT" && sha256sum "$NAME.img.xz" > "$NAME.img.xz.sha256")
fi

say "Klaar: $OUT/$NAME.img"
cat <<EOF
Op de kaart zetten (de kaart wordt volledig gewist):
  Raspberry Pi Imager → Choose OS → Use custom → $NAME.img
  (Sla de "OS customisation" van Imager over: gebruiker, SSH en hostnaam zitten al in het image.)
Bij de eerste start vergroot Raspberry Pi OS het bestandssysteem en herstart het één keer;
daarna verschijnt de effectinterface. Daarna eenmalig via SSH ($USER_NAME@$HOSTNAME_NEW.local):
  sudo fxbox-hwtest check
  sudo systemctl stop fxbox-ui && sudo fxbox-hwtest controls --write && sudo systemctl start fxbox-ui
EOF
