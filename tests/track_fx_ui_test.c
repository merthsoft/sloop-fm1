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
    /* Reset is confined to the selected track and leaves pitch/sequence alone. */
    song.sel=0;open_family(FAM_FX);song.octave=2;
    TSEL->p[P_ROOT]=7;TSEL->step[0]=(step_t){.n=1,.note={60}};
    fm1_in.buttons=1u<<panel.btn[B_OCTDN];edges_btn=BT(B_OCTDN);ui_input();
    assert(song.octave==2);
    fm1_in.buttons|=1u<<panel.btn[B_OCTUP];edges_btn=BT(B_OCTUP);ui_input();
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) assert(TSEL->p[TRACK_FX[i]]==TP[TRACK_FX[i]].def);
    assert(song.octave==2&&TSEL->p[P_ROOT]==7&&TSEL->step[0].note[0]==60);
    assert(trk[1].p[P_DLY]==19&&trk[1].p[P_REV]==27);
    puts("track FX: preset, engine and MIDI program browsing preserve effects; both OCT reset is selected-track only PASS");
    return 0;
}
