# SPDX-License-Identifier: GPL-3.0-only
"""Pool mergeable target string literals in flash, before final relocation.

No linked addresses or playback decoder. Restore a bounded subset at boot;
keep the remaining literals pooled in flash. Refuse unfamiliar ELF or
relocation layouts rather than guessing. Complete literal objects (including
embedded NULs) retain their bytes and size, not just their first C string.
"""
from pathlib import Path
import hashlib
import json
import struct
import lz4blk
from collections import Counter


def pair_encode(parts, max_codes=159):
    """Shared byte-pair grammar, expanded backwards at startup, never playback.

    Original high bytes remain literals. New codes use only unused values and
    refer to already defined symbols; depth is capped for the 8-byte C stack.
    """
    assert 0 <= max_codes <= 159
    streams = [list(p) for p in parts]
    depth = {v: 0 for p in parts for v in p}
    dictionary = bytearray()
    for code in (list(range(128,256)) + list(range(1,32)))[:max_codes]:
        if code in depth:
            continue
        counts = Counter((p[i], p[i + 1]) for p in streams for i in range(len(p) - 1))
        choices = [(n, pair) for pair, n in counts.items()
                   if n >= 4 and pair != (0, 0) and max(depth[v] for v in pair) < 7]
        if not choices:
            break
        _, pair = min(choices, key=lambda item: (-item[0], item[1]))
        offset = (code - 128 if code >= 128 else code + 127) * 2
        dictionary.extend(bytes(offset + 2 - len(dictionary)))
        dictionary[offset:offset + 2] = bytes(pair)
        depth[code] = max(depth[v] for v in pair) + 1
        for k, p in enumerate(streams):
            out, i = [], 0
            while i < len(p):
                if tuple(p[i:i + 2]) == pair:
                    out.append(code)
                    i += 2
                else:
                    out.append(p[i])
                    i += 1
            streams[k] = out
    return [bytes(p) for p in streams], bytes(dictionary)


def pair_decode(tokens, dictionary):
    out = bytearray()
    for token in tokens:
        stack = [token]
        while stack:
            token = stack.pop()
            i = (token - 128 if token >= 128 else token + 127 if 0 < token < 32 else -1) * 2
            pair = dictionary[i:i + 2] if 0 <= i < len(dictionary) else b''
            if pair and any(pair):
                stack.extend(reversed(pair))
                assert len(stack) <= 8
            else:
                out.append(token)
    return bytes(out)


