#!/bin/bash
# Regenerates the C++ effect classes from the Faust sources.
# Only needed when a .dsp file changes; the generated headers are committed,
# so the Raspberry Pi does not need the Faust compiler.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p generated
gen() {
    local src="$1" cls="$2"
    faust -lang cpp -single -ftz 0 -cn "$cls" -a arch_minimal.cpp "$src" -o "generated/${src%.dsp}.hpp"
    echo "generated/${src%.dsp}.hpp  <- $src ($cls)"
}
gen tape_echo.dsp TapeEchoDsp
gen reverb.dsp    ReverbDsp
gen chorus.dsp    ChorusDsp
gen phaser.dsp    PhaserDsp
gen distortion.dsp DistortionDsp
gen saturation.dsp SaturationDsp
gen filter.dsp    FilterDsp
gen compressor.dsp CompressorDsp
{ faust --version || true; } 2>/dev/null | sed -n 1p > generated/FAUST_VERSION.txt
echo "Faust libraries as installed with this compiler (/usr/share/faust)" >> generated/FAUST_VERSION.txt
