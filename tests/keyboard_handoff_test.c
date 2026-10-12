/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint32_t gates(track_t *t,uint32_t note) {
    uint32_t n=0;for(uint32_t i=0;i<NVOICE;i++)n+=t->v[i].active&&t->v[i].gate&&t->v[i].note==note;return n;
}
int main(void) {
    panel=PANEL_DEFAULT;layers_init();palette_set(4);host_tracks_init();song.sel=0;
    track_t *t=TSEL;t->p[P_CHORD]=t->p[P_AMODE]=t->p[P_QUANT]=0;t->p[P_VOICE]=V_POLY;
    lights_notes=0;usb.config=1;go_home();frame();
    for(int direction=-1;direction<=1;direction+=2)for(int order=0;order<2;order++) {
        song.octave=0;uint32_t original=direction>0?19:7, target=original-direction*12;
        mo_r=mo_w;uint32_t packets=mo_w;
        uint32_t note=kb_map(t,original);fm1_in.notes=1u<<original;frame();assert(gates(t,note)==1&&mo_w==packets+1);
        uint32_t age=vage;tap(direction>0?B_OCTUP:B_OCTDN);
        assert(kb_map(t,target)==note&&gates(t,note)==1&&(keys_lit()&(1u<<target)));
        fm1_in.notes|=1u<<target;frame();assert(vage==age&&gates(t,note)==1&&mo_w==packets+1);
        fm1_in.notes&=~(1u<<(order?target:original));frame();assert(gates(t,note)==1&&vage==age&&mo_w==packets+1);
        fm1_in.notes=0;frame();assert(gates(t,note)==0&&mo_w==packets+2);
        /* A later, non-overlapping press is still a new attack. */
        fm1_in.notes=1u<<target;frame();assert(vage>age&&gates(t,note)==1);
        fm1_in.notes=0;frame();assert(gates(t,note)==0);
    }
    /* Same pitch on another synth is independent. */
    song.octave=0;song.sel=0;fm1_in.notes=1u<<7;frame();uint32_t note=kb_map(t,7);
    song.sel=1;trk[1].p[P_QUANT]=trk[1].p[P_CHORD]=trk[1].p[P_AMODE]=0;
    fm1_in.notes|=1u<<19;song.octave=-1;frame();assert(gates(&trk[1],note)==1);
    fm1_in.notes&=~(1u<<7);frame();assert(gates(t,note)==0&&gates(&trk[1],note)==1);
    fm1_in.notes=0;frame();assert(gates(&trk[1],note)==0);
    puts("keyboard handoff: both octave directions, release orders, no attack, final release, reattack and track isolation PASS");
    return 0;
}
