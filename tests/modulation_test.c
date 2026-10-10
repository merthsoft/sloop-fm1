/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    song.sel=1;go_home();frame();
    trk[1].p[P_LD_PIT]=7;
    midi_cc(&trk[1],1,127);
    assert(mod_pitch(&trk[1],32767)==2031 && mod_pitch(&trk[0],32767)==0);
    int rate=trk[1].p[P_LRATE];
    uint32_t page=ui.page;
    press(B_LFO);frames(12);assert(live_mod[0].on && ui.layer==LY_VIB && ui.page==page);
    assert(layer_now()==LY_PLAY);
    fm1_in.notes = 1u << key_of_white(0);frame();
    uint32_t playing=0;for(int i=0;i<NVOICE;i++)playing+=trk[1].v[i].gate;
    assert(playing);fm1_in.notes=0;frame();

    for(int i=0;i<50;i++)live_mod_tick(&trk[1]);
    assert(mod_pitch(&trk[0],32767)==0 && mod_pitch(&trk[1],32767)!=2031);
    encs[panel.enc[EN_K2]]=127;frame();
    assert(live_mod[0].depth==127 && trk[1].p[P_LRATE]==rate && trk[1].p[P_LD_PIT]==7);
    release(B_LFO);assert(!live_mod[0].on && ui.layer==LY_PLAY && ui.page==page && mod_pitch(&trk[1],32767)==2031);
    press(B_ENV);frames(12);assert(live_mod[1].on && ui.layer==LY_TREM);
    live_mod[1].value=32767;
    assert(live_tremolo(&trk[1])<32767 && live_tremolo(&trk[0])==32767);
    release(B_ENV);assert(!live_mod[1].on && ui.page==page);
    midi_cc(&trk[1],121,0);assert(!mod_pitch(&trk[1],32767));
    midi_cc(TDRUM,1,127);assert(!mod_pitch(TDRUM,32767));
    midi_cc(&trk[1],1,50);midi_cc(TDRUM,121,0);assert(mod_midi[1]==50);
    for(int v=0;v<128;v++) { midi_cc(&trk[1],1,v);int q=mod_pitch(&trk[1],32767);assert(q>=0&&q<=2031);assert(mod_pitch(&trk[1],-32768)<=0); }
    press(B_LFO);frames(12);assert(live_mod[0].on);
    song.sel=2;frame();assert(!live_mod[0].on);release(B_LFO);
    midi_cc(&trk[1],1,127);panic_req|=2;frame();assert(!mod_midi[1]);
    midi_cc(&trk[2],1,127);seq_stop();assert(!mod_midi[2]);
    midi_in_q[mi_w++%MQ]=0x0bu | (0xb1u<<8) | (1u<<16) | (70u<<24);frame();
    assert(mod_midi[1]==70 && mod_source[1]==1);
    midi_in_q[mi_w++%MQ]=0x1bu | (0xb2u<<8) | (1u<<16) | (90u<<24);frame();
    assert(mod_midi[2]==90 && mod_source[2]==2);
    usb.resets++;frame();assert(!mod_midi[1] && mod_midi[2]==90);
    int saved=trk[1].p[P_LD_PIT];trk_note_on(&trk[1],60,100);frame();
    uint32_t gates=0;for(int i=0;i<NVOICE;i++)gates+=trk[1].v[i].gate;
    midi_cc(&trk[1],1,127);frame();
    uint32_t after=0;for(int i=0;i<NVOICE;i++)after+=trk[1].v[i].gate;
    assert(gates && gates==after && trk[1].p[P_LD_PIT]==saved);
    song.sel=1;press(B_LFO);frames(12);seq_stop();encs[panel.enc[EN_K1]]=8;frame();
    assert(!live_mod[0].on && trk[1].p[P_LRATE]==rate);release(B_LFO);
    for(int wave=0;wave<4;wave++) {
        live_mod[0].on=1;live_mod[0].part=1;live_mod[0].wave=wave;
        live_mod[0].phase=0;live_mod[0].sync=0;
        int lo=9999,hi=-9999;
        for(int i=0;i<1200;i++) {
            live_mod_tick(&trk[1]);int pitch=mod_pitch(&trk[1],0);
            if(pitch<lo)lo=pitch;if(pitch>hi)hi=pitch;
            assert(pitch>=-2032 && pitch<=2031 && mod_pitch(&trk[0],0)==0);
        }
        assert(lo<0 && hi>0);
    }
    /* SYNC follows transport phase, independent of free Hz and tempo changes. */
    live_mod[0].on=1;live_mod[0].part=1;song.playing=1;
    for(int div=0;div<NDIV_STEP;div++) {
        live_mod[0].sync=div+1;
        for(int quarter=0;quarter<4;quarter++) {
            uint32_t pos=div_units(div)*quarter/4;
            clk_beat=pos/BEAT_U;clk_pos=pos%BEAT_U;live_mod_tick(&trk[1]);
            uint32_t expected=(uint32_t)(((uint64_t)pos<<32)/div_units(div));
            assert(expected-live_mod[0].phase<30000000u);
            uint32_t phase=live_mod[0].phase;live_mod[0].rate=17;song.g[G_BPM]=210;live_mod_tick(&trk[1]);
            assert(live_mod[0].phase==phase);
        }
    }
    song.playing=0;live_mod[0].sync=1;live_mod[0].phase=0;
    song.g[G_BPM]=60;live_mod_tick(&trk[1]);uint32_t slow=live_mod[0].phase;
    live_mod[0].phase=0;song.g[G_BPM]=120;live_mod_tick(&trk[1]);assert(live_mod[0].phase==2*slow);
    live_mod[0].on=0;press(B_LFO);frames(12);encs[panel.enc[EN_K4]]=3;frame();assert(live_mod[0].sync);
    encs[panel.enc[EN_K1]]=1;frame();assert(!live_mod[0].sync);release(B_LFO);
    mod_reset(NTRK);
    press(B_LFO);release(B_LFO);assert(cur_page()->fam==FAM_LFO);
    press(B_ENV);release(B_ENV);assert(cur_page()->fam==FAM_ENV);
    /* HOME locks the panel/effect on the captured track; notes remain playable. */
    outdir="build/host";song.sel=1;go_home();frame();lights_scale=1;
    trk[1].p[P_ROOT]=0;trk[1].p[P_SCALE]=2;trk[1].p[P_QUANT]=2;
    for(int effect=0;effect<2;effect++) {
        uint32_t button=effect ? B_ENV : B_LFO, layer=effect ? LY_TREM : LY_VIB;
        uint32_t old_page=ui.page, old_home=ui.home;
        press(button);frames(12);tap(B_HOME);release(button);frames(12);
        assert(ly_lock==layer && ui.layer==layer && live_mod[effect].on && live_mod[effect].part==1);
        assert(layer_now()==LY_PLAY && keys_notes_dim()==scale_keys(0) && !keys_guide());
        assert(ui.page==old_page && ui.home==old_home);
        int depth=live_mod[effect].depth;
        encs[panel.enc[EN_K2]]=-1;frame();assert(live_mod[effect].depth==depth-1 && live_mod[effect].on);
        fm1_in.notes=1u<<key_of_white(4);frame();
        uint32_t held=0;for(int voice=0;voice<NVOICE;voice++)held+=trk[1].v[voice].gate;
        assert(held);fm1_in.notes=0;frame();
        ui.force=1;frame();ppm(effect ? "tremolo-locked" : "vibrato-locked");
        tap(B_HOME);assert(ly_lock==LY_PLAY && !live_mod[effect].on && ui.layer==LY_PLAY);
        assert(ui.page==old_page && ui.home==old_home);
        press(button);frames(12);tap(B_HOME);release(button);seq_stop();frame();
        assert(!live_mod[effect].on && ly_lock==LY_PLAY && ui.layer==LY_PLAY);
        press(button);frames(12);tap(B_HOME);release(button);panic_req=2;frame();
        assert(!live_mod[effect].on && ly_lock==LY_PLAY);
        press(button);frames(12);tap(B_HOME);release(button);song.sel=2;frame();
        assert(!live_mod[effect].on && ly_lock==LY_PLAY);song.sel=1;frame();
        press(button);frames(12);tap(B_HOME);release(button);
        tap(B_SEQ);assert(!live_mod[effect].on && ly_lock==LY_PLAY && ui.page==old_page);
        tap(B_SEQ);assert(cur_page()->fam==FAM_SEQ);go_home();frame();
    }
    song.sel=TRK_DRUM;frame();press(B_ENV);frames(12);tap(B_HOME);release(B_ENV);
    assert(ly_lock==LY_PLAY && !live_mod[1].on);song.sel=1;go_home();frame();
    puts("modulation: CC1, temporary vibrato/tremolo isolation and release, saved depth/rate, bounds and cleanup PASS");
    return 0;
}
