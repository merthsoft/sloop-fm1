/* SPDX-License-Identifier: GPL-3.0-only */
/* An independent raw oracle verifies random cache misses and every factory byte. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint8_t raw[400000];
int main(int argc, char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb"); assert(f);
    size_t n=fread(raw,1,sizeof raw,f); fclose(f);
    assert(n==SMP_HUFF_RAW_N);
    smp_cache_t a={0},b={0};
    for (uint32_t i=0;i<n;i++) {
        assert(smp_rom_byte(&a,i)==raw[i]);
        assert(smp_rom_byte(&b,(uint32_t)n-1u-i)==raw[n-1u-i]);
    }
    uint32_t seed=123;
    for (uint32_t i=0;i<100000;i++) {
        seed=seed*1664525u+1013904223u;
        uint32_t at=seed%(uint32_t)n;
        assert(smp_rom_byte(&a,at)==raw[at]);
    }
    puts("factory sample ROM: every byte forward/reverse and 100000 random reads PASS");
    return 0;
}
