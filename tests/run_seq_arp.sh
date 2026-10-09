#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host
${CC:-cc} -UNDEBUG -O2 -Wall -Wno-unused-function -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o build/host/seq_arp_test tests/seq_arp_test.c -lm
build/host/seq_arp_test
