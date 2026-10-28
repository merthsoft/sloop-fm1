/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen .88 output/state hashes protect the optimization's slow and settled
 * paths, including phase/history retained by currently unused mechanisms. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
static uint64_t hash_bytes(uint64_t h,const void *data,size_t n)
{
    const uint8_t *p=data;while(n--)h=(h^*p++)*1099511628211ull;return h;
}
int main(void)
{
    static const uint64_t expected[VINYL_MODE_COUNT]={
        0x071d3a26421f8811ULL,0x67d02216d9646d59ULL,0x62ffd5fc56a06ae8ULL,
        0xcff50a420dc5564bULL,0x8b27b5fb5208c788ULL,0xf02a8e015becdb38ULL,
        0x4e83c5062a2981efULL};
    for(uint32_t v=0;v<=32767;v++)assert(vinyl_div127(v)==v/127u);
    for(uint32_t bits=1;bits<=15;bits++) {
        int32_t edge[]={INT32_MIN,INT32_MIN+1,-131071,-32769,-127,-3,-1,0,1,3,127,32769,131071,INT32_MAX};
        for(uint32_t i=0;i<NELEM(edge);i++)assert(vinyl_div_pow2(edge[i],bits)==edge[i]/(1<<bits));
    }
    uint32_t rnd=1741;
    for(uint32_t mode=0;mode<VINYL_MODE_COUNT;mode++) {
        uint64_t hash=1469598103934665603ull;
        for(uint32_t scenario=0;scenario<4;scenario++) {
            host_tracks_init();punch_clear();song.playing=1;fxfade=0;
            memset(&vinyl,0,sizeof vinyl);
            song.g[G_VWOW]=scenario==1?0:64;
            song.g[G_VTONE]=scenario==1?80:64;song.g[G_VCRACKLE]=scenario==1?30:64;
            int32_t l[CTL],r[CTL];
            for(uint32_t b=0;b<2500;b++) {
                uint32_t n=scenario==3?1+b%CTL:CTL,active=mode;
                if(scenario==2 && b==1500)song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=0;
                if(scenario==2 && b==2100)song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=127;
                if(scenario==3) {
                    if(!(b%37u)) {
                        song.g[G_VWOW]=rnd%128;song.g[G_VTONE]=(rnd>>8)%128;song.g[G_VCRACKLE]=(rnd>>16)%128;
                    }
                    active=(mode+b/97u)%VINYL_MODE_COUNT;song.playing=(b/331u)&1u;fxfade=(b/181u)%4u;
                    punch_hold(0,PX_VINYL,(b/101u)&1u);
                }
                for(uint32_t i=0;i<n;i++) {
                    rnd=rnd*1664525u+1013904223u;l[i]=(int32_t)(rnd%8000001u)-4000000;
                    rnd=rnd*1664525u+1013904223u;r[i]=(int32_t)(rnd%8000001u)-4000000;
                }
                vinyl_process_mode(l,r,n,active);
                hash=hash_bytes(hash,l,n*sizeof *l);hash=hash_bytes(hash,r,n*sizeof *r);
                hash=hash_bytes(hash,&vinyl,sizeof vinyl);
            }
        }
#ifdef LOFI_PRINT_HASHES
        printf("0x%016llxULL,\n",(unsigned long long)hash);
#else
        assert(hash==expected[mode]);
#endif
    }
    puts("LOFI fast path: exact bounded arithmetic and frozen .88 samples/state, settled controls, zero WOW, bypass, punch/mode/transport transitions PASS");
    return 0;
}
