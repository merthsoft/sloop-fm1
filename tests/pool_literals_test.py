# SPDX-License-Identifier: GPL-3.0-only
"""Object-format refusal tests and actual target literal/boot restoration proof."""
from pathlib import Path
import struct
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from pool_literals import pool_object, pair_encode, pair_decode


def elf(data):
    h = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    sections = [struct.unpack_from('<10I', data, h[6] + i * 40) for i in range(h[12])]
    def contents(s):
        return data[s[4]:s[4] + s[5]]
    symbols = []
    for s in sections:
        if s[1] != 2:
            continue
        names = contents(sections[s[6]])
        for at in range(s[4], s[4] + s[5], 16):
            n, value, size, kind, _, index = struct.unpack_from('<IIIBBH', data, at)
            symbols.append(dict(name=names[n:].split(b'\0', 1)[0].decode(), value=value,
                                size=size, kind=kind, index=index))
    return sections, symbols, contents


def fixture(addend=0, relocation=5, flags=0x32):
    literals = [b'LONG LABEL\0', b'LABEL\0', b'EMBED\0DED\0',
                b'ANOTHER LONG LABEL\0', b'A\0', b'LAST UNRELATED PHRASE\0']
    raw = b''.join(literals)
    sn = b'\0.rodata.str1.1\0.symtab\0.strtab\0.shstrtab\0.rela.text\0'
    names = b'\0' + b''.join(('literal%d' % i).encode() + b'\0' for i in range(len(literals)))
    symbols = bytes(16)
    offset = 0
    for i, literal in enumerate(literals):
        symbols += struct.pack('<IIIBBH', names.index(('literal%d' % i).encode()), offset,
                               len(literal), 1, 0, 1)
        offset += len(literal)
    data = bytearray(52)
    sections = [[0] * 10]
    for name, kind, fl, payload, link, info, entsize in [
        (b'.rodata.str1.1', 1, flags, raw, 0, 0, 1),
        (b'.symtab', 2, 0, symbols, 3, 1, 16),
        (b'.strtab', 3, 0, names, 0, 0, 0),
        (b'.shstrtab', 3, 0, sn, 0, 0, 0),
        (b'.rela.text', 4, 0, struct.pack('<IIi', 0, 1 << 8 | relocation, addend), 2, 0, 12),
    ]:
        sections.append([sn.index(name), kind, fl, 0, len(data), len(payload), link, info, 1, entsize])
        data.extend(payload)
    start = len(data)
    for s in sections:
        data.extend(struct.pack('<10I', *s))
    struct.pack_into('<16sHHIIIIIHHHHHH', data, 0, b'\x7fELF\x01\x01\x01' + bytes(9),
                     1, 241, 1, 0, 0, start, 0, 52, 0, 0, 40, len(sections), 4)
    return bytes(data)


def compare_objects(before, after, parts):
    ss, sy, content = elf(before)
    ds, dy, dest = elf(after)
    mapped = {s['name']: s for s in dy}
    checked = 0
    for s in sy:
        if s['index'] >= len(ss):
            continue
        if s['kind'] & 15 != 1:
            continue
        # Actual target section index is found through its mergeable section flags.
        if ss[s['index']][2] != 0x32:
            continue
        d = mapped[s['name']]
        raw = content(ss[s['index']])[s['value']:s['value'] + s['size']]
        target = (dest(ds[d['index']]) if ds[d['index']][1] != 8
                  else parts[d['index'] - len(ss)])
        assert raw == target[d['value']:d['value'] + d['size']], s['name']
        assert s['size'] == d['size']
        checked += 1
    assert checked
    return checked


