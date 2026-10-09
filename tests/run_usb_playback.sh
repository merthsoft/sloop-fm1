#!/bin/sh
# Focused firmware USB playback regression. No device access or flashing.
set -eu
CC=${CC:-cc}
OUT=${OUT:-build/usb-playback-tests}
mkdir -p "$OUT"
HALF=$(sed -n 's/^#define HALF_FRAMES \([0-9]*\).*/\1/p' firmware/src/core.h)
for cdc in 0 1 2; do
    $CC -O1 -Wall -Wno-unused-function -DT_CDC=$cdc -DHALF_FRAMES=$HALF \
        tests/usb_playback_test.c -o "$OUT/playback-$cdc"
    "$OUT/playback-$cdc" > "$OUT/playback-$cdc.log"
    tail -n 12 "$OUT/playback-$cdc.log"
done
UAC_DUMP=1 "$OUT/playback-0" | tail -n 2 > "$OUT/plain.desc"
UAC_DUMP=1 "$OUT/playback-2" | tail -n 2 > "$OUT/serial-off.desc"
cmp "$OUT/plain.desc" "$OUT/serial-off.desc"
$CC -O1 -Wall -Wno-unused-function -DUSB_PLAYBACK_LOADER_TEST \
    tests/usb_playback_test.c -o "$OUT/loader"
"$OUT/loader"
echo 'USB playback variants and loader: PASS'
