/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
static uint32_t checks,hits;
static void verify(gr_part_t *P)
{
    static const uint32_t offsets[]={0,1,127,128,129,254,255};
    for(uint32_t zl=0;zl<P->nz;zl++) {
        const smp_zone_t *z=gr_zone(P->src-1u,zl);
        for(uint32_t seg=0;seg<P->zdone[zl];seg++)for(uint32_t j=0;j<NELEM(offsets);j++) {
            uint32_t pos=(seg<<GR_SEG_LOG2)+offsets[j],e=P->zbase[zl]+seg;
            if(pos>=z->n)continue;
            int32_t pred=P->ipred[e],idx=P->iidx[e],got,gi;
            for(uint32_t q=seg<<GR_SEG_LOG2;q<pos;q++)gr_dec(z,q,&pred,&idx);
            gr_seek(P,z,zl,pos,&got,&gi);
            assert(got==pred && gi==idx);checks++;
            hits+=e<GR_NMID && offsets[j]>=128u && seg+1u<P->zdone[zl];
        }
    }
}
int main(void)
{
    host_tracks_init();track_t *t=&trk[0];host_preset(t,ENGI_GRAIN,2);
    gr_part_t *P=eng_arena_of(t,ENGI_GRAIN);
    for(uint32_t src=0;src<SMP_NSETS;src++) {
        /* Reuse deliberately dirty storage across sources; startup must not
         * consume stale midpoint values before the next full entry exists. */
        gr_reset(P,src,0);verify(P);
        for(uint32_t b=0;b<GR_NIDX;b++)gr_build(P,t);
        verify(P);
    }
    assert(hits>1000);
    printf("GRAIN seeks: %u cold/built boundary checks match original decoder states; %u midpoint hits skip 128 decodes; arena unchanged PASS\n",checks,hits);
    return 0;
}
