/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

/* Frozen control-wave formulas, before the shared evaluator. */
static int32_t reference(uint32_t wave,uint32_t phase,int32_t held)
{
    switch(wave) {
    case 1:return osc_tri(phase);
    case 2:return (int32_t)(phase>>16)-32768;
    case 3:return phase<0x80000000u?32767:-32767;
    case 4:return held;
    default:return osc_sine(phase);
    }
}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();
    uint32_t seed=12345,global=rng_state;
    for(uint32_t i=0;i<100000;i++) {
        seed=seed*1664525u+1013904223u;
        uint32_t phase=i<8?i*0x20000000u:seed;
        int32_t held=(int32_t)(seed>>16)-32768;
        for(uint32_t wave=0;wave<6;wave++)assert(modulation_wave(wave,phase,held)==reference(wave,phase,held));
        for(uint32_t wave=0;wave<4;wave++) {
            live_mod_t m={0};m.wave=(uint8_t)wave;m.depth=127;
            int32_t w=reference(wave,phase,0);
            assert(live_mod_plot_y(&m,phase,0)==62-w*127*42/(32768*127));
            assert(live_mod_plot_y(&m,phase,1)==20+((w+32768)/2)*127*84/(32768*127));
        }
    }
    assert(rng_state==global); /* Sampling and display never draw random state. */
    assert(modulation_wave(3,0,0)==32767 && modulation_wave(3,0x80000000u,0)==-32767);
    assert(modulation_wave(2,0,0)==-32768 && modulation_wave(2,UINT32_MAX,0)==32767);
    track_t *t=&trk[0];
    for(uint32_t mode=2;mode<=5;mode++)for(uint32_t rate=0;rate<NELEM(N_EVOLVE_CYCLE);rate++) {
        memset(drift,0,sizeof drift);drift_p[0][0]=(int16_t)mode;drift_p[0][1]=(int16_t)rate;
        drift_p[0][2]=127;drift_p[0][3]=DRIFT_MORPH;snapshot_p[0][0]=64;
        song.playing=1;clk_beat=1;clk_pos=BEAT_U/4;
        for(uint32_t tick=0;tick<600;tick++) {
            drift_tick(t);
            int32_t expected=reference(mode-2,drift[0].phase,0)*drift[0].depth>>15;
            assert(drift[0].value==expected);
            assert(snapshot_position(0)==clamp(64+(expected>>8),0,127));
        }
        assert(drift[0].depth==16256 && drift[0].shape==mode-1);
        if(rate>=8) {
            song.playing=0;int32_t held=drift[0].value;uint32_t phase=drift[0].phase;
            for(uint32_t tick=0;tick<10;tick++)drift_tick(t);
            assert(drift[0].phase==phase && drift[0].value==held);
        }
        uint32_t oldshape=drift[0].shape;
        drift_p[0][0]=1;drift_tick(t);assert(drift[0].shape==oldshape && drift[0].depth==16224);
        for(uint32_t tick=0;tick<1100;tick++)drift_tick(t);
        assert(!drift[0].shape && drift[0].depth==16256); /* Fade through zero before RAND. */
        drift_p[0][0]=0;for(uint32_t tick=0;tick<600;tick++)drift_tick(t);
        assert(!drift[0].depth && !drift[0].value);
    }
    assert(rng_state==global);
    assert(DRIFT_DESC[0].max==5 && sizeof drift[0]==20);
    song.sel=0;go_home();open_family(FAM_LFO);
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_DRIFT)ui.page=p;
    ui.home=0;page_entered();frame();
    encs[panel.enc[EN_K1]]=100;frame();assert(drift_p[0][0]==5 && !strcmp(ui.big_v[0],"SQR"));
    puts("Shared modulation: 600000 frozen waveform comparisons, plot parity, all shaped MORPH cycles, fades/pause/RNG, UI and unchanged state size PASS");
}
