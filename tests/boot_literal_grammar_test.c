/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "boot_tables.c"
uint8_t boot_literal_main_start[32],boot_literal_pool_start[32];
const uint8_t boot_literal_main_blob[]={0x30,129,129,129};
const uint8_t boot_literal_pool_blob[]={0x10,255};
/* 128=AB, 129=ABC, 130=invalid cycle, 131=DEL+ABC, 132=literal. */
const uint8_t boot_literal_pairs[]={'A','B',128,'C',130,130,127,129,0,0};
const uint32_t boot_literal_sizes[]={4,3,9,2,1,1,5};
#include "boot_literals.c"

int main(void)
{
    memset(boot_literal_main_start,0xa5,sizeof boot_literal_main_start);
    memset(boot_literal_pool_start,0xa5,sizeof boot_literal_pool_start);
    assert(boot_literals_init());
    assert(!memcmp(boot_literal_main_start,"ABCABCABC",9));
    assert(boot_literal_pool_start[0]==255);
    for(unsigned i=9;i<32;i++)assert(boot_literal_main_start[i]==0xa5);
    for(unsigned i=1;i<32;i++)assert(boot_literal_pool_start[i]==0xa5);
    const uint8_t tokens[]={0x40,129,'X',128,132};
    const uint32_t valid[]={5,4,7};
    uint8_t out[32];memset(out,0xa5,sizeof out);
    const uint8_t expected[]={'A','B','C','X','A','B',132};
    assert(boot_literal_restore(out,tokens,valid));
    assert(!memcmp(out,expected,sizeof expected));
    assert(out[7]==0xa5);
    /* Refuse over/under expansion and output shorter than the input tokens. */
    const uint8_t one[]={0x10,129};
    const uint32_t short_output[]={2,1,2},long_output[]={2,1,4},input_overflow[]={5,4,3};
    memset(out,0xa5,sizeof out);assert(!boot_literal_restore(out,one,short_output));
    assert(out[2]==0xa5);
    assert(!boot_literal_restore(out,one,long_output));
    assert(!boot_literal_restore(out,tokens,input_overflow));
    /* Cyclic dictionary must terminate via the bounded stack, not recurse. */
    const uint8_t cycle[]={0x10,130};
    const uint32_t size[]={2,1,1};
    assert(!boot_literal_restore(out,cycle,size));
    const uint8_t nul[]={0x20,0,0};
    const uint32_t zeros[]={3,2,2};
    assert(boot_literal_restore(out,nul,zeros)&&!out[0]&&!out[1]);
    puts("boot literal grammar: backward overlap, nested tokens, high/NUL literals, guards and malformed trees PASS");
    return 0;
}
