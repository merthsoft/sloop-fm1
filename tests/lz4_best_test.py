# SPDX-License-Identifier: GPL-3.0-only
"""Build-time compressor contract: exact bytes, bounded offsets, LZ4 tail rules."""
import random
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from lz4blk import compress, compress_best, decompress


def check(raw):
    packed = compress_best(raw)
    assert packed == compress_best(raw), 'non-deterministic output'
    assert len(packed) <= len(compress(raw)), 'worse than fast fallback'
    assert decompress(packed) == raw
    i = at = 0
    while i < len(packed):
        token = packed[i]; i += 1
        literal = token >> 4
        if literal == 15:
            while True:
                b = packed[i]; i += 1; literal += b
                if b < 255: break
        i += literal; at += literal
        if i == len(packed):
            assert literal >= min(5, len(raw))
            break
        assert at < len(raw) - 12, 'last match starts too late'
        offset = int.from_bytes(packed[i:i+2], 'little'); i += 2
        assert 0 < offset <= min(65535, at)
        length = token & 15
        if length == 15:
            while True:
                b = packed[i]; i += 1; length += b
                if b < 255: break
        at += length + 4
    assert at == len(raw)

witness = b'ABCDlonger pattern blah|ABCDshort|ABCDlonger pattern blah|tail.'
assert len(compress_best(witness)) < len(compress(witness))
check(witness)
rng = random.Random(66)
for n in list(range(80)) + [255, 256, 270, 1024, 4096]:
    for alphabet in [1, 2, 7, 256]:
        check(bytes(rng.randrange(alphabet) for _ in range(n)))
# Window edge, overlapping copies, embedded repeats and match-length extensions.
check(bytes(range(256)) * 270)
for n in [18, 19, 273, 274, 529, 4096]:
    check(b'head' + b'A' * n + b'tail!')
for _ in range(100):
    chunks = [rng.randbytes(rng.randrange(4, 32)) for _ in range(8)]
    check(b''.join(rng.choice(chunks) for _ in range(40)))
print('LZ4 best: deterministic round trips, fast fallback, offsets/tails and boundaries PASS')
