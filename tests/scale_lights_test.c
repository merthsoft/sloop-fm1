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
        for(int layer=0;layer<3;layer++) {
        ui.layer=layer==0 ? LY_PLAY : layer==1 ? LY_VIB : LY_TREM;
        assert(keys_guide()==0);
        assert(keys_notes_dim()==expected);
        for (int mode=0;mode<KEYS_N;mode++) for (int level=0;level<LIGHTS_N;level++) {
            lights_keys=mode;lights_lvl=level;
            assert(lights_keys_mask()==0);
            assert((keys_notes_dim() | lights_keys_mask())==expected);
        }
    }
        }
    ui.layer=LY_PLAY;
    /* Real button holds must preserve every keyboard LED role, including release. */
    trk[1].p[P_SCALE]=1;trk[1].p[P_ROOT]=2;
    for(int enabled=0;enabled<2;enabled++) {
        lights_scale=enabled;lights_keys=KEYS_ALL;lights_lvl=LIGHTS_HIGH;
        fm1_in.notes=1u<<5;frame();
        uint32_t dim=keys_notes_dim(),back=lights_keys_mask(),bright=keys_lit(),guide=keys_guide();
        for(int button=0;button<2;button++) {
            press(button ? B_ENV : B_LFO);frames(12);
            assert(ui.layer==(button ? LY_TREM : LY_VIB));
            assert(keys_notes_dim()==dim && lights_keys_mask()==back && keys_lit()==bright && keys_guide()==guide);
            release(button ? B_ENV : B_LFO);
            assert(ui.layer==LY_PLAY && keys_notes_dim()==dim && lights_keys_mask()==back && keys_lit()==bright && keys_guide()==guide);
        }
        fm1_in.notes=0;frame();
    }
    lights_scale=1;
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
    uint32_t stored=lights_word(), forked=fork_settings_word();lights_scale=0;lights_from_word(stored);fork_settings_from_word(forked);
    assert(lights_scale && lights_lvl==2 && lights_keys==KEYS_WHITE && lights_notes && vis_style==3);
    lights_from_word(stored);assert(!lights_scale); /* upstream settings default off */
    lights_scale=1;press(B_SCL);tap(B_OCTUP);release(B_SCL);
    assert(!lights_scale && song.octave==octave);
    trk[1].p[P_CHORD]=1;trk[1].p[P_AHOLD]=0;
    fm1_in.notes=1u<<7;frame();
    press(B_SCL);tap(B_OCTUP);frames(50);
    assert(lights_scale && !trk[1].p[P_AHOLD]); /* gesture cancels SEL long-hold latch */
    release(B_SCL);fm1_in.notes=0;frame();
    /* Level changes alter only the dim guide plane, and survive side-record restore. */
    const uint16_t ns[]={4000,8000,16000,32000};
    song.sel=1;ui.layer=LY_PLAY;kb_grid=0;lights_scale=1;
    uint32_t guide_mask=keys_notes_dim();
    for(uint32_t level=0;level<4;level++) {
        scale_light_level=(uint8_t)level;ui_leds();
        assert(fm1_led_dim_ticks==ns[level]*FM1_TICKS_PER_US/1000&&keys_notes_dim()==guide_mask);
        fm1_in.notes=1u<<5;assert(keys_lit()&(1u<<5));fm1_in.notes=0;
        uint32_t saved=fork_settings_word();scale_light_level=0;fork_settings_from_word(saved);
        assert(scale_light_level==level);
        uint32_t canonical=lights_word()^(1u<<17);settings_restore_word(canonical);
        assert(scale_light_level==level); /* changing upstream visualizer preserves level */
    }
    scale_light_level=0;
    for(uint32_t level=1;level<4;level++){mi_set(MI_SCALELEVEL,1);assert(scale_light_level==level);}
    mi_set(MI_SCALELEVEL,1);assert(scale_light_level==3);
    mi_set(MI_SCALELEVEL,0);assert(!scale_light_level);
    mi_set(MI_SCALELEVEL,-1);assert(!scale_light_level);
    scale_light_level=3;lights_scale=0;ui_leds();assert(fm1_led_dim_ticks==4*FM1_TICKS_PER_US);
    lights_scale=1;ui.layer=LY_FX;ui_leds();assert(fm1_led_dim_ticks==4*FM1_TICKS_PER_US);
    ui.layer=LY_PLAY;song.sel=3;ui_leds();assert(fm1_led_dim_ticks==4*FM1_TICKS_PER_US);
    fork_settings_from_word(0);assert(!scale_light_level);
    puts("scale lights: physical toggle, all roots/scales, layer isolation and settings PASS");
    return 0;
}
