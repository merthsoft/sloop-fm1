#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host
${CC:-cc} -O2 -UNDEBUG -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/editor_musical_starters_test tests/editor_musical_starters_test.c -lm
build/host/editor_musical_starters_test
