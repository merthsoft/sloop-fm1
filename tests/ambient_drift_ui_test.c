/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    song.sel=0;go_home();open_family(FAM_LFO);
    for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_DRIFT)ui.page=p;
    ui.home=0;page_entered();frame();
    assert(!strcmp(ui.big_l[0],"MODE") && !strcmp(ui.big_l[1],"CYCLE"));
    int16_t patch[P_COUNT];memcpy(patch,TSEL->p,sizeof patch);uint8_t lock=ui.lock_par;
    encs[panel.enc[EN_K1]]=1;frame();assert(drift_p[0][0]==1);
    encs[panel.enc[EN_K2]]=100;frame();assert(drift_p[0][1]==5);
    encs[panel.enc[EN_K3]]=100;frame();assert(drift_p[0][2]==127);
    encs[panel.enc[EN_K4]]=100;frame();assert(drift_p[0][3]==2);
    assert(!memcmp(patch,TSEL->p,sizeof patch) && lock==ui.lock_par);
    /* Audio-owned value changes invalidate the graph without touching a knob.
     * Its moving marker stays visible while zoom/focus is active. */
    drift[0].value=8192;ui.force=0;draw_graph();
    assert(screen[(Y_GRAPH+46)*240+172]==swap16(C_WHITE));
    drift[0].value=-8192;draw_graph();
    assert(screen[(Y_GRAPH+46)*240+68]==swap16(C_WHITE));
    uint16_t shown[240*240];memcpy(shown,screen,sizeof shown);
    ui.force=1;draw_graph();assert(!memcmp(shown,screen,sizeof shown));
    ppm("ambient-drift");track_select(1);frame();assert(!drift_p[1][0] && drift_p[1][1]==1);
    track_select(TRK_DRUM);int16_t *v;for(unsigned k=0;k<4;k++)assert(!page_desc(cur_page(),k,&v));
    puts("ambient UI: real knob routing, limits, no patch/lock edits, per-track defaults and drum exclusion PASS");
    return 0;
}
