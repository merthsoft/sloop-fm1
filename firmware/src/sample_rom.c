/* SPDX-License-Identifier: GPL-3.0-only */
/* Factory-only lossless packing. Each decoder owns its cache, including across cores. */
#if SMP_ROM_PACKED
_Static_assert(sizeof(((smp_cache_t *)0)->bytes) == SMP_HUFF_BLOCK, "sample cache block size");
static __attribute__((noinline)) void smp_rom_fill(smp_cache_t *cache, uint32_t block)
{
    uint32_t k=block*3u, off=SMP_HUFF_OFF[k] | (uint32_t)SMP_HUFF_OFF[k+1u]<<8 | (uint32_t)SMP_HUFF_OFF[k+2u]<<16;
    const uint8_t *p=SMP_DATA+off;
    uint32_t n=SMP_HUFF_RAW_N-block*SMP_HUFF_BLOCK, bit=0, i;
    if (n>SMP_HUFF_BLOCK) n=SMP_HUFF_BLOCK;
    for (i=0;i<n;i++) {
        uint32_t at=bit>>3, peek=(((uint32_t)p[at]<<8 | p[at+1u])>>(8u-(bit&7u)))&255u;
        uint32_t v=SMP_HUFF_FAST[peek];
        if (v&0x8000u) { bit+=(v>>8)&15u; v&=255u; }
        else {
            v=0;
            do { v=SMP_HUFF_TREE[v][(p[bit>>3]>>(7u-(bit&7u)))&1u]; bit++; } while (v<256u);
            v-=256u;
        }
        cache->bytes[i]=(uint8_t)v;
    }
    cache->tag=block+1u;
}
#endif
DSP_INLINE uint32_t smp_rom_byte(smp_cache_t *cache, uint32_t off)
{
#if SMP_ROM_PACKED
    if (off<SMP_HUFF_RAW_N) {
        uint32_t block=off/SMP_HUFF_BLOCK;
        if (cache->tag!=block+1u) smp_rom_fill(cache,block);
        return cache->bytes[off&(SMP_HUFF_BLOCK-1u)];
    }
#else
    (void)cache;
#endif
    return SMP_DATA[off]; /* user slots keep their upstream XIP representation */
}
