/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_AUDIO_DIAG 1
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void pump(uint32_t frames) {
    int32_t out[CTL*2];while(frames>=CTL){mix_block(out,CTL);frames-=CTL;}
}
int main(int argc,char **argv) {
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    voice_expand=4;track_t *t=&trk[2];host_preset(t,3,0);
    t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=t->p[P_CHORD]=0;
    drift_p[2][0]=2;pump(CTL*4);
    drone_toggle(t,60,27);drone_toggle(t,64,27);drone_toggle(t,67,27);
    assert(!voice_kills&&!shed_count&&!drone_restores);
    shed_voice();pump(FS/2);
    assert(voice_kills==1&&shed_count==1&&drone_restores==1);
    voice_t *v=voice_alloc(t,72);assert(v);voice_start(t,v,72,100,0);
    /* Explicit budget/slot kill is counted separately from CPU shedding. */
    voice_kill(t,v);assert(voice_kills==2&&shed_count==1);
    felucca_dbg.late=3;audio_wall_max=5100;
    mi_set(MI_ABOUT,0);assert(ui.menu==2&&ui.force);draw_menu();ui.force=0;
    static uint16_t before[240*240];memcpy(before,screen,sizeof screen);
    voice_kills++;draw_menu();assert(!memcmp(before,screen,sizeof screen));
    menu_input(1u<<panel.btn[B_OCTUP]);assert(ui.force);draw_menu();
    assert(memcmp(before,screen,sizeof screen));ppm("audio-diagnostics");
    menu_input(1u<<panel.btn[B_OCTDN]);assert(ui.menu==1);
    puts("audio diagnostics: real shed/recovery counters, distinct non-CPU steal, snapshot/refresh/back, bounded rendering PASS");
}
