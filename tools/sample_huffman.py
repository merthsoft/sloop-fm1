# SPDX-License-Identifier: GPL-3.0-only
"""Lossless, independently addressable 128-byte factory ADPCM blocks."""
from collections import Counter
import heapq

BLOCK = 128

def header(blob):
    counts = Counter(blob)
    heap = [(max(1, counts[i]), i, i) for i in range(256)]
    heapq.heapify(heap)
    serial = 256
    while len(heap) > 1:
        a, _, x = heapq.heappop(heap)
        b, _, y = heapq.heappop(heap)
        heapq.heappush(heap, (a+b, serial, (x, y)))
        serial += 1
    tree, codes = [], {}
    def walk(node, bits=0, length=0):
        if isinstance(node, int):
            codes[node] = (bits, length)
            return 256+node
        idx = len(tree)
        tree.append(None)
        tree[idx] = (walk(node[0], bits << 1, length+1),
                     walk(node[1], bits << 1 | 1, length+1))
        return idx
    assert walk(heap[0][2]) == 0
    fast = [0]*256
    for symbol, (bits, length) in codes.items():
        if length <= 8:
            for i in range(1 << (8-length)):
                fast[bits << (8-length) | i] = 0x8000 | length << 8 | symbol
    data, offsets = bytearray(), bytearray()
    for at in range(0, len(blob), BLOCK):
        offsets.extend(len(data).to_bytes(3, 'little'))
        bits = length = 0
        for symbol in blob[at:at+BLOCK]:
            code, n = codes[symbol]
            bits = bits << n | code
            length += n
            while length >= 8:
                length -= 8
                data.append(bits >> length & 255)
                bits &= (1 << length)-1
        if length:
            data.append(bits << (8-length))
    data.extend(b'\0\0')  # eight-bit lookahead, including the final block
    def array(ctype, name, values):
        return [f'static const {ctype} {name}[] = {{',
                *('    '+','.join(map(str, values[i:i+24]))+',' for i in range(0, len(values), 24)), '};']
    out = [f'#define SMP_HUFF_RAW_N {len(blob)}u', f'#define SMP_HUFF_BLOCK {BLOCK}u']
    out += array('uint8_t', 'SMP_DATA', data)
    out += array('uint8_t', 'SMP_HUFF_OFF', offsets)
    out += array('uint16_t', 'SMP_HUFF_FAST', fast)
    out += ['static const uint16_t SMP_HUFF_TREE[][2] = {',
            *(f'    {{{a},{b}}},' for a, b in tree), '};']
    return out
