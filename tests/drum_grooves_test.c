/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#define PROJ_HOST 1
#include "../firmware/src/project.c"

int main(int argc, char **argv)
{
    uint32_t g, i, l;
    static track_t before, synth;
    static project_t saved;
    panel = PANEL_DEFAULT; layers_init(); host_tracks_init(); palette_set(4);
    song.sel = TRK_DRUM; studio_open(SC_DRUM); drum_page = 2;
    outdir = argc > 1 ? argv[1] : "build/host";
    for (g = 0; g < NDRUM_GROOVES; g++) {
        const drum_groove_t *p = &DRUM_GROOVES[g];
        check(p->len <= NSTEP && p->len > 0 && p->div < NDIV_STEP, "template length/division bounded");
        for (i = 0; i < NSTEP; i++) {
            dstep_t s = drum_groove_step(g, i);
            assert((dstep_mask(&s) & ~0x3Du) == 0);
            if (i >= p->len) assert(!dstep_mask(&s));
            for (l = 0; l < DRUM_LANES; l++) if (dstep_has(&s, l)) {
                assert(dstep_lvl(&s, l) <= LV_HARD && !dstep_rat(&s, l));
            }
        }
        groove_sel = (uint8_t)g; groove_screen_draw();
        for (i = 0; i < p->len; i += 16) {
            groove_cursor = (uint8_t)i; ui.force = 1; groove_screen_draw();
        }
    }
    assert(NDRUM_GROOVES == 16);
    assert(DRUM_GROOVES[14].len == 64 && DRUM_GROOVES[15].len == 32);
    { dstep_t last = drum_groove_step(14, 60), ghost = drum_groove_step(14, 42);
      assert(dstep_has(&last, 2) && dstep_lvl(&last, 2) == LV_HARD);
      assert(dstep_has(&ghost, 2) && dstep_lvl(&ghost, 2) == LV_GHOST);
      assert(drum_groove_apply(14) && TDRUM->p[P_SLEN] == 64);
      assert(dstep_has(&TDRUM->dstep[60], 2));
      assert(undo_swap(0));
    }
    groove_cursor = 0;
    check(1, "Amen fourth-bar hits, ghost levels, apply and undo");
    for (i = 0; i < 16; i++) {
        dstep_t s = drum_groove_step(0, i);
        assert(dstep_has(&s, 0) == (i % 4 == 0));
        assert(dstep_has(&s, 2) == (i == 4 || i == 12));
        if (i % 4 == 0) assert(dstep_lvl(&s, 0) == LV_HARD);
    }
    check(1, "four-floor kick/snare positions and accents");
    dstep_set(&TDRUM->dstep[63], 15, LV_SOFT, 3);
    TDRUM->micro[63] = -17; step_fill_set(TDRUM, 63, FC_FILL);
    assert(lock_set(TDRUM, 63, P_PAN, 19));
    TDRUM->p[P_SLEN] = 64; TDRUM->p[P_SDIV] = 3; TDRUM->p[P_SSWING] = 12;
    TDRUM->seq_active = 1;
    before = *TDRUM; synth = trk[0];
    encs[panel.enc[EN_K1]] = -99; drum_screen_input(0, 0);
    encs[panel.enc[EN_K2]] = 99; drum_screen_input(0, 0);
    encs[panel.enc[EN_K3]] = 99; drum_screen_input(0, 0);
    check(!memcmp(&before, TDRUM, sizeof before), "browsing changes no drum state");
    drum_screen_input(BT(B_OCTUP), 0);
    check(groove_confirm && !memcmp(&before, TDRUM, sizeof before), "existing content needs second apply press");
    drum_screen_input(BT(B_OCTDN), 0);
    check(!groove_confirm && drum_page == 0 && !memcmp(&before, TDRUM, sizeof before), "cancel preserves pattern and metadata");
    drum_page = 2; song.playing = 1;
    drum_screen_input(BT(B_OCTUP), 0);
    check(!groove_confirm && !strcmp(ui.msg, "STOP FIRST") && !memcmp(&before, TDRUM, sizeof before), "running apply refuses safely");
    song.playing = 0; transport_req = 1;
    check(!drum_groove_apply(0), "pending transport start refuses apply"); transport_req = 0;
    rec_wait = 1; check(!drum_groove_apply(0), "record arm refuses apply"); rec_wait = 0;
    groove_sel = 0;
    drum_screen_input(BT(B_OCTUP), 0); drum_screen_input(BT(B_OCTUP), 0);
    check(TDRUM->p[P_SLEN] == 16 && TDRUM->p[P_SDIV] == 2 && TDRUM->p[P_SSWING] == 0, "apply aligns length/division and removes local swing");
    for (i = 0; i < NSTEP; i++) {
        dstep_t s = drum_groove_step(0, i);
        assert(!memcmp(&s, &TDRUM->dstep[i], sizeof s));
        assert(!TDRUM->micro[i] && !step_fill(TDRUM, i));
    }
    for (i = 0; i < NLOCK; i++) assert(TDRUM->lock[i].step == LOCK_FREE);
    check(!memcmp(&synth, &trk[0], sizeof synth) && TDRUM->p[P_E0] == before.p[P_E0] && TDRUM->p[P_PAN] == before.p[P_PAN], "apply preserves synth and drum sound/mixer");
    assert(undo_swap(0));
    check(!memcmp(before.dstep, TDRUM->dstep, sizeof before.dstep) && !memcmp(before.micro, TDRUM->micro, sizeof before.micro) &&
          !memcmp(before.lock, TDRUM->lock, sizeof before.lock) && !memcmp(before.fill, TDRUM->fill, sizeof before.fill) &&
          before.p[P_SLEN] == TDRUM->p[P_SLEN] && before.p[P_SDIV] == TDRUM->p[P_SDIV] && before.p[P_SSWING] == TDRUM->p[P_SSWING], "one undo restores overwritten steps and full metadata");
    assert(undo_swap(1));
    before = *TDRUM;
    proj_capture(&saved); assert(proj_ok(&saved));
    memset(TDRUM->dstep, 0, sizeof TDRUM->dstep); proj_apply(&saved, 1);
    check(!undo_swap(0), "project adoption invalidates pre-load groove undo");
    check(!memcmp(before.dstep, TDRUM->dstep, sizeof before.dstep) && TDRUM->p[P_SLEN] == 16 && TDRUM->p[P_SDIV] == 2, "ordinary pattern project roundtrip");
    for (g = 0; g < NDRUM_GROOVES; g++) {
        groove_sel = (uint8_t)g; drum_page = 2; ui.force = 1; groove_screen_draw();
    }
    groove_sel = 0; ui.msg_t = 0; groove_cursor = 0; groove_lane = 0; groove_screen_draw(); ppm("live-groove");
    { uint32_t lit = 0; for (i = 180 * 240; i < 240 * 240; i++) lit += screen[i] != 0; check(lit > 100, "groove knob labels render below canvas split"); }
    check(1, "all groove screens fit framebuffer");
    undo_mark(TDRUM, (undo_sess += 4u) | 3u);
    check(!groove_undo_matches(), "later ordinary edit supersedes supplemental undo");
    song.sel = TRK_DRUM;
    open_family(FAM_SCL); drum_page = 0;
    tap(B_EDIT);
    check(on_drum_page() && drum_page == 0, "EDIT enters drums from a parameter page");
    open_family(FAM_SEQ);
    tap(B_SEQ);
    check(on_drum_page() && drum_page == 0, "SEQ enters drums from ordinary synth sequence pages");
    tap(B_SEQ);
    check(on_drum_page() && drum_page == 1, "second SEQ tap reaches KIT");
    tap(B_SEQ);
    check(on_drum_page() && drum_page == 2, "third SEQ tap reaches GROOVE rather than SONG");
    tap(B_SEQ);
    check(on_drum_page() && drum_page == 0, "fourth SEQ tap wraps to GRID");
    encs[panel.enc[EN_SELECT]] = 1; frame();
    check(on_drum_page() && drum_page == 1, "SELECT detent advances GRID to KIT");
    encs[panel.enc[EN_SELECT]] = 1; frame();
    check(on_drum_page() && drum_page == 2, "SELECT detent advances KIT to GROOVE");
    studio_open(SC_DRUM); drum_page = 1;
    tap(B_SEQ);
    check(drum_page == 2, "physical SEQ tap enters groove view from kit");
    tap(B_OCTUP);
    check(groove_confirm, "physical OCT+ requests replacement confirmation");
    tap(B_OCTDN);
    check(!groove_confirm && drum_page == 0, "physical OCT- cancels to grid");
    encs[panel.enc[EN_SELECT]] = 2; frame();
    check(drum_page == 2, "physical SELECT reaches groove view");
    tap(B_OCTUP); tap(B_OCTUP);
    check(!groove_confirm && groove_undo_matches(), "physical OCT+ applies on second press");
    groove_sel = 14; drum_page = 2; song.playing = 0; song.rec = 0; rec_wait = 0; transport_req = 0;
    proj_capture(&saved);
    tap(B_OCTDN);
    assert(groove_preview.active);
    groove_preview.phase = 0; groove_preview.step = 0; groove_preview.first = 1;
    for (i = 0; i < 65; i++) {
        dstep_t expected = drum_groove_step(14, i % 64);
        drums.hits = 0;
        groove_preview_block(0);
        assert(groove_preview.step == i % 64);
        assert(drums.hits == dstep_mask(&expected));
        groove_preview_block((div_units(2) + (uint32_t)song.g[G_BPM] - 1u) / (uint32_t)song.g[G_BPM]);
    }
    { project_t after; proj_capture(&after); assert(!memcmp(&saved, &after, sizeof after)); }
    assert(!song.playing && !song.rec);
    tap(B_OCTDN); assert(!groove_preview.active && drum_page == 2);
    check(1, "preview loops full Amen phrase with exact hits and no project or transport mutation");
    tap(B_OCTDN); tap(B_SEQ); assert(!groove_preview.active);
    drum_page = 2; tap(B_OCTDN); seq_stop(); assert(!groove_preview.active);
    tap(B_OCTDN); song.playing = 1; groove_preview_block(1); assert(!groove_preview.active); song.playing = 0;
    tap(B_OCTDN); proj_apply(&saved, 1); assert(!groove_preview.active);
    tap(B_OCTDN); encs[panel.enc[EN_K1]] = -1; frame(); assert(!groove_preview.active);
    tap(B_OCTDN); track_select(0); assert(!groove_preview.active);
    song.sel = TRK_DRUM; drum_page = 2; studio_open(SC_DRUM);
    song.playing = 1; tap(B_OCTDN); assert(!groove_preview.active); song.playing = 0;
    song.rec = 1; tap(B_OCTDN); assert(!groove_preview.active); song.rec = 0;
    check(1, "preview cancels on exit, STOP, playback, project adoption, selection and refuses recording");
    printf("drum grooves: %s\n", fails ? "FAIL" : "PASS");
    return fails;
}
