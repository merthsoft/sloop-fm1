#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
OUT=${OUT:-build/code-review-sequencing-51/host}
mkdir -p "$OUT"
for t in sequencing_ownership_test sequencing_phone_step_test; do
    ${CC:-cc} -O2 -UNDEBUG -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal "tests/$t.c" -o "$OUT/$t" -lm -lpthread
    "$OUT/$t"
done
