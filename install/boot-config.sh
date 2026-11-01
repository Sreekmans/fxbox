#!/bin/bash
# Edits config.txt and cmdline.txt in the given boot directory for FXBox.
# Idempotent: the FXBox block is replaced on every run; backups are made once.
#     boot-config.sh /boot/firmware           apply
#     boot-config.sh /boot/firmware --restore restore the backups
set -euo pipefail
BOOT="${1:?boot directory}"
CFG="$BOOT/config.txt"
CMD="$BOOT/cmdline.txt"

if [ "${2:-}" = "--restore" ]; then
    [ -f "$CFG.fxbox-backup" ] && cp "$CFG.fxbox-backup" "$CFG"
    [ -f "$CMD.fxbox-backup" ] && cp "$CMD.fxbox-backup" "$CMD"
    exit 0
fi

[ -f "$CFG.fxbox-backup" ] || cp "$CFG" "$CFG.fxbox-backup"
[ -f "$CMD.fxbox-backup" ] || cp "$CMD" "$CMD.fxbox-backup"

sed -i '/^# >>> fxbox/,/^# <<< fxbox/d' "$CFG"
sed -i 's/^dtparam=audio=on/#dtparam=audio=on  # disabled by fxbox/' "$CFG"
cat >> "$CFG" <<'EOF'
# >>> fxbox (managed by FXBox install.sh; edits inside this block are overwritten)
[all]
dtparam=i2c_arm=on
dtparam=spi=on
dtparam=i2s=on
dtparam=audio=off
hdmi_force_hotplug=1
disable_splash=1
enable_uart=1
dtoverlay=miniuart-bt
dtoverlay=midi-uart0
dtoverlay=hifiberry-dacplusadcpro
force_eeprom_read=0
dtoverlay=piscreen2r-notouch,rotate=270
dtoverlay=piscreen2r-backlight
# Backlight on from power-up (GPIO 22), also if the backlight overlay is not applied.
gpio=22=op,dh
# <<< fxbox
EOF

# cmdline.txt is one line. Remove the serial console (the UART is DIN MIDI)
# and keep boot messages off the screens.
line="$(head -n1 "$CMD")"
line="$(echo "$line" | sed -E 's/console=(serial0|ttyAMA0|ttyS0),[0-9]+ ?//g')"
for opt in quiet loglevel=3 logo.nologo vt.global_cursor_default=0 consoleblank=0 fbcon=map:0; do
    [[ " $line " == *" $opt "* ]] || line="$line $opt"
done
echo "$line" | tr -s ' ' | sed 's/^ //; s/ $//' > "$CMD"