def pool_object(data, main_limit=6250, pool_limit=2500):
    b = bytearray(data)
    h = struct.unpack_from('<16sHHIIIIIHHHHHH', b)
    assert h[0][:7] == b'\x7fELF\x01\x01\x01' and h[1] == 1 and h[2] == 241  # Pinned JieLi pi32v2 ELF contract.
    assert h[11] == 40 and h[6] + h[12] * 40 <= len(b)
    sections = [struct.unpack_from('<10I', b, h[6] + i * 40) for i in range(h[12])]
    sh = sections[h[13]]
    names = bytes(b[sh[4]:sh[4] + sh[5]])
    candidates = [i for i, s in enumerate(sections)
                  if names[s[0]:].split(b'\0', 1)[0] == b'.rodata.str1.1']
    assert len(candidates) == 1
    index = candidates[0]
    section = sections[index]
    assert section[1] == 1 and section[2] == 0x32 and section[8:10] == (1, 1)
    raw = bytes(b[section[4]:section[4] + section[5]])
    symbols = []
    for s in sections:
        if s[1] != 2:
            continue
        assert s[9] == 16 and s[5] % 16 == 0
        for at in range(s[4], s[4] + s[5], 16):
            _, value, size, kind, _, si = struct.unpack_from('<IIIBBH', b, at)
            if si == index:
                assert kind & 15 == 1 and size and value + size <= len(raw)
                literal = raw[value:value + size]
                assert literal[-1] == 0
                symbols.append((at, literal))
    assert symbols
    for s in sections:
        assert not (s[1] == 9)  # This toolchain uses explicit-addend relocations.
        if s[1] != 4:
            continue
        assert s[9] == 12 and s[5] % 12 == 0 and s[7] != index
        sym = sections[s[6]]
        assert sym[1] == 2
        for at in range(s[4], s[4] + s[5], 12):
            _, info, addend = struct.unpack_from('<IIi', b, at)
            assert info >> 8 < sym[5] // 16
            _, _, _, kind, _, si = struct.unpack_from('<IIIBBH', b, sym[4] + (info >> 8) * 16)
            if si == index:
                assert addend == 0 and info & 255 in (5, 6, 8) and kind & 15 == 1
    roots = []
    pool = bytearray()
    for literal in sorted({literal for _, literal in symbols}, key=lambda x: (-len(x), x)):
        if pool.find(literal) < 0:
            pool.extend(literal)
            roots.append(literal)
    groups = [[], [], []]  # main RAM, pool RAM, flash
    used = [0, 0]
    for root in sorted(roots):
        destination = 2
        for i, limit in enumerate((main_limit, pool_limit)):
            if used[i] + len(root) <= limit:
                used[i] += len(root)
                destination = i
                break
        groups[destination].append(root)
    parts = [b''.join(sorted(group, reverse=True)) for group in groups]
    assert parts[0] and parts[1] and parts[2]
    tokens, dictionary = pair_encode(parts[:2])
    blobs = [lz4blk.compress_best(token) for token in tokens]
    for part, token, blob in zip(parts, tokens, blobs):
        assert lz4blk.decompress(blob) == token
        assert pair_decode(token, dictionary) == part
    # Keep all existing section numbers. Append two zero-initialized destinations.
    extra = len(sections)
    sections = [list(s) for s in sections]
    for i, label in enumerate((b'.boot_literal_main\0', b'.boot_literal_pool\0')):
        sections.append([len(names), 8, 3, 0, 0, len(parts[i]), 0, 0, 1, 0])
        names += label
    for at, literal in symbols:
        for i, part in enumerate(parts):
            value = part.find(literal)
            if value >= 0:
                assert part[value:value + len(literal)] == literal
                struct.pack_into('<I', b, at + 4, value)
                struct.pack_into('<H', b, at + 14, extra + i if i < 2 else index)
                break
        else:
            raise AssertionError('lost literal')
    flash = parts[2]
    b[section[4]:section[4] + len(raw)] = flash + bytes(len(raw) - len(flash))
    sections[index][5] = len(flash)
    sections[index][2], sections[index][9] = 2, 0  # Already pooled; keep explicit symbol offsets.
    sections[h[13]][4] = len(b)
    sections[h[13]][5] = len(names)
    b.extend(names)
    b.extend(bytes((-len(b)) % 4))
    h = list(h)
    h[6], h[12] = len(b), len(sections)
    for sh in sections:
        b.extend(struct.pack('<10I', *sh))
    struct.pack_into('<16sHHIIIIIHHHHHH', b, 0, *h)
    report = dict(original=len(raw), pooled=len(pool), symbols=len(symbols),
                  ram=list(map(len, parts[:2])), flash=len(flash),
                  packed=list(map(len, blobs)),
                  tokens=list(map(len, tokens)), dictionary=len(dictionary),
                  payload_saved=len(raw) - len(flash) - sum(map(len, blobs)) - len(dictionary),
                  sha256=hashlib.sha256(b''.join(parts)).hexdigest())
    return bytes(b), report, parts, blobs


def pack(path, gen):
    path, gen = Path(path), Path(gen)
    raw = path.read_bytes()
    result, report, parts, blobs = pool_object(raw)
    tokens, dictionary = pair_encode(parts[:2])
    path.with_name('felucca.literals-raw.o').write_bytes(raw)
    path.write_bytes(result)
    (gen / 'pooled_literals_report.json').write_text(json.dumps(report, indent=2) + '\n')
    code = ['#include <stdint.h>']
    for name, part, blob in zip(('main','pool'), parts, blobs):
        code.append('const uint8_t boot_literal_'+name+'_blob[]={'+','.join(map(str,blob))+'};')
    code.append('const uint8_t boot_literal_pairs[]={'+','.join(map(str,dictionary or b'\0\0'))+'};')
    code.append('const uint32_t boot_literal_sizes[]={'+','.join(map(str,
        [len(blobs[0]),len(tokens[0]),len(parts[0]),
         len(blobs[1]),len(tokens[1]),len(parts[1]),len(dictionary)//2]))+'};')
    (gen / 'boot_literals_blob.c').write_text('\n'.join(code)+'\n')
    for name, part in zip(('main','pool','flash'), parts):
        (gen / ('literal_'+name+'_oracle.bin')).write_bytes(part)
    print('Boot/flash literals:', report, flush=True)
