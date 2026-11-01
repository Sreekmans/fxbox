#!/bin/bash
# Removes FXBox autostart and restores the original boot configuration.
# User presets in /var/lib/fxbox are kept unless --purge is given.
set -euo pipefail
[ "$(id -u)" = 0 ] || { echo "run with sudo"; exit 1; }
if [ -d /boot/firmware ]; then BOOT=/boot/firmware; else BOOT=/boot; fi
systemctl disable --now fxbox-ui fxbox-engine fxbox-jack fxbox-mixer fxbox-tune 2>/dev/null || true
rm -f /etc/systemd/system/fxbox-*.service
systemctl daemon-reload
systemctl unmask serial-getty@ttyAMA0.service serial-getty@serial0.service 2>/dev/null || true
[ -f "$BOOT/config.txt.fxbox-backup" ] && cp "$BOOT/config.txt.fxbox-backup" "$BOOT/config.txt"
[ -f "$BOOT/cmdline.txt.fxbox-backup" ] && cp "$BOOT/cmdline.txt.fxbox-backup" "$BOOT/cmdline.txt"
rm -rf /opt/fxbox /usr/local/bin/fxbox-hwtest /etc/security/limits.d/95-fxbox.conf /etc/systemd/journald.conf.d/fxbox.conf
if [ "${1:-}" = "--purge" ]; then rm -rf /var/lib/fxbox /etc/fxbox; fi
echo "FXBox removed. Reboot to restore the original boot configuration."