class LiteralTests(unittest.TestCase):
    def test_complete_objects_and_embedded_zero(self):
        before = fixture()
        after, report, parts, _ = pool_object(before, 25, 18)
        self.assertGreater(report['payload_saved'], 0)
        self.assertEqual(compare_objects(before, after, parts), 6)
        self.assertEqual(pool_object(before, 25, 18)[0], after)

    def test_grammar_roundtrip(self):
        cases = [[b'NOTE TRACK SYNC\0' * 100, b'TRACK NOTE SYNC\0' * 80],
                 [bytes(range(256)) * 3, bytes(range(255, -1, -1))],
                 [b'\0' * 1000, b'AB' * 500],
                 [b'A' * 1000, b'\xc2\xb0\0' * 30]]
        for parts in cases:
            encoded, dictionary = pair_encode(parts)
            self.assertEqual([pair_decode(p, dictionary) for p in encoded], parts)
            self.assertEqual(pair_encode(parts), (encoded, dictionary))

    def test_refuses_unknown_contracts(self):
        for args in [dict(addend=1), dict(relocation=99), dict(flags=2)]:
            with self.assertRaises(AssertionError):
                pool_object(fixture(**args), 25, 18)
        with self.assertRaises(AssertionError):
            pool_object(bytes(52), 25, 18)


def target_check():
    gen, out = ROOT / 'build/gen', ROOT / 'build/voice-experiment'
    before = (ROOT / 'build/felucca.literals-raw.o').read_bytes()
    after = (ROOT / 'build/felucca.o').read_bytes()
    parts = [(gen / ('literal_' + n + '_oracle.bin')).read_bytes() for n in ('main', 'pool', 'flash')]
    assert pool_object(before)[0] == after  # Deterministic production transformation.
    count = compare_objects(before, after, parts)
    code = '''#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "boot_tables.c"
#include "boot_literals_blob.c"
uint8_t boot_literal_main_start[%d+16],boot_literal_pool_start[%d+16];
#include "boot_literals.c"
int main(int argc,char **argv) {
assert(argc==2);
for(int j=0;j<3;j++) {
memset(boot_literal_main_start,0xa5,sizeof boot_literal_main_start);
memset(boot_literal_pool_start,0xa5,sizeof boot_literal_pool_start);
assert(boot_literals_init());
for(int i=0;i<16;i++) {
assert(boot_literal_main_start[%d+i]==0xa5);
assert(boot_literal_pool_start[%d+i]==0xa5);
}}
FILE *f=fopen(argv[1],"wb");assert(f);
assert(fwrite(boot_literal_main_start,1,%d,f)==%d);
assert(fwrite(boot_literal_pool_start,1,%d,f)==%d);assert(!fclose(f));
return 0;}
''' % (len(parts[0]), len(parts[1]), len(parts[0]), len(parts[1]),
       len(parts[0]), len(parts[0]), len(parts[1]), len(parts[1]))
    source, binary, restored = out / 'boot-literals-test.c', out / 'boot-literals-test', out / 'boot-literals-restored.bin'
    source.write_text(code)
    subprocess.run([str(ROOT / 'build/host/optimization-cc'), '-O2', '-UNDEBUG',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all', '-I' + str(gen),
                    '-I' + str(ROOT / 'firmware/src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(restored)], check=True)
    assert restored.read_bytes() == parts[0] + parts[1]
    # Check every original literal's FINAL linked address against restored bytes.
    ss, sy, content = elf(before)
    ls, ly, load = elf((ROOT / 'build/felucca.elf').read_bytes())
    linked = {s['name']: s for s in ly}
    starts = [linked['boot_literal_' + n + '_start']['value'] for n in ('main', 'pool')]
    for s in sy:
        if s['index'] >= len(ss) or ss[s['index']][2] != 0x32 or s['kind'] & 15 != 1:
            continue
        d = linked[s['name']]
        raw = content(ss[s['index']])[s['value']:s['value'] + s['size']]
        if ls[d['index']][1] == 8:
            i = 1 if d['value'] >= starts[1] else 0
            offset = d['value'] - starts[i]
            got = parts[i][offset:offset + d['size']]
        else:
            offset = d['value'] - ls[d['index']][3]
            got = load(ls[d['index']])[offset:offset + d['size']]
        assert got == raw, s['name']
    print(f'Target literals: {count} complete objects, final relocated addresses, three cold restores, guard bytes and UBSan PASS')


if __name__ == '__main__':
    if '--target' in sys.argv:
        target_check()
    else:
        unittest.main()
