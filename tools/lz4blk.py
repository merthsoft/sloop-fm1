# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Minimal LZ4 *block* format (no frame), written for the Felucca loader
wrapper: greedy hash-chain-free compressor and a decoder for self-checks.
Spec: https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md
(min match 4, last 5 bytes literals, last match starts >= 12 bytes before the end)."""


def _len_bytes(n):
    out = bytearray()
    while n >= 255:
        out.append(255)
        n -= 255
    out.append(n)
    return out


def compress(src: bytes) -> bytes:
    n, out, table = len(src), bytearray(), {}
    anchor, i = 0, 0
    limit = n - 12                                  # matches must start before this
    while i < limit:
        key = src[i:i + 4]
        cand = table.get(key)
        table[key] = i
        if cand is None or i - cand > 65535:
            i += 1
            continue
        mlen = 4                                    # extend; leave the last 5 bytes as literals
        while i + mlen < n - 5 and src[cand + mlen] == src[i + mlen]:
            mlen += 1
        lit = src[anchor:i]
        tok = (min(len(lit), 15) << 4) | min(mlen - 4, 15)
        out.append(tok)
        if len(lit) >= 15:
            out += _len_bytes(len(lit) - 15)
        out += lit
        off = i - cand
        out += bytes((off & 0xFF, off >> 8))
        if mlen - 4 >= 15:
            out += _len_bytes(mlen - 4 - 15)
        i += mlen
        anchor = i
    lit = src[anchor:]                              # final literal run
    out.append(min(len(lit), 15) << 4)
    if len(lit) >= 15:
        out += _len_bytes(len(lit) - 15)
    out += lit
    return bytes(out)


def decompress(blk: bytes, prefix: bytes = b"") -> bytes:
    out, i = bytearray(prefix), 0
    while i < len(blk):
        tok = blk[i]
        i += 1
        ll = tok >> 4
        if ll == 15:
            while True:
                b = blk[i]
                i += 1
                ll += b
                if b != 255:
                    break
        out += blk[i:i + ll]
        i += ll
        if i >= len(blk):
            break
        off = blk[i] | blk[i + 1] << 8
        i += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = blk[i]
                i += 1
                ml += b
                if b != 255:
                    break
        ml += 4
        for _ in range(ml):
            out.append(out[-off])
    return bytes(out[len(prefix):])


def _compress_lazy(src: bytes) -> bytes:
    """Build-time hash-chain/lazy search; same block format and decoder.

    Index every input position, including positions skipped by a match. Bound
    the search to 64 recent candidates and retain the fast result if smaller.
    Intended for small cold-start metadata, not a device-side compressor.
    """
    from bisect import bisect_left
    chains = {}
    n = len(src)
    for i in range(max(0, n - 11)):
        chains.setdefault(src[i:i + 4], []).append(i)

    def match(i):
        positions = chains.get(src[i:i + 4], ())
        stop = bisect_left(positions, i)
        best, offset = 0, 0
        for cand in reversed(positions[max(0, stop - 64):stop]):
            if i - cand > 65535:
                break
            length = 4
            while i + length < n - 5 and src[cand + length] == src[i + length]:
                length += 1
            if length > best:
                best, offset = length, i - cand
                if i + length == n - 5:
                    break
        return best, offset

    out = bytearray()
    anchor = i = 0
    while i < n - 12:
        length, offset = match(i)
        if not length or (i + 1 < n - 12 and match(i + 1)[0] > length + 1):
            i += 1
            continue
        literals = src[anchor:i]
        out.append((min(len(literals), 15) << 4) | min(length - 4, 15))
        if len(literals) >= 15:
            out += _len_bytes(len(literals) - 15)
        out += literals
        out += offset.to_bytes(2, 'little')
        if length >= 19:
            out += _len_bytes(length - 19)
        i += length
        anchor = i
    literals = src[anchor:]
    out.append(min(len(literals), 15) << 4)
    if len(literals) >= 15:
        out += _len_bytes(len(literals) - 15)
    out += literals
    fast = compress(src)
    return bytes(out) if len(out) < len(fast) else fast



def compress_best(src: bytes) -> bytes:
    """Find a minimum-byte block parse for small cold-start metadata.

    Match offsets have equal encoded cost, so the longest legal match at each
    position covers all shorter choices. Backward dynamic programming compares
    every legal match length and literal run, including extension-byte costs.
    Bound quadratic build-time work to 8 KiB; retain the lazy/fast fallback.
    """
    from bisect import bisect_left
    n=len(src)
    if n>8192: return _compress_lazy(src)
    chains={}
    for i in range(max(0,n-11)):
        chains.setdefault(src[i:i+4],[]).append(i)
    cost=[0]*(n+1); choice=[None]*(n+1)
    matches=[None]*n
    def ext(v): return 0 if v<15 else 1+(v-15)//255
    for i in range(n,-1,-1):
        remain=n-i
        cost[i]=1+remain+ext(remain)
        if i<n-12:
            positions=chains.get(src[i:i+4],())
            stop=bisect_left(positions,i)
            longest=0;offset=0
            for cand in reversed(positions[:stop]):
                if i-cand>65535:break
                length=4
                while i+length<n-5 and src[cand+length]==src[i+length]:length+=1
                if length>longest:longest,offset=length,i-cand
                if i+length==n-5:break
            if longest:
                length=min(range(4,longest+1),key=lambda k:ext(k-4)+cost[i+k])
                matches[i]=(3+ext(length-4)+cost[i+length],length,offset)
        for j in range(i,n-12):
            m=matches[j]
            if m:
                value=j-i+ext(j-i)+m[0]
                if value<cost[i]:cost[i]=value;choice[i]=(j,m[1],m[2])
    out=bytearray();i=0
    while choice[i] is not None:
        j,length,offset=choice[i];ll=j-i
        out.append((min(ll,15)<<4)|min(length-4,15))
        if ll>=15:out.extend(_len_bytes(ll-15))
        out.extend(src[i:j]);out.extend(offset.to_bytes(2,'little'))
        if length>=19:out.extend(_len_bytes(length-19))
        i=j+length
    ll=n-i;out.append(min(ll,15)<<4)
    if ll>=15:out.extend(_len_bytes(ll-15))
    out.extend(src[i:])
    previous=_compress_lazy(src)
    result=bytes(out) if len(out)<len(previous) else previous
    assert decompress(result)==src
    return result
