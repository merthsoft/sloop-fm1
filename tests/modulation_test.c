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
    press(B_LFO);assert(mod_physical && !mod_pitch(&trk[1],32767));
    encs[panel.enc[EN_K1]]=40;frame();
    assert(mod_amount==127 && trk[1].p[P_LRATE]==rate && trk[1].p[P_LD_PIT]==7);
    release(B_LFO);assert(!mod_physical && mod_pitch(&trk[1],32767)==2031);
    midi_cc(&trk[1],121,0);assert(!mod_pitch(&trk[1],32767));
    midi_cc(TDRUM,1,127);assert(!mod_pitch(TDRUM,32767));
    midi_cc(&trk[1],1,50);midi_cc(TDRUM,121,0);assert(mod_midi[1]==50);
    for(int v=0;v<128;v++) { midi_cc(&trk[1],1,v);int q=mod_pitch(&trk[1],32767);assert(q>=0&&q<=2031);assert(mod_pitch(&trk[1],-32768)<=0); }
    press(B_LFO);encs[panel.enc[EN_K1]]=8;frame();assert(mod_amount==32);
    song.sel=2;frame();assert(!mod_physical);release(B_LFO);
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
    song.sel=1;press(B_LFO);seq_stop();encs[panel.enc[EN_K1]]=8;frame();
    assert(!mod_physical && trk[1].p[P_LRATE]==rate);release(B_LFO);
    puts("modulation: CC1, physical priority/release, saved depth/rate, bounds and cleanup PASS");
    return 0;
}
