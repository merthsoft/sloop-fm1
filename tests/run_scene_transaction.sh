#!/bin/sh
set -eu
CC=${CC:-cc}
OUT=${OUT:-build/scene-transaction-tests}
mkdir -p "$OUT"
$CC -std=c11 -O1 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/scene_transaction_test.c -o "$OUT/transaction"
"$OUT/transaction"
$CC -std=c11 -O1 -Wall -Wextra -Wno-unused-function -Werror -fsanitize=undefined -fno-sanitize-recover=all tests/scene_transaction_usb_test.c -o "$OUT/usb-transaction"
"$OUT/usb-transaction"
$CC -std=c11 -O1 -Wall -Wextra -Wno-unused-function -Werror -DT_SCENE=0 -fsanitize=undefined -fno-sanitize-recover=all tests/scene_transaction_usb_test.c -o "$OUT/usb-production"
"$OUT/usb-production"
