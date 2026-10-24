/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

static uint64_t silence_blocks(unsigned mode, unsigned blocks)
{
    int32_t l[CTL],r[CTL]; uint64_t heard=0;
    for (unsigned b=0;b<blocks;b++) {
        memset(l,0,sizeof l); memset(r,0,sizeof r);
        vinyl_process_mode(l,r,CTL,mode);
        for (unsigned i=0;i<CTL;i++) heard+=abs(l[i])+abs(r[i]);
    }
    return heard;
}

static int mixer_peak(unsigned blocks)
{
    int32_t out[2*CTL]; int peak=0;
    for (unsigned b=0;b<blocks;b++) {
        mix_block(out,CTL);
        for (unsigned i=0;i<2*CTL;i++) if (abs(out[i])>peak) peak=abs(out[i]);
    }
    return peak;
}

int main(void)
{
    host_tracks_init(); punch_clear();
    for (unsigned mode=0;mode<VINYL_MODE_COUNT;mode++) {
        if (mode==VINYL_MODE_OFF) continue;
        for (unsigned off_mode=0;off_mode<2;off_mode++) {
            memset(&vinyl,0,sizeof vinyl); vinyl.mode=mode;
            song.playing=1; song.g[G_VWOW]=song.g[G_VTONE]=0; song.g[G_VCRACKLE]=127;
            uint64_t playing=silence_blocks(mode,1000);
            if (mode!=VINYL_MODE_CD) assert(playing>0); /* CD damages content; no independent noise. */
            unsigned bypass=off_mode?VINYL_MODE_OFF:mode;
            if (!off_mode) song.g[G_VCRACKLE]=0;
            silence_blocks(bypass,1000);
            song.playing=0; assert(silence_blocks(bypass,1000)==0);
            song.g[G_VCRACKLE]=127;
            uint64_t heard=silence_blocks(mode,1000);
            if (heard) fprintf(stderr,"stopped LOFI mode %u bypass %u: heard %llu\n",mode,off_mode,(unsigned long long)heard);
            assert(heard==0 && song.g[G_VCRACKLE]==127);
            /* Re-enabling transport still produces noise; active STOP keeps its fade. */
            song.playing=1; playing=silence_blocks(mode,1000);
            if (mode!=VINYL_MODE_CD) assert(playing>0);
            int32_t before_stop=vinyl.bed;
            song.playing=0; silence_blocks(mode,1);
            assert(vinyl.bed>0 && vinyl.bed<before_stop); /* Active STOP slews; bypass STOP resets. */
            silence_blocks(mode,1000); assert(silence_blocks(mode,100)==0);
        }
    }
    /* Real saved TYPE mapping, events and master chain, with silent tracks. */
    host_tracks_init(); punch_clear(); memset(&vinyl,0,sizeof vinyl);
    song.g[G_LTYPE]=1; song.g[G_VCRACKLE]=127; song.playing=1;
    assert(mixer_peak(1000)>100);
    song.g[G_LTYPE]=0; mixer_peak(1000);
    song.playing=0; mixer_peak(1000);
    song.g[G_LTYPE]=1;
    assert(mixer_peak(1000)<=2); /* The master's integer DC blocker can leave a one-LSB tail. */
    puts("LOFI PLAY/bypass/STOP/re-enable: every character, OFF/zero bypass, active fades PASS");
    return 0;
}
