#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compile the real HAL clamp and interpret its target register operations.

This catches operand aliasing/register changes; it is not device execution or
a timing test. The portable contract test covers dynamic/inverted bounds.
"""
import os, random, re, subprocess
from pathlib import Path

root=Path(__file__).resolve().parents[1]
tc=Path(os.environ['JIELI_TOOLCHAIN'])
out=root/'build/target-clamp-test';out.mkdir(parents=True,exist_ok=True)
src=out/'probe.c';obj=out/'probe.o'
src.write_text('#include <stdint.h>\n#include "fm1_math.h"\nint32_t probe(int32_t v,int32_t lo,int32_t hi){return fm1_clamp_ordered(v,lo,hi);}\n')
subprocess.run([str(tc/'pi32v2/bin/clang'),'-target','pi32v2','-Oz','-Ifirmware/hal','-c',str(src),'-o',str(obj)],cwd=root,check=True)
dis=subprocess.check_output([str(tc/'common/bin/objdump'),'-d',str(obj)],text=True)
(out/'probe.dis').write_text(dis)
body=[]
for line in dis.splitlines():
    m=re.match(r'\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+\s*(.*)',line)
    if m:body.append(m[1].strip())
assert any('smax' in x for x in body) and any('smin' in x for x in body),body
assert body[-1]=='rts',body
def run(v,lo,hi):
    regs={'r0':v,'r1':lo,'r2':hi}
    for ins in body:
        if ins=='rts':return regs['r0']
        m=re.fullmatch(r'(r\d+) = s(max|min)\((r\d+), (r\d+)\)',ins)
        if m:
            dst,op,a,b=m.groups();regs[dst]=(max if op=='max' else min)(regs[a],regs[b]);continue
        m=re.fullmatch(r'(r\d+) = (r\d+)',ins)
        assert m,('unreviewed target instruction',ins)
        regs[m[1]]=regs[m[2]]
    raise AssertionError('no return')
edges=[-2**31,-2**31+1,-65536,-32768,-1,0,1,32767,65535,2**31-2,2**31-1]
count=0
for v in edges:
    for lo in edges:
        for hi in edges:
            if lo<=hi:assert run(v,lo,hi)==min(max(v,lo),hi);count+=1
rng=random.Random(71024)
for _ in range(20000):
    v,lo,hi=[rng.randrange(-2**31,2**31) for _ in range(3)]
    lo,hi=sorted((lo,hi));assert run(v,lo,hi)==min(max(v,lo),hi);count+=1
print(f'target clamp: {count} signed boundary/random cases using actual target registers PASS')
