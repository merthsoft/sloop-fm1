# SPDX-License-Identifier: GPL-3.0-only
"""Verify name alphabet and fallback independently of current factory spelling."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from pack_presets import pack_names
import lz4blk

def check(names,bits):
    raw,data,mode=pack_names(names)
    assert mode==bits
    if mode:
        n=int.from_bytes(data,'little')>>32
        decoded=bytearray()
        for _ in raw:
            v=n&63;n>>=6
            decoded.append(v|64 if 0<v<32 else v)
    else:decoded=lz4blk.decompress(data)
    assert decoded==raw
for n in range(1,36):
    check([''.join(chr(v) for v in range(32,64))+'ABCDEFGHIJKLMNOPQRSTUVWXYZ']*n,6)
    check(['@lowercase','[KEEP]','backslash\\'],0)
check(['','A','Z','0'],6)
print('preset names: complete six-bit alphabet, terminators, bit boundaries and ASCII fallback PASS')
