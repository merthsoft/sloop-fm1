/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/rhythm_shapes.c"
static uint32_t pop(uint64_t x) { uint32_t n=0; while(x) {x&=x-1; n++;} return n; }
static uint64_t shaped(rhythm_shape_t *s,uint32_t len,uint32_t part,uint64_t mask,uint32_t beat)
{
    uint64_t out=0,seen=0;
    for(uint32_t d=0;d<len;d++) {
        uint32_t src=rhythm_shape_source(s,len,d,part,mask,beat);
        assert(src<len); assert(!(seen&((uint64_t)1<<src)));
        seen|=(uint64_t)1<<src;
        if(mask&((uint64_t)1<<src)) out|=(uint64_t)1<<d;
    }
    assert(pop(out)==pop(mask)); return out;
}
int main(void)
{
    rhythm_shape_t s={0,0,0,RHYTHM_PART_ALL,0};
    assert(rhythm_shape_source(&s,0,0,0,0,4)==0);
    assert(rhythm_shape_source(&s,65,0,0,0,4)==0);
    assert(rhythm_shape_source(0,16,19,0,0,4)==3);
    assert(shaped(&s,16,0,0x9249,4)==0x9249);
    s.rotate=-1; assert(shaped(&s,16,0,1,4)==0x8000);
    s.rotate=1; assert(shaped(&s,64,0,1ULL<<63,4)==1);
    s.rotate=0; s.part=2; s.offset=2;
    assert(shaped(&s,16,2,1,4)==4); assert(shaped(&s,16,0,1,4)==1);
    s.offset=0;s.sync=2;
    assert(shaped(&s,16,0,0x1111,4)==0x8888);
    assert(shaped(&s,16,0,0x9999,4)==0x9999); /* occupied target survives */
    s.sync=1; assert(shaped(&s,16,0,0x1111,4)==0x9090);
    /* Every event maps exactly once, including partial bars and full 64-bit
     * masks. This tests collisions, rather than just the chosen sound. */
    for(uint32_t len=1;len<=64;len++) {
        uint64_t valid=len==64?UINT64_MAX:((1ULL<<len)-1);
        for(uint32_t beat=0;beat<=5;beat++) for(uint32_t sync=0;sync<=2;sync++) {
            s.sync=sync;s.rotate=-17;s.offset=9;s.part=2;
            uint64_t masks[]={0,valid,0x1111111111111111ULL&valid,0x9249249249249249ULL&valid,valid>>1};
            for(uint32_t k=0;k<sizeof masks/sizeof masks[0];k++) {
                uint64_t a=shaped(&s,len,2,masks[k],beat);
                assert(a==shaped(&s,len,2,masks[k],beat));
            }
        }
    }
    s.feel=-8;assert(rhythm_shape_micro(&s,-30)==-32);
    s.feel=8;assert(rhythm_shape_micro(&s,30)==31);
    assert(rhythm_shape_micro(&s,-12)==-4);assert(rhythm_shape_micro(0,0)==0);
    puts("rhythm_shapes: identity, wrap, isolation, sync collisions, all lengths and timing PASS");
    return 0;
}

