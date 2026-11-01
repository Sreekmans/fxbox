#!/bin/bash
# Meet de echte rondgangsvertraging van de audio-interface met jack_iodelay.
# Verbind eerst een korte kabel van UITGANG L naar INGANG L en zet je versterker
# zacht (er klinkt een testtoon). De FXBox-engine staat tijdens de meting uit.
set -euo pipefail
[ "$(id -u)" = 0 ] || { echo "start met sudo"; exit 1; }
command -v jack_iodelay >/dev/null || apt-get install -y --no-install-recommends jackd2 >/dev/null
. /etc/fxbox/audio.conf

echo "fxbox-ui en fxbox-engine worden gestopt tijdens de meting..."
systemctl stop fxbox-ui fxbox-engine
trap 'systemctl start fxbox-engine fxbox-ui' EXIT

# shellcheck disable=SC2024  # the output file is written by root on purpose
sudo -u fxbox env JACK_NO_AUDIO_RESERVATION=1 timeout 15 jack_iodelay > /tmp/fxbox-iodelay.txt 2>&1 &
sleep 1
sudo -u fxbox jack_connect jack_delay:out system:playback_1 || true
sudo -u fxbox jack_connect system:capture_1 jack_delay:in || true
sleep 10
wait || true
echo
echo "Instellingen: ${SAMPLE_RATE} Hz, periode ${PERIOD} x ${NPERIODS}"
tail -n 5 /tmp/fxbox-iodelay.txt
echo
echo "De regel 'total roundtrip latency' is de gemeten vertraging van ingang naar uitgang."
echo "Noteer de waarde in docs/TESTRAPPORT.md (sectie: metingen op het toestel)."
