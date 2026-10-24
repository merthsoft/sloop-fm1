/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void blocks(unsigned n) {
    int32_t l[CTL],r[CTL];
    for(unsigned b=0;b<n;b++) {
        for(unsigned i=0;i<CTL;i++)l[i]=r[i]=10000;
        punch_process(l,r,CTL);
    }
}
int main(void) {
    host_tracks_init();
    assert(punch_fade_step()==512);
    fxfade=15;assert(punch_fade_step()==2);
    punch_hold(0,PX_CRUSH,1);punch_hold(0,PX_LPF,1);punch_hold(0,PX_GATE,1);
    blocks(1);assert(punch.g==CTL*2 && punch_filter.gain==CTL*2 && punch_gate.gain==CTL*2);
    blocks(520);assert(punch.g==32767 && punch_filter.gain==32767 && punch_gate.gain==32767);
    punch_hold(0,PX_CRUSH,0);punch_hold(0,PX_LPF,0);punch_hold(0,PX_GATE,0);
    blocks(1);assert(punch.g==32767-CTL*2 && punch_filter.gain==32767-CTL*2 && punch_gate.gain==32767-CTL*2);
    blocks(520);assert(!punch.g && !punch_filter.gain && !punch_gate.gain);
    /* LOFI punch follows fade while preserving saved controls and OFF. */
    int32_t l[CTL]={0},r[CTL]={0};
    song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=0;
    punch_hold(0,PX_VINYL,1);vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);
    assert(vinyl.punch_gain==CTL*2 && !song.g[G_VWOW] && !song.g[G_VTONE]);
    punch_hold(0,PX_VINYL,0);vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);assert(!vinyl.punch_gain);
    song.g[G_VWOW]=127;song.g[G_VTONE]=121;song.g[G_VCRACKLE]=115;
    punch_hold(0,PX_VINYL,1);
    for(unsigned b=0;b<540;b++)vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);
    assert(vinyl.punch_gain==32768 && vinyl.wow==127*256 && vinyl.tone==121*256 && vinyl.crackle==115*256);
    assert(song.g[G_VWOW]==127 && song.g[G_VTONE]==121 && song.g[G_VCRACKLE]==115);
    punch_hold(0,PX_VINYL,0);
    for(unsigned b=0;b<540;b++)vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);
    assert(!vinyl.punch_gain && !vinyl.wow && !vinyl.tone && !vinyl.crackle);
    /* Resonance updates existing coefficient stage; bypass and wah remain distinct. */
    for(unsigned res=0;res<=10;res++) {
        djf_t f={0};tsvf_t c,base;flt_reso=res;
        for(unsigned b=0;b<100;b++)assert(djf_block_res(&f,-32,&c,40+7*flt_reso));
        tsvf_coef(&base,f.cut,40+7*res);assert(!memcmp(&c,&base,sizeof c) && f.mode==-1);
        int32_t hot[CTL];for(unsigned b=0;b<300;b++) {
            for(unsigned i=0;i<CTL;i++)hot[i]=(i&1)?140000:-140000;
            djf_run(&f,&c,hot,CTL,0);
            for(unsigned i=0;i<CTL;i++)assert(abs(hot[i])<1000000);
        }
        for(unsigned b=0;b<100;b++)djf_block_res(&f,0,&c,40+7*flt_reso);
        assert(!f.mode && !djf_block_res(&f,0,&c,40+7*flt_reso));
        assert(djf_block_res(&f,-32,&c,116) && f.mode==-2);
        assert(djf_block_res(&f,-32,&c,40+7*flt_reso) && f.mode==-1);
    }
    puts("upstream effects: primary/filter/gate/LOFI fades, settings immutability, resonance stress/wah/bypass PASS");return 0;
}
