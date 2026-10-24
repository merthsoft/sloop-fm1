/* Real UI transactions and audition requests; no hardware required. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static void entry_reset(int advance)
{
    seq_stop(); song.sel = 0; song.rec = rec_wait = ft_on = 0;
    fm1_in.notes = fm1_in.buttons = 0;
    mi_held[0] = mi_held[1] = mi_held[2] = mi_held[3] = 0;
    step_in_r = step_in_w = 0;
    steps_clear(TSEL); TSEL->p[P_SLEN] = 16; TSEL->p[P_VOICE] = V_POLY;
    TSEL->p[P_CHORD] = 0;
    go_home(); open_family(FAM_SEQ); cursor_set(0);
    step_adv_off = !advance;
}
static void midi_event(uint32_t note, int on)
{
    step_in_q[step_in_w++ % STEP_IN_Q] = (uint16_t)(note | (on ? 128u : 0u));
    mi_ok = 1; midi_entry();
}
static void entry_transactions(void)
{
    for (int advance = 0; advance <= 1; advance++) {
        entry_reset(advance);
        /* Mixed panel/MIDI chord must not finish until both owners release. */
        fm1_in.notes = 1; seq_entry(1);
        midi_event(67, 1); assert(TSEL->step[0].n == 2);
        fm1_in.notes = 0; seq_entry(0);
        assert(ui.entry_open && ui.cursor == 0);
        midi_event(67, 0); seq_entry(0);
        assert(!ui.entry_open && ui.cursor == (uint32_t)advance);
        /* OFF starts a new replacement transaction, not an accidental append. */
        midi_event(72, 1); seq_entry(0);
        assert(TSEL->step[ui.cursor].n == 1 && TSEL->step[ui.cursor].note[0] == 72);
        midi_event(72, 0); seq_entry(0);
        assert(!ui.entry_open && ui.cursor == 2u * (uint32_t)advance);
        assert(undo_swap(0));
        assert(TSEL->step[advance].n == (advance ? 0 : 2));
        assert(undo_swap(1) && TSEL->step[advance].note[0] == 72);
    }
}
static void drum_turn(uint32_t knob)
{
    encs[panel.enc[knob]] = 1;
    drum_screen_input(0, 0, 0);
}
static void audition_preferences(void)
{
    seq_stop(); song.sel = TRK_DRUM; studio_open(SC_DRUM);
    TDRUM->p[P_SLEN] = 16;
    for (int on = 0; on <= 1; on++) {
        drum_preview = on; drum_page = 0; drum_lane = drum_cursor = 0;
        aud_lanes = 0; drum_turn(EN_K1);
        assert(drum_lane == 1 && !!aud_lanes == on);
        dstep_set(&TDRUM->dstep[1], 3, LV_HARD, 0);
        aud_lanes = 0; drum_turn(EN_K2);
        assert(drum_cursor == 1 && !!aud_lanes == on);
        drum_page = 2; drum_lane = 0;
        aud_lanes = 0; drum_turn(EN_K1);
        assert(drum_lane == 1 && !!aud_lanes == on);
    }
    /* Explicit grid entry and stopped TUNE audition stay available with preview OFF. */
    drum_preview = 0; drum_page = 0; drum_cursor = 0; drum_lane = 2;
    memset(&TDRUM->dstep[0], 0, sizeof(dstep_t)); aud_lanes = 0;
    grid_key(0); assert(aud_lanes == (1u << 2));
    drum_page = 2; drum_tune[2] = 0; aud_lanes = 0;
    drum_turn(EN_K2); assert(aud_lanes == (1u << 2));
    song.playing = 1; aud_lanes = 0; drum_turn(EN_K2);
    assert(!aud_lanes); seq_stop();
}
static void menus(void)
{
    int32_t total = 0;
    for (uint32_t s = 0; s < MS_COUNT; s++) total += text_w(&FONT_S, MS_TAB[s]);
    assert(total + 2 * (MS_COUNT - 1) <= 232);
    for (uint32_t s = 0; s < MS_COUNT; s++) {
        assert(MS_FIRST[s+1] > MS_FIRST[s] && MS_FIRST[s+1] - MS_FIRST[s] <= 4);
        ui.menu = 1; ui.menu_sel = MS_FIRST[s]; ui.force = 1; draw_menu();
    }
    step_adv_off = drum_preview = 0;
    mi_set(MI_ADVANCE, -1); assert(step_adv_off);
    mi_set(MI_ADVANCE, 1); assert(!step_adv_off);
    mi_set(MI_PREVIEW, 1); assert(drum_preview);
    mi_set(MI_PREVIEW, -1); assert(!drum_preview);
    for (uint32_t i = 0; i < 2; i++) {
        uint8_t *v = i ? &fxfade : &flt_reso;
        uint32_t item = i ? MI_FADE : MI_RESO, max = i ? 15 : 10;
        *v = 0; mi_set(item, -1); assert(!*v);
        for (uint32_t k = 0; k < max + 2; k++) mi_set(item, 1);
        assert(*v == max); mi_set(item, 0); assert(!*v);
    }
    ui.menu = 0; go_home();
    int16_t bpm = song.g[G_BPM];
    /* Immediate turn (< SHOW_MS) owns fade and must not become a tap. */
    press(B_FX); assert(ui.layer == LY_PLAY);
    encs[panel.enc[EN_SELECT]] = 1; frame();
    assert(fxfade == 1 && song.g[G_BPM] == bpm && settings_later && ui.layer == LY_FX);
    release(B_FX);
    assert(ui.layer == LY_PLAY && cur_page()->fam == FAM_TRK); /* no tap navigation to FX */
    /* Continued holds keep claiming subsequent SELECT detents. */
    press(B_FX); frames(10);
    encs[panel.enc[EN_SELECT]] = 1; frame();
    encs[panel.enc[EN_SELECT]] = 1; frame();
    assert(fxfade == 3 && song.g[G_BPM] == bpm);
    release(B_FX); assert(cur_page()->fam == FAM_TRK && ui.layer == LY_PLAY);
    /* Non-FX holds retain their existing SELECT tempo behavior. */
    press(B_ENV); frames(10);
    encs[panel.enc[EN_SELECT]] = 1; frame();
    assert(song.g[G_BPM] > bpm && fxfade == 3);
    release(B_ENV); fxfade = 0; song.g[G_BPM] = bpm; go_home();
}
static void section_badge(void)
{
    for(unsigned pal=0;pal<NPALETTES;pal++) {
        palette_set(pal);song.playing=1;arrangement_clock.running=1;
        for(unsigned scene=0;scene<4;scene++) {
            arrangement_clock.index=3;arrangement.entry[3].scene=scene;
            song.g[G_SLOT]=(scene+1)%4; /* editor selection is not the playing section */
            cv_begin(240,124,C_BLACK);cv_oy=0;graph_home();
            assert(cv_px[2*240+218]==swap16(TE_DIM[scene]));
        }
        arrangement_clock.running=0;cv_begin(240,124,C_BLACK);graph_home();
        assert(cv_px[2*240+218]==swap16(C_BLACK));
    }
    song.playing=0;palette_set(4);
}
int main(int argc, char **argv)
{
    panel = PANEL_DEFAULT; layers_init(); host_tracks_init(); palette_set(4);
    entry_transactions(); audition_preferences(); menus(); section_badge();
    if (argc > 1) {
        outdir=argv[1];palette_set(NPALETTES-1u);go_home();song.playing=1;arrangement_clock.running=1;arrangement_clock.index=0;arrangement.entry[0].scene=2;ui.force=1;frame();ppm("home-playing-section");song.playing=arrangement_clock.running=0;
        palette_set(NPALETTES - 1u);
        outdir = argv[1]; ui.menu = 1; ui.menu_sel = MI_RESO; ui.force = 1;
        draw_menu(); ppm("menu-fx-controls");
        ui.menu_sel = MI_ADVANCE; ui.force = 1; draw_menu(); ppm("menu-seq-controls");
    }
    puts("upstream controls: mixed MIDI/panel entry, preview and bounded menus pass");
    return 0;
}
