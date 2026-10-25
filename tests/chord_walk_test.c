/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint32_t original(uint32_t note,uint32_t degree,uint32_t root,uint32_t mask)
{
    int32_t m=(int32_t)note,guard=48;
    while(degree>0 && guard--) {m++;if((mask>>(uint32_t)((m-(int32_t)root+120)%12))&1u)degree--;}
    return (uint32_t)m;
}
int main(void)
{
    unsigned cases=0;
    for(unsigned mask=0;mask<4096;mask++)for(unsigned d=0;d<16;d++)for(unsigned root=0;root<12;root++) {
        uint32_t n=(mask+d*7+root)%128;
        assert(chord_walk(n,d,mask,(int32_t)root)==original(n,d,root,mask));cases++;
    }
    printf("shared chord walk: %u bounded chromatic/empty/diatonic mask and octave cases PASS\n",cases);
    return 0;
}
