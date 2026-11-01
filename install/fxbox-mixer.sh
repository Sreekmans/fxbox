#!/bin/bash
# Applies the sound card mixer settings once, then restores them on every boot.
# The input routing is the one Zynthian applies for ZynADAC / HiFiBerry DAC+ ADC Pro
# (zynthian-sys sbin/fix_alsamixer_settings.sh).
CARD="$(. /etc/fxbox/audio.conf 2>/dev/null; echo "${ALSA_CARD:-sndrpihifiberry}")"
STATE=/var/lib/fxbox/asound.state

# Wait briefly for the card (driver may still be probing).
for _ in $(seq 1 20); do
    grep -q "$CARD" /proc/asound/cards 2>/dev/null && break
    sleep 0.5
done
if ! grep -q "$CARD" /proc/asound/cards 2>/dev/null; then
    echo "fxbox-mixer: card $CARD not present"
    exit 0          # not fatal: the engine reports the missing card on screen
fi

if [ -f "$STATE" ]; then
    alsactl --file "$STATE" restore "$CARD" && exit 0
fi

echo "fxbox-mixer: first-time mixer setup for $CARD"
amixer -c "$CARD" scontrols > /var/lib/fxbox/mixer-controls.txt 2>&1 || true
amixer -c "$CARD" sset 'Auto Mute' mute >/dev/null 2>&1 || true
amixer -c "$CARD" sset 'Auto Mute Mono' mute >/dev/null 2>&1 || true
amixer -c "$CARD" sset 'Deemphasis' off >/dev/null 2>&1 || true
amixer -c "$CARD" sset 'ADC Left Input' 'VINL2[SE] + VINL1[SE]' >/dev/null 2>&1 || true
amixer -c "$CARD" sset 'ADC Right Input' 'VINR2[SE] + VINR1[SE]' >/dev/null 2>&1 || true
# Unity gain on the DAC digital volume and the ADC PGA (control names differ
# between kernel versions; whichever exists is set).
for c in 'Digital' 'Digital Left' 'Digital Right'; do
    amixer -c "$CARD" sset "$c" 0dB >/dev/null 2>&1 || true
done
for c in 'PGA Gain Left' 'PGA Gain Right'; do
    amixer -c "$CARD" sset "$c" 0dB >/dev/null 2>&1 || true
done
alsactl --file "$STATE" store "$CARD" || true
exit 0
