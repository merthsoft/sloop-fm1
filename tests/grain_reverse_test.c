/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen full-window playback oracle versus actual optimized production code.
 * Default covers every window boundary; GRAIN_REVERSE_FULL=1 checks all offsets. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
static void old_fill(const gr_part_t *P, gr_grain_t *g, int16_t *rb, uint32_t zl, uint32_t hi)
{
    uint32_t lo = hi >= GR_RB - 1u ? hi - (GR_RB - 1u) : 0u, q;
    int32_t pred, idx;
    gr_seek(P, g->z, zl, lo, &pred, &idx);
    for (q = lo; q <= hi; q++)
        rb[q - lo] = (int16_t)gr_dec(g->z, q, &pred, &idx);
    g->rlo = lo;
}
static int old_run(gr_part_t *P, gr_grain_t *g, int32_t *acc, uint32_t n)
{
    uint32_t i, m = n < g->left ? n : g->left, step = g->step, pos = g->pos;
    int32_t a = g->a, b = g->b, frac = g->frac, w0, w1, wq, dq;
    const smp_zone_t *z = g->z;
    w0 = mulq15(gr_win(g->wph), g->gain);
    g->wph += g->winc * m;
    g->left -= m;
    w1 = g->left ? mulq15(gr_win(g->wph), g->gain) : 0;
    wq = w0 * 1024;
    dq = m == CTL ? ((w1 - w0) * 1024) >> CTL_LOG2 : m ? (w1 - w0) * 1024 / (int32_t)m : 0;
    if (!g->rev) {
        int32_t pred = g->pred, idx = g->idx;
        for (i = 0; i < m; i++) {
            int32_t s = a + (((b - a) * (frac >> 1)) >> 15);
            acc[i] += (s * (wq >> 10)) >> 15;
            wq += dq;
            frac += (int32_t)step;
            while (frac >= 65536) {
                frac -= 65536;
                a = b;
                b = pos < z->n ? gr_dec(z, pos++, &pred, &idx) : a;
            }
        }
        g->pred = pred;
        g->idx = idx;
    } else {
        int16_t *rb = P->rb[g - P->g];
        for (i = 0; i < m; i++) {
            int32_t s = a + (((b - a) * (frac >> 1)) >> 15);
            acc[i] += (s * (wq >> 10)) >> 15;
            wq += dq;
            frac -= (int32_t)step;
            while (frac < 0) {
                frac += 65536;
                b = a;
                if (pos) {                          /* (fitted: always) */
                    pos--;
                    if (pos < g->rlo)
                        old_fill(P, g, rb, g->zl, pos);
                }
                a = rb[pos - g->rlo];
            }
        }
    }
    g->pos = pos;
    g->a = a;
    g->b = b;
    g->frac = frac;
    if (!g->left)
        g->owner = 0;
    return g->left != 0;
}

int main(void)
{
    host_tracks_init();track_t *t=&trk[0];host_preset(t,ENGI_GRAIN,2);
    gr_part_t *P=eng_arena_of(t,ENGI_GRAIN),*A=calloc(1,sizeof *A),*B=calloc(1,sizeof *B);
    assert(A && B);
    unsigned cases=0,blocks=0,full=getenv("GRAIN_REVERSE_FULL")!=0;
    const uint32_t steps[]={256,16384,32768,65536,98304,131072};
    for(uint32_t src=0;src<SMP_NSETS;src++)for(unsigned phase=0;phase<2;phase++) {
        gr_reset(P,src,0);
        if(phase)for(unsigned q=0;q<GR_NIDX;q++)gr_build(P,t);
        for(uint32_t zl=0;zl<P->nz;zl++) {
            const smp_zone_t *z=gr_zone(src,zl);
            if(z->n<260 || !P->zcnt[zl])continue;
            for(uint32_t k=0;k<3;k++)for(uint32_t off=0;off<256;off++)for(uint32_t si=0;si<NELEM(steps);si++) {
                if(!full && off!=0 && off!=1 && off!=126 && off!=127 && off!=128 && off!=129 && off!=254 && off!=255)continue;
                uint32_t base=k==0?0:k==1?((z->n/2)&~255u):((P->zdone[zl]-1u)*256u);
                uint32_t pos=base+off;
                if(pos+2>=z->n || ((pos+1)>>8)>=P->zdone[zl])continue;
                *A=*P;*B=*P;gr_grain_t *a=&A->g[0],*b=&B->g[0];memset(a,0,sizeof *a);
                a->z=z;a->pos=pos;a->step=steps[si];a->rev=1;a->zl=zl;a->owner=1;
                uint64_t duration=(uint64_t)pos*65536u/steps[si];
                a->left=duration>2048?2048:(uint32_t)duration;
                a->winc=0xffffffffu/(a->left?a->left:1);a->gain=32767;*b=*a;
                old_fill(A,a,A->rb[0],zl,pos+1);gr_fill(B,b,B->rb[0],zl,pos+1);
                a->a=A->rb[0][pos-a->rlo];a->b=A->rb[0][pos+1-a->rlo];
                b->a=B->rb[0][pos-b->rlo];b->b=B->rb[0][pos+1-b->rlo];
                while(a->left) {
                    int32_t x[CTL]={0},y[CTL]={0};uint32_t n=cases%CTL+1;
                    int ra=old_run(A,a,x,n),rb=gr_run(B,b,y,n);
                    assert(ra==rb && !memcmp(x,y,sizeof x));
                    assert(a->pos==b->pos && a->frac==b->frac && a->a==b->a && a->b==b->b);
                    assert(a->left==b->left && a->wph==b->wph && a->owner==b->owner);
                    blocks++;
                }
                cases++;
            }
        }
    }
    printf("GRAIN reverse: %u grains, %u blocks match original playback across cold/full indexes, source changes, interpolation boundaries and rates PASS\n",cases,blocks);
    free(A);free(B);return 0;
}
