/* SPDX-License-Identifier: GPL-3.0-only */
/* FREEZE's bounded spring return and both crossfades against a wide oracle. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

static int32_t freeze_expected(int32_t dry, int32_t wet, int32_t gain)
{
    int32_t held = (int32_t)(((int64_t)wet * gain) >> 10);
    int32_t wl = dry + (int32_t)((((int64_t)(held - dry) >> 3) * (32767 >> 3)) >> 9);
    return dry + (int32_t)((((int64_t)(wl - dry) >> 3) * (32767 >> 3)) >> 9);
}

static void range_oracle(void)
{
    static const int32_t returns[] = {-196608,-131072,-65536,-1,0,1,65535,131071,196602};
    static const int32_t inputs[] = {-4000000,-1000000,-200000,-1,0,1,200000,1000000,4000000};
    int32_t l[CTL], r[CTL]; unsigned checks = 0;
    for (unsigned v=0;v<NELEM(returns);v++) for (int gain=1024;gain<=16384;gain+=1024)
        for (unsigned x=0;x<NELEM(inputs);x++) {
            punch_clear(); punch.req=PX_FREEZE; punch_start(PX_FREEZE);
            punch.t=PX_FRZ_CAP; punch.g=32767; punch.fg=gain;
            punch.mabs=(int32_t)(((int64_t)abs(returns[v])*2*gain)>>10);
            for (unsigned i=0;i<CTL;i++) {
                l[i]=inputs[x]; r[i]=-inputs[x]; rev_l[i]=returns[v]; rev_r[i]=-returns[v];
            }
            punch_primary(l,r,CTL);
            for (unsigned i=0;i<CTL;i++) {
                int32_t el=freeze_expected(inputs[x],returns[v],punch.fg);
                int32_t er=freeze_expected(-inputs[x],-returns[v],punch.fg);
                if (l[i]!=el || r[i]!=er) {
                    fprintf(stderr,"FREEZE range: dry=%d return=%d gain=%d actual=%d/%d expected=%d/%d\n",
                            inputs[x],returns[v],punch.fg,l[i],r[i],el,er);
                    assert(l[i]==el && r[i]==er);
                }
                checks++;
            }
        }
    printf("FREEZE wide oracle: %u stereo samples PASS\n",checks);
}

static void spring_chain(int sign)
{
    host_tracks_init(); punch_clear(); memset(&punch,0,sizeof punch);
    punch.req=punch.cur=-1; punch.fg=8192;
    memset(&fx,0,sizeof fx); memset(rev_line,0,sizeof rev_line);
    memset(&rev_storage,0,sizeof rev_storage); memset(&spring,0,sizeof spring);
    rev_frz=0; song.g[G_RMODEL]=1; song.g[G_RSIZE]=127;
    reverb_model.cur=1; reverb_model.gain=32768;
    int32_t l[CTL],r[CTL],rev[CTL],z[CTL]={0},wl[CTL],wr[CTL],dl[CTL],dr[CTL];
    int32_t dry=sign*4000000; unsigned large_products=0;
    for (unsigned b=0;b<1000;b++) {
        for (unsigned i=0;i<CTL;i++) l[i]=r[i]=dry;
        punch_process(l,r,CTL);
    }
    punch_hold(0,PX_FREEZE,1);
    for (unsigned b=0;b<2000;b++) {
        for (unsigned i=0;i<CTL;i++) { l[i]=r[i]=dry; rev[i]=0; }
        punch_freeze_feed(rev,l,r,CTL); fx_buses(z,z,rev,wl,wr,CTL);
        for (unsigned i=0;i<CTL;i++) { dl[i]=l[i]+=wl[i]; dr[i]=r[i]+=wr[i]; }
        unsigned full=punch.t>=PX_FRZ_CAP && punch.g==32767;
        punch_process(l,r,CTL);
        if (full) for (unsigned i=0;i<CTL;i++) {
            assert(l[i]==freeze_expected(dl[i],rev_l[i],punch.fg));
            assert(r[i]==freeze_expected(dr[i],rev_r[i],punch.fg));
            int64_t p=(int64_t)rev_l[i]*punch.fg;
            large_products+=p>INT32_MAX || p<INT32_MIN;
        }
    }
    assert(large_products>1000);
    punch_hold(0,PX_FREEZE,0);
    for (unsigned b=0;b<10;b++) {
        for (unsigned i=0;i<CTL;i++) { l[i]=12345; r[i]=-6789; }
        punch_process(l,r,CTL);
    }
    for (unsigned i=0;i<CTL;i++) assert(l[i]==12345 && r[i]==-6789);
    printf("SPRING capture/hold/release sign %d: %u large products checked PASS\n",sign,large_products);
}

int main(void)
{
    host_tracks_init(); range_oracle(); spring_chain(1); spring_chain(-1);
    return 0;
}
