/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    song.sel=1;go_home();frame();
    uint8_t page=ui.page, home=ui.home;
    lights_scale=0;lights_notes=0;
    int octave=song.octave;
    press(B_SCL);tap(B_OCTUP);
    assert(lights_scale && settings_later && song.octave==octave);
    frame();assert(lights_scale); /* held combination does not repeat */
    release(B_SCL);assert(ui.page==page && ui.home==home); /* no SEL page tap */
    assert(ui.layer==LY_PLAY);
    for (int scale=0;scale<NSCALES;scale++) for (int root=0;root<12;root++) {
        trk[0].p[P_ROOT]=(root+1)%12;trk[0].p[P_SCALE]=(scale+1)%NSCALES;
        trk[1].p[P_ROOT]=root;trk[1].p[P_SCALE]=scale;
        uint32_t expected=0;
        for (int k=0;k<27;k++)
            if ((SCALE_MASK[scale] >> ((53+k-root+120)%12)) & 1u) expected |= 1u<<k;
        assert(keys_notes_dim()==expected);
        for (int mode=0;mode<KEYS_N;mode++) for (int level=0;level<LIGHTS_N;level++) {
            lights_keys=mode;lights_lvl=level;
            assert(lights_keys_mask()==0);
            assert((keys_notes_dim() | lights_keys_mask())==expected);
        }
    }
    trk[0].p[P_SCALE]=0;trk[1].p[P_SCALE]=1;trk[1].p[P_ROOT]=2;
    press(B_SCL);frames(20);assert(strstr(sub_line(),"D maj"));
    encs[panel.enc[EN_K2]]=1;frame();
    for (int part=0;part<NPART;part++) assert(trk[part].p[P_SCALE]==2);
    release(B_SCL); /* held knob starts from displayed selected scale, retains all-part shortcut */
    lights_keys=KEYS_ALL;lights_lvl=LIGHTS_HIGH;
    lights_scale=0;assert(lights_keys_mask()==((1u<<27)-1u));lights_scale=1;
    song.sel=3;assert(is_drum(TSEL));assert(keys_notes_dim()==0);
    assert(lights_keys_mask()==((1u<<27)-1u)); /* drum backlight remains intact */
    song.sel=1;kb_grid=1;assert(keys_notes_dim()==0);kb_grid=0;
    ui.layer=LY_FX;assert(keys_notes_dim()==0);ui.layer=LY_STEP;assert(keys_notes_dim()==0);
    assert(lights_keys_mask()==((1u<<27)-1u)); /* control layers retain their backlight */
    ui.layer=LY_PLAY;
    fm1_in.notes=1u<<5;assert(keys_lit() & (1u<<5));fm1_in.notes=0; /* played keys stay bright */
    lights_lvl=2;lights_keys=KEYS_WHITE;lights_notes=1;vis_style=3;
    uint32_t stored=lights_word();lights_scale=0;lights_from_word(stored);
    assert(lights_scale && lights_lvl==2 && lights_keys==KEYS_WHITE && lights_notes && vis_style==3);
    lights_from_word(stored & ~(1u<<22));assert(!lights_scale); /* older settings default off */
    lights_scale=1;press(B_SCL);tap(B_OCTUP);release(B_SCL);
    assert(!lights_scale && song.octave==octave);
    trk[1].p[P_CHORD]=1;trk[1].p[P_AHOLD]=0;
    fm1_in.notes=1u<<7;frame();
    press(B_SCL);tap(B_OCTUP);frames(50);
    assert(lights_scale && !trk[1].p[P_AHOLD]); /* gesture cancels SEL long-hold latch */
    release(B_SCL);fm1_in.notes=0;frame();
    puts("scale lights: physical toggle, all roots/scales, layer isolation and settings PASS");
    return 0;
}
