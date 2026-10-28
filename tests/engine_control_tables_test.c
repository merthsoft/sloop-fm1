/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

static uint32_t original_pow2(int32_t d16)
{
    uint32_t u=(uint32_t)(d16+192*16),oct=u/192u;
    uint32_t r=pitch_inc(1600+u%192u)/(pitch_inc(1600)>>16);
    return oct>=16u?r<<(oct-16u):r>>(16u-oct);
}
int main(void)
{
    for(int32_t d=-3072;d<6144;d++)assert(pow2_q16(d)==original_pow2(d));
    for(uint32_t a=0;a<1024;a++) {
        uint32_t expected=0;
        for(uint32_t k=0;k<6;k++)expected|=((FM6_ALG[a&31u][k]&3u)?0u:1u)<<k;
        assert(fm6_carriers(a)==expected);
    }
    for(uint32_t bits=0;bits<2;bits++)for(uint32_t table=0;table<16;table++) {
        int32_t sum=0;
        for(uint32_t j=0;j<16;j++) {
            int32_t b=WRAM[table][j];
            sum+=bits?((b>>7)+((b>>3)&1))*2-2:(b>>4)+(b&15)-15;
        }
        assert(WRAM_DC[bits][table]==(bits?sum*32767/32:sum*2*Q4/32));
    }
    /* Low eight inc bits are unused in both formulas. All upper 24-bit values
     * therefore cover every uint32 input, including every saturation boundary. */
    for(uint32_t r=0;r<NELEM(RATIO_Q8);r++) {
        uint32_t ratio=RATIO_Q8[r],limit=0x73000000u/ratio;
        assert(RATIO_LIMIT[r]==limit);
        for(uint32_t hi=0;hi<(1u<<24);hi++) {
            uint32_t expected=hi>limit?0x73000000u:hi*ratio;
            assert(fm_ratio_inc(hi<<8,r)==expected);
        }
    }
    puts("engine tables: full defined pitch domain, all FM6 carrier masks/aliases, signed WRAM means and exhaustive DIGITAL increment range PASS");
    return 0;
}
