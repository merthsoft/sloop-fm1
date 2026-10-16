/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(void)
{
    int16_t fx[NELEM(TRACK_FX)];
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();song.sel=0;
    set_engine_of(TSEL,0);
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) {
        uint32_t p=TRACK_FX[i];TSEL->p[p]=fx[i]=TP[p].max;
    }
    apply_preset(1);
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(TSEL->p[TRACK_FX[i]]==fx[i]);
    set_engine(1);
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(TSEL->p[TRACK_FX[i]]==fx[i]);
    bank_resolve();assert(!preset_go_to(TSEL,0));
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(TSEL->p[TRACK_FX[i]]==fx[i]);
    song.sel=1;set_engine_of(TSEL,0);TSEL->p[P_DLY]=19;TSEL->p[P_REV]=27;
    assert(!preset_go_to(&trk[0],NBANK-1));
    assert(TSEL->p[P_DLY]==19&&TSEL->p[P_REV]==27);
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(trk[0].p[TRACK_FX[i]]==fx[i]);
    /* Each playing mode survives both knobs and MIDI program changes. Factory
     * initialization still chooses a preset's own default mode. */
    for (uint32_t mode=V_POLY;mode<=V_UNISON;mode++) {
        track_t *t=&trk[0];song.sel=0;
        t->p[P_VOICE]=mode;t->p[P_GLIDE]=47;t->p[P_GLMODE]=1;t->p[P_PRIO]=1;t->p[P_ALLOC]=1;
        apply_preset(2);set_engine((mode+1)%NENGINES);preset_go_to(t,mode);
        assert(t->p[P_VOICE]==mode && t->p[P_GLIDE]==47 && t->p[P_GLMODE]==1);
        assert(t->p[P_PRIO]==1 && t->p[P_ALLOC]==1);
        pc_req[0]=mode+1;pc_poll();
        assert(t->p[P_VOICE]==mode && t->p[P_GLIDE]==47);
    }
    /* Reset is confined to the selected track and leaves pitch/sequence alone. */
    song.sel=0;open_family(FAM_FX);song.octave=2;
    TSEL->p[P_ROOT]=7;TSEL->step[0]=(step_t){.n=1,.note={60}};
    fm1_in.buttons=1u<<panel.btn[B_OCTDN];edges_btn=BT(B_OCTDN);ui_input();
    assert(song.octave==2);
    fm1_in.buttons|=1u<<panel.btn[B_OCTUP];edges_btn=BT(B_OCTUP);ui_input();
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(TSEL->p[TRACK_FX[i]]==TP[TRACK_FX[i]].def);
    assert(song.octave==2&&TSEL->p[P_ROOT]==7&&TSEL->step[0].note[0]==60);
    assert(TSEL->p[P_VOICE]==V_UNISON&&TSEL->p[P_GLIDE]==47);
    assert(trk[1].p[P_DLY]==19&&trk[1].p[P_REV]==27);
    puts("track FX: preset, engine and MIDI program browsing preserve effects; both OCT reset is selected-track only PASS");
    return 0;
}
