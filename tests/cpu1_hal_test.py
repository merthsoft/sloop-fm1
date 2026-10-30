#!/usr/bin/env python3
"""Exercise production worker HAL with deterministic MMIO/barrier doubles.

This checks protocol/fallback behavior, not silicon cache ordering or stack traps.
Only register accesses, assembly barriers, and terminal fault loops are replaced.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / 'build' / 'host' / 'cpu1-hal'
out.mkdir(parents=True, exist_ok=True)
hal = (root / 'firmware/hal/fm1_cpu1.h').read_text()
hal, barriers = re.subn(r'__asm__ volatile\("csync" ::: "memory"\)', 'test_sync()', hal)
assert barriers == 1
hal, registers = re.subn(r'\*\(volatile uint32_t \*\)(0x[0-9A-Fa-f]+u)', r'*test_reg(\1)', hal)
assert registers >= 9
hal = hal.replace('*(volatile uint32_t *)(0x1EEF300u + 4u * i)', '*test_reg(0x1EEF300u + 4u * i)')
assert 'extern uint32_t _c1_ustack[], _c1_sstack_top[];' in hal
hal = hal.replace('extern uint32_t _c1_ustack[], _c1_sstack_top[];', '')
(out / 'worker.h').write_text(hal)
irq = (root / 'firmware/hal/fm1_irq.h').read_text()
fault = irq[irq.index('void fm1_fault_c('):]
fault = fault.replace('__asm__ volatile("csync" ::: "memory")', 'test_sync()')
fault = fault.replace('*(volatile uint32_t *)0x10804u', '*test_reg(0x10804u)')
fault = fault.replace('*(volatile uint32_t *)0x10000u', '*test_reg(0x10000u)')
fault, loops = re.subn(r'for \(;;\)\s*;', 'test_fault_stop();', fault)
assert loops == 2
(out / 'fault.h').write_text(fault)
source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stddef.h>
static uint32_t regs[64], addrs[64], nregs;
static uint32_t *test_reg(uint32_t a) {
    for(uint32_t i=0;i<nregs;i++) if(addrs[i]==a) return &regs[i];
    assert(nregs<64);addrs[nregs]=a;return &regs[nregs++];
}
static void test_sync(void);
static unsigned halted, calls, worker, boot_reply, worker_steps, replay;
static jmp_buf worker_end, fault_end;
static void fm1_cpu1_halt(void) { halted++; }
void fm1_c1_entry(void) {}
static uint32_t test_stack[800];
#define _c1_ustack test_stack
#define _c1_sstack_top (test_stack+799)
#include "worker.h"
static void test_sync(void) {
    *test_reg(0x10804u)+=24000u;
    if(worker) {
        assert(++worker_steps<100);
        if(replay && fm1_c1_mb.done==fm1_c1_mb.job) {
            fm1_c1_mb.done--;replay=0; /* stale published completion must not rerun work */
        } else if(fm1_c1_mb.bad || fm1_c1_mb.done==fm1_c1_mb.job || worker_steps>20) {
            longjmp(worker_end,1);
        }
    } else if(boot_reply && (FM1_C1_CON&8u) && !(FM1_C1_CON&2u)) fm1_c1_mb.alive=1;
}
static void execute(void) {
    worker_steps=0; worker=1;
    if(!setjmp(worker_end)) fm1_c1_main();
    worker=0;
}
static void sum(uint32_t x) { assert(x==calls+7);calls++; }
static void other(uint32_t x) { (void)x;assert(0 && "corrupt function executed"); }
static void reset(void) {
    memset(&fm1_c1_mb,0,sizeof fm1_c1_mb);memset(regs,0,sizeof regs);
    fm1_c1_on=0;fm1_c1_gave_up=0;fm1_c1_bar.magic=fm1_c1_bar.barred=0;
    calls=halted=worker=replay=0;boot_reply=1;
}
#define FM1_CRASH_MAGIC 0x43525348u
typedef struct {
 uint32_t magic,count,vec,pc,rets,emu,dbg,sp,psr,icfg,uptime_ms,etm[4],early;
} fm1_crash_t;
static fm1_crash_t fm1_crash;
static const char fm1_fatal_stubs[256];
#define FM1_DBG_MSG 0x1234u
#define FM1_EMU_MSG 0x5678u
#define FM1_ETM_PC(i) (100u+(i))
static unsigned fault_reported;
static void fm1_irq_off(void) {}
static void fm1_fault(const fm1_crash_t *c) { assert(c==&fm1_crash);fault_reported++;longjmp(fault_end,1); }
static void test_fault_stop(void) { longjmp(fault_end,1); }
#include "fault.h"
int main(void) {
 _Static_assert(sizeof(void*)!=4 || (sizeof(fm1_c1_mb_t)==128 && offsetof(fm1_c1_mb_t,alive)==64),"separate target mailbox lines");
 _Static_assert(sizeof fm1_c1_bar==8 && sizeof fm1_crash==64,"retained record sizes");
 reset();assert(!fm1_cpu1_start());assert(fm1_c1_on && fm1_c1_mb.alive);
 for(unsigned i=0;i<2816/4;i++) assert(_c1_ustack[i]==FM1_C1_MARK);
 assert(fm1_cpu1_start()==0);
 for(unsigned i=0;i<20000;i++) {
    assert(fm1_cpu1_run(sum,i+7));execute();assert(!fm1_cpu1_wait());
 }
 assert(calls==20000 && !fm1_c1_mb.bad && !fm1_c1_gave_up);
 assert(fm1_c1_mb.job==fm1_c1_mb.done);
 execute();assert(calls==20000); /* no pending job */
 assert(fm1_cpu1_run(sum,calls+7));replay=1;execute();assert(calls==20001);
 reset();assert(!fm1_cpu1_start());fm1_c1_mb.job=fm1_c1_mb.done=0xffffffffu;
 assert(fm1_cpu1_run(sum,7));execute();assert(!fm1_cpu1_wait() && !fm1_c1_mb.job && calls==1);
 for(unsigned bad=0;bad<5;bad++) {
    reset();assert(!fm1_cpu1_start());assert(fm1_cpu1_run(sum,7));
    if(bad==0) fm1_c1_mb.check^=1;
    if(bad==1) fm1_c1_mb.fn=0;
    if(bad==2) fm1_c1_mb.arg^=1;
    if(bad==3) fm1_c1_mb.job++;
    if(bad==4) fm1_c1_mb.fn=other;
    execute();assert(!calls && fm1_c1_mb.bad==1);
    assert(fm1_cpu1_wait()==-1 && !fm1_c1_on && fm1_c1_gave_up==1);
    assert(!fm1_cpu1_run(sum,7));
    /* BSS reset, retained bar survives: no restart until a cold power-on. */
    memset(&fm1_c1_mb,0,sizeof fm1_c1_mb);
    assert(fm1_cpu1_start()==-1 && fm1_c1_gave_up==3);
    fm1_c1_bar.magic=fm1_c1_bar.barred=0;assert(!fm1_cpu1_start());
 }
 reset();assert(!fm1_cpu1_start());assert(fm1_cpu1_run(sum,7));
 *test_reg(0x10804u)=0xffff0000u;
 assert(fm1_cpu1_wait()==-1 && fm1_c1_gave_up==2 && !fm1_c1_on && !calls);
 assert(fm1_cpu1_start()==-1);
 reset();boot_reply=0;assert(fm1_cpu1_start()==-1 && !fm1_c1_on);
 reset();assert(!fm1_cpu1_start());assert(fm1_cpu1_run(sum,7));execute();
 assert((*test_reg(0x1EEF2D0u)&8u) && !(*test_reg(0x1EEF2D0u)&(4u|0x1f0000u)));
 assert(*test_reg(0x1EEF2DCu)==(uint32_t)(uintptr_t)_c1_ustack+256u);
 /* Exercise the production fault handler: a worker fault records and parks;
    a main-core fault records, preserves early-crash tracking and reports. */
 uint32_t f[26]={0};
 f[19]=(uint32_t)(uintptr_t)fm1_fatal_stubs+12;f[16]=321;f[25]=654;f[20]=987;
 /* Host pointers may truncate in the firmware's 32-bit stack-range comparison. */
 uint32_t lo=(uint32_t)(uintptr_t)_c1_ustack,hi=(uint32_t)(uintptr_t)_c1_sstack_top;
 assert(lo<hi);
 {
    f[24]=lo+16;
    if(!setjmp(fault_end)) fm1_fault_c(f);
    assert(!fault_reported && fm1_crash.emu==0xc1 && fm1_crash.pc==321 && fm1_crash.rets==654);
 }
 memset(&fm1_crash,0,sizeof fm1_crash);*test_reg(0x10804u)=24000;
 f[24]=0;
 if(!setjmp(fault_end)) fm1_fault_c(f);
 assert(fault_reported==1 && fm1_crash.magic==FM1_CRASH_MAGIC && fm1_crash.count==1);
 assert(fm1_crash.emu==FM1_EMU_MSG && fm1_crash.early==1 && fm1_crash.etm[3]==103);
 puts("cpu1 HAL: 20000 valid jobs, rejected corrupt jobs, timeout/wrap, warm-reset bar, stack setup and fault paths PASS");
 return 0;
}
'''
(out / 'test.c').write_text(source)
compiler = shlex.split(os.environ.get('CC', 'cc'))
subprocess.run(compiler + ['-O2', '-UNDEBUG', '-Wno-attributes', '-Wno-unknown-attributes',
                         '-I' + str(out), str(out / 'test.c'), '-o', str(out / 'test')], check=True)
subprocess.run([str(out / 'test')], check=True, timeout=20)
if '--target' in sys.argv:
    # Host function pointers are wider: verify the linked 32-bit layout too.
    sys.path.insert(0, str(root / 'tests'))
    from pool_literals_test import elf
    _, symbols, _ = elf((root / 'build/felucca.elf').read_bytes())
    symbols = {s['name']: s for s in symbols}
    for name, size in [('fm1_c1_mb', 128), ('fm1_c1_bar', 8), ('fm1_crash', 64)]:
        assert symbols[name]['size'] == size, name
    assert symbols['fm1_c1_mb']['value'] % 64 == 0
    assert symbols['fm1_c1_bar']['value'] >= symbols['fm1_crash']['value'] + 64
    print('Target mailbox size/alignment and retained crash/bar layout PASS')
