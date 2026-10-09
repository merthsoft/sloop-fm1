#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host
${CC:-cc} -O2 -UNDEBUG -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/drum_grooves_test tests/drum_grooves_test.c -lm
build/host/drum_grooves_test build/host
${CC:-cc} -O2 -UNDEBUG -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/editor_drum_grooves_test tests/editor_drum_grooves_test.c -lm
build/host/editor_drum_grooves_test
