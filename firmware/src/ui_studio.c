/* SPDX-License-Identifier: GPL-3.0-only */
/* LIVE screens, 240 x 240, in a teenage-engineering-like style: black, four colours (one per
 * track and per knob: blue, green, yellow, orange), white for what you touch, red for recording,
 * big numbers, lowercase labels, four dials at the bottom that show what KNOB 1..4 do.
 * Screens: TRACKS (the performance view), DRUMS (GRID / KIT, pads that flash on each hit), the
 * LAYERS (a function button held: what the 16 white keys and the knobs do now), HOLD (a hold to
 * confirm: clear, save), REC (armed / free take). Every band is drawn into the canvas only when its
 * signature changed. */
static uint8_t drum_page, drum_lane, drum_cursor;   /* 0 GRID, 1 KIT, 2 SOUND, 3 GROOVE, 4 RHYTHM */
static uint8_t groove_sel, groove_cursor, groove_lane, groove_confirm, groove_hold_sel;
static uint32_t groove_hold_t0;
static void trk_short_name(uint32_t c, char *b);
UI_SMALL static int on_drum_page(void) { return !ui.home && cur_page()->scope == SC_DRUM; }

/* ---------------------------------------------------------------- style --- */
/* TE_G1 .. TE_G4: the greys (tiles, empty steps, labels, secondary text), gfx.c (2.7: on a theme's background) */
#define TE_RED RGB(255, 44, 52)          /* recording, erasing */
/* TE_COL, TE_MID, TE_DIM: the tracks' colours, gfx.c (2.7: a theme may set them) */
#define TE_DRUM TE_COL[3]

UI_SMALL static void te_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)     /* filled circle */
{
    int32_t y, x;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r)
                cv_pset(cx + x, cy + y, c);
}
UI_SMALL static uint32_t studio_hash(uint32_t h, const char *p)
{ while (*p) h = h * 31u + (uint8_t)*p++; return h; }
UI_SMALL static void te_text_c(int32_t cx, int32_t y, const char *s, uint16_t c)   /* centred on cx */
{
    cv_text_s(cx - text_w_s( s) / 2, y, s, c);
}

/* the dial strip: KNOB 1..4, label + value under each (a dial with no label: an empty column); a
 * message replaces it */
UI_SMALL static void te_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4],
                     uint32_t sig, uint32_t *cache)
{
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]);
        sig = studio_hash(sig, val[k]);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_band(44);
    for (k = 0; k < 4u; k++)
        ui_card(2+60*(int32_t)k,0,lab[k],val[k],ratio[k],TE_COL[k],ui.hot_t&&ui.hot_col==k);
    /* Four cards replace the old ring and value passes. */
    cv_band_blit(26);

}

UI_SMALL static void studio_open(uint32_t scope)
{
    groove_preview.active = 0;
    uint32_t i;
    if (scope == SC_SONG && rec_wait)
        rec_wait = 0;                                   /* (an arm does not follow into the song page) */
    for (i = 0; i < NPAGES; i++) if (PAGES[i].scope == scope) {
        ui.page = (uint8_t)i; ui.home = 0; song.seq_mode = 0; ui.force = 1;
        ui.msg_t = 0; ui.entry_open = 0; return;
    }
}
/* LIVE: REC, on any page. One record arm, on the selected track (it follows ALGORITHM). No
 * click (seq.c): playing, recording starts at once; stopped, REC arms and the first note
 * starts the loop (or, in an empty project, a free take that REC closes: seq.c ft_close).
 * REC again stops recording / cancels the arm (playback continues). */
UI_SMALL static void rec_toggle(void)
{
    if (ft_owns_press())
        return;
    if (song.playing && arrangement_enabled) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    if (song.rec || rec_wait) {
        song.rec = 0;
        rec_wait = 0;
        ui_message("REC OFF");
        return;
    }
    arrangement_enabled = 0;
    if (song.playing)
        rec_begin();                    /* playing: record now */
    else
        rec_wait = 1;                   /* stopped: the first note starts it */
}

/* --------------------------------------------------------------- TRACKS --- */
/* swing, MPC style: "62%" */
UI_SMALL static void swing_str(char *b, int32_t v)            /* 0 straight .. 100 the most (params.c F_SWING) */
{
    fmt_int(b, clamp(v, 0, 100));
}

/* A short stepped decay per quarter-note; stopped keeps the original scene.
 * Use the musical clock, not wall time, so external sync and tempo changes follow. */
UI_SMALL static uint32_t home_sun_light(void)
{
    return song.playing ? 16u-(clk_pos>>19) : 16u;
}

/* Scene accents advance only on beats already redrawn by the sun pulse. */
UI_SMALL static uint32_t home_scene_step(void) { return song.playing?clk_beat:0u; }

UI_SMALL static void studio_tracks_draw(void)
{
    static uint32_t body,footer;
    uint32_t sig=song.sel+song.solo*7u+song.rec*31u+home_sun_light()*7919u+home_scene_step()*104729u;
    char name[16];
    if(ui.force) draw_frame();
    draw_head();
    {   /* KNOB 1 swing (the global groove), 2 level, 3 steps, 4 pan of the selected track */
        track_t *t = TSEL;
        static char v[4][8];
        const char *lab[4] = {swing_eighth?"swing8":"swing", "level", "steps", "pan"};
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        int32_t ratio[4];
        uint32_t lvl = is_drum(t) ? song.g[G_DRLVL] : t->p[P_LEVEL];
        swing_str(v[0], song.g[G_SWING]);
        fmt_int(v[1], t->p[P_MUTE] ? 0 : (int32_t)lvl * 100 / 127);
        fmt_int(v[2], t->p[P_SLEN]);
        fmt_int(v[3], t->p[P_PAN]);
        ratio[0] = song.g[G_SWING] * 10;
        ratio[1] = t->p[P_MUTE] ? 0 : (int32_t)lvl * 1000 / 127;
        ratio[2] = (t->p[P_SLEN] - 1) * 1000 / 63;
        ratio[3] = (t->p[P_PAN] + 64) * 1000 / 127;

        te_dials(lab, val, ratio, song.sel, &footer);

    }
    for(uint32_t r=0;r<NTRK;r++) sig=sig*31u+steps_hash(&trk[r])+trk[r].seq_idx*71u+
        trk[r].p[P_MUTE]*101u+trk_level(r)*17u;
    sig+=song.playing+up_gen+TSEL->preset*131u+TSEL->eng_req*104729u+TSEL->user*65537u+
        (is_drum(TSEL)?drum_kit()*997u:0u);
    sig=sig*31u+song.g[G_FILT]+song.g[G_LTYPE]*131u+song.g[G_DUCK]*997u;
    for(uint32_t i=0;i<NPART;i++)sig=sig*31u+duck_bypass[i];
    if(ui.force||sig!=body) {
        body=sig;cv_band(124);graph_home();cv_band_blit(74);
        cv_band(38);
        trk_short_name(song.sel,name);
        int32_t x=cv_text_c_white(4,0,name);
        cv_text_s(x+8,0,is_drum(TSEL)?"DRUMS":ENGINES[TSEL->eng_req%NENGINES]->name,TE_COL[song.sel]);
        cv_text_te_g4(4,20,"PRESETS: SOUND / ALGO: TRACK");
        cv_band_blit(202);
    }
}

/* ---------------------------------------------------------------- DRUMS --- */
static const char *const LV_NAME[4] = {"norm", "ghost", "soft", "hard"};
UI_SMALL static uint32_t lvl_rank(uint32_t lvl);                 /* (below) ghost 0, soft 1, norm 2, hard 3 */
UI_SMALL static uint16_t lvl_col(uint32_t lvl)                   /* a hit's colour by its level */
{
    return lvl == LV_GHOST ? TE_DIM[3] : lvl == LV_SOFT ? TE_MID[3] : lvl == LV_HARD ? C_WHITE : TE_DRUM;
}
static uint16_t pad_lit[DRUM_LANES];                   /* pads and key LEDs: frames left lit */
static uint8_t key_lit[27];                             /* menu NOTES, a synth track: frames its key stays lit */
static uint8_t key_lit_trk = 0xFF;                      /* (the track key_lit is for) */
UI_SMALL static void pads_tick(void)                             /* once a frame: the hits since the last one */
{
    uint32_t i, hits, n[NPART][4], k, w;
    fm1_irq_off();
    hits = drums.hits;
    drums.hits = 0;
    for (i = 0; i < NPART; i++)
        for (w = 0; w < 4u; w++) {
            n[i][w] = note_hits[i][w];
            vis_note_hits[i][w] |= n[i][w];
            note_hits[i][w] = 0;
        }
    fm1_irq_on();
    for (i = 0; i < DRUM_LANES; i++) {
        if ((hits >> i) & 1u) pad_lit[i] = 6;
        else if (pad_lit[i]) pad_lit[i]--;
    }
    /* a synth track: each note it started lights its key 6 frames (~0.1 s), so a short sequencer note
     * (a 1/16 at GATE 50 %: 60..80 ms) is seen even when it ends between two frames */
    if (song.sel != key_lit_trk) {
        memset(key_lit, 0, sizeof key_lit);
        key_lit_trk = song.sel;
    }
    for (k = 0; k < 27u; k++)
        if (key_lit[k])
            key_lit[k]--;
    if (song.sel < NPART && (n[song.sel][0] | n[song.sel][1] | n[song.sel][2] | n[song.sel][3]))
        for (k = 0; k < 27u; k++) {
            uint32_t note = kb_map(TSEL, k);
            if (note < 128u && (n[song.sel][note >> 5] >> (note & 31u)) & 1u)
                key_lit[k] = 6;
        }
}

UI_SMALL static void groove_screen_draw(void)
{
    static uint32_t cache,cards;
    const drum_groove_t *p=&DRUM_GROOVES[groove_sel];
    const char *lab[4]={"GROOVE","STEP","SOUND","HEAR"};
    char v[4][12],b[24];const char *val[4]={v[0],v[1],v[2],v[3]};
    int32_t ratio[4]={-1,-1,-1,-1};uint32_t i,k,sig;
    if(ui.force) draw_frame();draw_head();
    fmt_int(v[0],groove_sel+1);fmt_int(v[1],groove_cursor+1);
    str_cpy(v[2],LANE_SHORT[groove_lane],12);str_cpy(v[3],"TURN",12);
    if(drum_page==4) {
        lab[0]="TARGET";lab[1]="SHIFT";lab[2]="SYNCOP";lab[3]="FEEL";
        str_cpy(v[0],groove_shape.part==RHYTHM_PART_ALL?"ALL":LANE_SHORT[groove_shape.part],12);
        fmt_int(v[1],groove_shape.offset);fmt_int(v[2],groove_shape.sync);fmt_int(v[3],groove_shape.feel);
    }

    te_dials(lab,val,ratio,drum_page,&cards);
    sig=groove_sel+groove_cursor*13u+groove_lane*257u+groove_confirm*8191u+groove_preview.active*65537u;
    sig+=drum_page*7919u+(uint32_t)(groove_shape.offset+64)*31u+groove_shape.part*71u+
        groove_shape.sync*239u+(uint32_t)(groove_shape.feel+32)*101u;
    if(!ui.force&&sig==cache) return;
    cache=sig;
    cv_band(124);
    cv_text_c_white(4,0,p->name);
    fmt_int(b,p->len);str_cpy(b+str_len(b)," STEPS",sizeof b-str_len(b));
    cv_text_te_g4(4,18,b);cv_text_te_g4(96,18,N_SDIV[p->div]);
    if(groove_preview.active) cv_text_s(164,18,"PREVIEW",TE_DRUM);
    for(k=0;k<5;k++) {
        static const uint8_t lanes[5]={0,2,3,4,5};
        cv_text_te_g4(4,42+(int32_t)k*14,LANE_SHORT[lanes[k]]);
        for(i=0;i<16&&(groove_cursor/16u)*16u+i<p->len;i++) {
            uint32_t step=(groove_cursor/16u)*16u+i;
            dstep_t st=drum_groove_shaped_step(groove_sel,step,&groove_shape);
            cv_rect(52+(int32_t)i*11,45+(int32_t)k*14,9,9,
                dstep_has(&st,lanes[k])?lvl_col(dstep_lvl(&st,lanes[k])):TE_G1);
            if(step==groove_cursor&&lanes[k]==groove_lane)
                cv_rect_c_white(52+(int32_t)i*11,44+(int32_t)k*14,9,1);
        }
    }
    cv_band_blit(74);cv_band(38);
    cv_text_c_white(4,0,drum_page==4?"RHYTHM / T4":"GROOVE / T4");
    cv_text_te_g4(4,20,groove_confirm?"OCT+ AGAIN / HOLD":groove_preview.active?
        "OCT- STOP / OCT+ APPLY":"OCT- LISTEN / OCT+ APPLY");
    cv_band_blit(202);
}

UI_SMALL static void drum_screen_draw(void)
{
    static uint32_t title_sig, body_sig, footer;
    uint32_t i, j, len = (uint32_t)clamp(TDRUM->p[P_SLEN], 1, 64), kit = drum_kit(), sig, bank;
    char st[16];
    if(drum_page==5) {actions_draw();return;}
    if (drum_page >= 3) { groove_screen_draw(); return; }
    if (drum_cursor >= len) drum_cursor = (uint8_t)(len - 1u);
    bank = drum_cursor / 16u;
    {
        char nm[9];
        const char *style = DRUM_KIT_STYLES[kit];
        if (kit >= DRUM_SYN) {                          /* SYN1..4: the name you gave it (editor DRUM SYNTH) */
            memcpy(nm, dsu_kit(kit - DRUM_SYN)->name, 8);
            nm[8] = 0;
            if (nm[0])
                style = nm;
        }
        str_cpy(st, style, sizeof st);
        st[12] = 0;
        draw_head();
    }
    sig = kit * 131u + drum_page * 7u + drum_lane * 977u + bank * 31u + len + (uint32_t)drum_tweaked() * 7919u;   /* title band */
    if (ui.force || sig != title_sig) {
        char b[8];
        title_sig = sig;
        cv_band(24);
        fmt_int(b,(int32_t)kit+1);
        cv_text_s(4,2,b,TE_DRUM);
        cv_text_c_white(36,2,DRUM_KIT_NAMES[kit]);
        cv_band_blit(74);
        cv_band(38);
        cv_text_s(4,0,drum_page==0?"DRUM GRID":drum_page==1?"DRUM KIT":"DRUM SOUND",TE_DRUM);
        cv_text_te_g4(236-text_w_s(st),0,st);
        cv_text_te_g4(4,20,"SELECT: PAGE / PRESETS: KIT");
        cv_band_blit(202);
    }
    sig = drum_page + drum_lane * 7u + drum_cursor * 101u + song.playing * 71u + len * 3u;
    for (i = 0; i < DRUM_LANES; i++) sig = sig * 5u + (uint32_t)(drum_tune[i] + 8) * 9u + (uint32_t)(drum_dec[i] + 4);
    if (song.playing) sig = sig * 31u + TDRUM->seq_idx;
    for (i = 0; i < DRUM_LANES; i++) sig = sig * 3u + (pad_lit[i] != 0);
    for (i = 0; i < len; i++) {
        const dstep_t *s = &TDRUM->dstep[i];
        sig = sig * 31u + dstep_mask(s);
        sig = sig * 31u + s->lvl[0] + s->lvl[1] * 7u + s->lvl[2] * 49u + s->lvl[3] * 343u;
        sig = sig * 31u + s->rat[0] + s->rat[1] * 7u + s->rat[2] * 49u + s->rat[3] * 343u;
    }
    if (ui.force || sig != body_sig) {
        body_sig = sig;
        cv_band(100);
        if (!drum_page) {                              /* GRID: the 16 lanes x 16 steps of this bank */
            for (i = 0; i < DRUM_LANES; i++) {
                int32_t y = (int32_t)i * 6 + 2;
                cv_rect(2, y, 6, 5, pad_lit[i] ? C_WHITE : i == drum_lane ? TE_DRUM : TE_G2);
                for (j = 0; j < 16u; j++) {
                    uint32_t p = bank * 16u + j;
                    const dstep_t *s = &TDRUM->dstep[p < NSTEP ? p : 0];
                    uint16_t c;
                    if (p >= len) continue;
                    c = dstep_has(s, i) ? lvl_col(dstep_lvl(s, i)) : i == drum_lane ? TE_G2 : TE_G1;
                    if (song.playing && p == TDRUM->seq_idx && c == TE_G1) c = TE_G2;
                    cv_rect(12 + (int32_t)j * 14, y, 12, 5, c);
                    if (dstep_has(s, i) && dstep_rat(s, i)) {   /* a ratchet: a notch per extra hit */
                        uint32_t r;
                        for (r = 0; r < dstep_rat(s, i); r++)
                            cv_rect_c_black(13 + (int32_t)j * 14 + (int32_t)r * 3, y + 2, 2, 1);
                    }
                    if (p == drum_cursor && i == drum_lane) {
                        cv_rect_c_white(11 + (int32_t)j * 14, y - 1, 14, 1);
                        cv_rect_c_white(11 + (int32_t)j * 14, y + 5, 14, 1);
                    }
                }
            }
        } else {                                       /* KIT: 16 pads, lit on each hit; SOUND (2.7): the one KNOB 1 picks
                                                        * framed, a tuned or shortened / lengthened one marked */
            for (i = 0; i < DRUM_LANES; i++) {
                int32_t x = 2 + (int32_t)(i % 4u) * 60, y = (int32_t)(i / 4u) * 25;
                cv_rect(x, y, 56, 23, pad_lit[i] ? TE_DRUM : TE_G1);
                if (drum_page == 2u && i == drum_lane) {
                    cv_rect_c_white(x, y, 56, 1), cv_rect_c_white(x, y + 22, 56, 1);
                    cv_rect_c_white(x, y, 1, 23), cv_rect_c_white(x + 55, y, 1, 23);
                }
                if (drum_page == 2u && (drum_tune[i] || drum_dec[i]))
                    cv_rect(x + 3, y + 3, 4, 4, TE_DRUM);
                te_text_c(x + 28, y + 4, LANE_SHORT[i], pad_lit[i] ? C_BLACK : i == drum_lane ? C_WHITE : TE_G3);
            }
        }
        cv_band_blit(98);
    }
    {
        static char v[4][12];
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        static const char *const LG[4] = {"sound", "step", "hit", "level"}, *const LK[4] = {"kit", "level", "reverb", "pan"},
                                 *const LS[4] = {"sound", "tune", "decay", "level"};
        int32_t ratio[4];
        if (!drum_page) {
            const dstep_t *s = &TDRUM->dstep[drum_cursor];
            str_cpy(v[0], LANE_SHORT[drum_lane], 8);
            fmt_int(v[1], drum_cursor + 1);
            str_cpy(v[2], dstep_has(s, drum_lane) ? "on" : "--", 4);
            str_cpy(v[3], dstep_has(s, drum_lane) ? LV_NAME[dstep_lvl(s, drum_lane)] : "--", 8);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1);
            ratio[1] = (int32_t)drum_cursor * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
            ratio[2] = v[2][0] == 'o' ? 1000 : 0;
            ratio[3] = dstep_has(s, drum_lane) ? (int32_t)lvl_rank(dstep_lvl(s, drum_lane)) * 333 : 0;   /* ghost left ..
                                                                     * hard right (2.5: it followed the stored order) */
            te_dials( LG, val, ratio, 1u, &footer);
        } else if (drum_page == 2u) {                  /* SOUND (2.7): the sound, its TUNE and DECAY, the drums' level */
            str_cpy(v[0], LANE_SHORT[drum_lane], 8);
            fmt_int(v[1], drum_tune[drum_lane]);
            if (drum_tune[drum_lane] > 0) { v[1][0] = '+'; fmt_int(v[1] + 1, drum_tune[drum_lane]); }
            fmt_int(v[2], drum_dec[drum_lane]);
            if (drum_dec[drum_lane] > 0) { v[2][0] = '+'; fmt_int(v[2] + 1, drum_dec[drum_lane]); }
            fmt_int(v[3], song.g[G_DRLVL] * 100 / 127);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1);
            ratio[1] = (drum_tune[drum_lane] - DRUM_TUNE_MIN) * 1000 / (DRUM_TUNE_MAX - DRUM_TUNE_MIN);
            ratio[2] = (drum_dec[drum_lane] - DRUM_DEC_MIN) * 1000 / (DRUM_DEC_MAX - DRUM_DEC_MIN);
            ratio[3] = song.g[G_DRLVL] * 1000 / 127;
            te_dials( LS, val, ratio, 3u, &footer);
        } else {
            fmt_int(v[0], (int32_t)kit + 1);
            fmt_int(v[1], song.g[G_DRLVL] * 100 / 127);
            fmt_int(v[2], song.g[G_DRREV] * 100 / 127);
            fmt_int(v[3], TDRUM->p[P_PAN]);
            ratio[0] = (int32_t)kit * 1000 / (int32_t)(DRUM_KITS - 1u);
            ratio[1] = song.g[G_DRLVL] * 1000 / 127;
            ratio[2] = song.g[G_DRREV] * 1000 / 127;
            ratio[3] = (TDRUM->p[P_PAN] + 64) * 1000 / 127;
            te_dials( LK, val, ratio, 2u, &footer);
        }
    }
}

/* the level order of the knobs: ghost < soft < norm < hard */
static const uint8_t LV_UP[4] = {LV_GHOST, LV_SOFT, LV_NORM, LV_HARD};
UI_SMALL static uint32_t lvl_rank(uint32_t lvl) { return lvl == LV_GHOST ? 0u : lvl == LV_SOFT ? 1u : lvl == LV_NORM ? 2u : 3u; }

/* the GRID page shown, no layer held: its keys are steps (seq.c kb_grid) */
UI_SMALL static int grid_keys_on(void) { return on_drum_page() && !drum_page && song.sel == TRK_DRUM && !ui.menu; }
/* a key down on the GRID page: the white keys are the 16 steps of this page of steps for the sound of
 * KNOB 1 — an empty one is set (NORM, the sound heard), a set one cleared; the first four black keys
 * pick the page (steps 1-16, 17-32, 33-48, 49-64), as in the SEQ layer. The cursor follows */
UI_SMALL static void grid_key(uint32_t k)
{
    static const int8_t PG[12] = {-1, 0, -1, 1, -1, 2, -1, -1, 3, -1, -1, -1};
    uint32_t len = trk_len(TDRUM), idx;
    int32_t w = punch_key(k);
    dstep_t *st;
    if (w < 0) {
        if (k < 12u && PG[k] >= 0 && (uint32_t)PG[k] * 16u < len) {
            idx = (uint32_t)PG[k] * 16u + drum_cursor % 16u;
            drum_cursor = (uint8_t)(idx < len ? idx : len - 1u);
            ui.force = 1;
        }
        return;
    }
    idx = drum_cursor / 16u * 16u + (uint32_t)w;
    if (idx >= len)
        return;
    drum_cursor = (uint8_t)idx;
    if (song.playing && arrangement_enabled) {
        ui_message("STOP THE SONG FIRST");
        return;
    }
    undo_mark(TDRUM, ui.step_sess ? ui.step_sess : (ui.step_sess = (undo_sess += 4u) | 3u));
    st = &TDRUM->dstep[idx];
    fm1_irq_off();
    if (dstep_has(st, drum_lane)) {
        dstep_clr(st, drum_lane);
        if (!dstep_mask(st)) {                          /* an empty step keeps no lock, no nudge, no condition */
            lock_del(TDRUM, idx, P_COUNT);
            TDRUM->micro[idx] = 0;
            step_fill_set(TDRUM, idx, FC_NORM);
        }
    } else {
        dstep_set(st, drum_lane, LV_NORM, 0);
        TDRUM->seq_active = 1;
    }
    fm1_irq_on();
    if (dstep_has(st, drum_lane))
        audition_lane(drum_lane);
    sync_reload = 1;
}

UI_SMALL static void drum_screen_input(uint32_t pressed, uint32_t home, uint32_t notes)
{
    uint32_t k, b;
    int32_t s;
    if (song.sel != TRK_DRUM || home == 1u) {
        groove_confirm = 0;
        go_home();
        return;
    }
    for (k = 0; k < NB; k++) if ((pressed >> panel.btn[k]) & 1u) {
        b = k;
        if (b == B_PLAY) {
            if (ft_owns_press()) ;
            else if (!song.playing && arrangement_enabled && !arr_valid(&arrangement, arrangement_ready())) ui_message("EMPTY SECTION: REC");
            else transport_req = song.playing ? 2 : 1;
        } else if (b == B_SEQ || b == B_EDIT) {
            groove_preview.active = 0;
            drum_page = (uint8_t)((drum_page + 1u) % 6u);
            if(drum_page==5){groove_preview.active=0;actions_open(1,drum_lane,drum_cursor);return;}
            groove_confirm = 0;
            ui.msg_t = 0;
            ui.force = 1;
        } else if (b == B_SAVE) {
            studio_open(SC_SONG);
            return;
        } else if (drum_page >= 3 && b == B_OCTDN) {
            if (groove_confirm) {
                groove_confirm = 0; groove_preview.active = 0; drum_page = 0;
            } else if (groove_preview.active) {
                groove_preview.active = 0;
            } else if (song.playing || transport_req || song.rec || rec_wait || ft_on) {
                ui_message("STOP FIRST");
            } else {
                const drum_groove_t *p = &DRUM_GROOVES[groove_sel];
                fm1_irq_off();
                for(uint32_t lane=0;lane<7;lane++) groove_preview.mask[lane]=drum_groove_mask(p,lane);
                groove_preview.len = p->len; groove_preview.div = p->div;
                groove_preview_id = groove_sel; groove_preview_read = drum_groove_preview_step;
                groove_preview.feel=groove_shape.feel;
                groove_preview.phase = -(int32_t)groove_shape.feel * (int32_t)swing_step_units(p->div,song.g[G_SWING],0) / 64;
                groove_preview.step = 0; groove_preview.first = 1;
                groove_preview.active = 1;
                fm1_irq_on();
            }
            ui.force = 1;
        } else if (drum_page >= 3 && b == B_OCTUP) {
            groove_hold_t0 = 0;
            groove_preview.active = 0;
            if (song.playing || transport_req == 1 || song.rec || rec_wait || ft_on) {
                groove_confirm = 0; ui_message("STOP FIRST");
            } else if (!groove_confirm && drum_groove_has_content()) {
                groove_confirm = 1; ui.msg_t = 0; ui.force = 1;
                groove_hold_t0 = fm1_ms | 1u; groove_hold_sel = groove_sel;
            } else if (drum_groove_apply_shaped(groove_sel, &groove_shape)) {
                groove_confirm = 0; drum_cursor = 0; ui_message("GROOVE APPLIED");
            }
        }
    }
    if ((s = panel_enc(EN_SELECT))) {   /* SELECT: GRID / KIT / GROOVE */
        groove_preview.active = 0;
        drum_page = (uint8_t)clamp((int32_t)drum_page + clamp(s,-5,5), 0, 5);
        if(drum_page==5){groove_preview.active=0;actions_open(1,drum_lane,drum_cursor);return;}
        groove_confirm = 0;
        ui.msg_t = 0;
        ui.force = 1;
    }
    if (drum_page == 2u && notes) {                    /* SOUND (2.7): the key played picks its sound (it is heard
                                                        * as the keys always play it) */
        for (k = 0; k < 32u; k++)
            if ((notes >> k) & 1u) {
                drum_lane = (uint8_t)lane_of_key(k);
                ui.force = 1;
            }
    }
    if ((s = panel_enc(EN_ALGO)) && !ft_on) {
        track_select((uint32_t)clamp((int32_t)song.sel + s, 0, 3));
        if (song.sel != TRK_DRUM) {
            go_home();
            return;
        }
    }
    if ((s = panel_enc(EN_PRESET))) TDRUM->p[P_E0] = (int16_t)clamp(TDRUM->p[P_E0] + s, 0, DRUM_KITS - 1);
    for (k = 0; k < 4u; k++) if ((s = panel_enc(EN_K1 + k))) {
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
        if (drum_page == 4) {
            groove_preview.active = 0; groove_confirm = 0; groove_hold_t0 = 0; ui.msg_t = 0;
            if (k == 0) {
                int32_t part = groove_shape.part == RHYTHM_PART_ALL ? 0 : groove_shape.part + 1;
                part = clamp(part + s, 0, DRUM_LANES);
                groove_shape.part = part ? (uint8_t)(part - 1) : RHYTHM_PART_ALL;
            }
            if (k == 1) groove_shape.offset = (int8_t)clamp((int32_t)groove_shape.offset + s, -63, 63);
            if (k == 2) groove_shape.sync = (uint8_t)clamp((int32_t)groove_shape.sync + s, 0, 2);
            if (k == 3) groove_shape.feel = (int8_t)clamp((int32_t)groove_shape.feel + s, -32, 31);
        } else if (drum_page == 3) {
            if (k == 0) {
                groove_preview.active = 0;
                groove_sel = (uint8_t)clamp((int32_t)groove_sel + s, 0, NDRUM_GROOVES - 1);
                groove_cursor = (uint8_t)clamp(groove_cursor, 0, DRUM_GROOVES[groove_sel].len - 1);
                groove_confirm = 0; ui.msg_t = 0;
            }
            if (k == 1) groove_cursor = (uint8_t)clamp((int32_t)groove_cursor + s, 0, DRUM_GROOVES[groove_sel].len - 1);
            if (k == 2) groove_lane = (uint8_t)clamp((int32_t)groove_lane + s, 0, DRUM_LANES - 1);
            if (k == 3) {
                dstep_t st = drum_groove_step(groove_sel, groove_cursor), one = {{0}, {0}, {0}};
                if (dstep_has(&st, groove_lane)) dstep_set(&one, groove_lane, dstep_lvl(&st, groove_lane), 0);
                audition_step(&one);
            }
        } else if (!drum_page) {
            dstep_t *st = &TDRUM->dstep[drum_cursor];
            if (k == 0) {                              /* the sound: heard */
                uint8_t l = (uint8_t)clamp(drum_lane + s, 0, DRUM_LANES - 1);
                if (drum_preview && l != drum_lane) audition_lane(l);
                drum_lane = l;
            }
            if (k == 1) {                              /* the step: what it holds, heard */
                uint8_t c = (uint8_t)clamp(drum_cursor + s, 0, TDRUM->p[P_SLEN] - 1);
                if (drum_preview && c != drum_cursor) audition_step(&TDRUM->dstep[c]);
                drum_cursor = c;
            }
            if (k >= 2) {
                if (song.playing && arrangement_enabled) { ui_message("STOP THE SONG FIRST"); continue; }
                undo_mark(TDRUM, ui.step_sess ? ui.step_sess : (ui.step_sess = (undo_sess += 4u) | 3u));
                fm1_irq_off();
                if (k == 2) {
                    if (s > 0) dstep_set(st, drum_lane, LV_NORM, 0);
                    else dstep_clr(st, drum_lane);
                } else if (dstep_has(st, drum_lane)) {
                    uint32_t r = (uint32_t)clamp((int32_t)lvl_rank(dstep_lvl(st, drum_lane)) + (s > 0 ? 1 : -1), 0, 3);
                    dstep_set(st, drum_lane, LV_UP[r], dstep_rat(st, drum_lane));
                }
                fm1_irq_on();
                sync_reload = 1;
            }
        } else if (drum_page == 2u) {                  /* SOUND (2.7) */
            if (k == 0) {
                uint8_t l = (uint8_t)clamp(drum_lane + s, 0, DRUM_LANES - 1);
                if (drum_preview && l != drum_lane) audition_lane(l);
                drum_lane = l;
            }
            if (k == 1 || k == 2) {                    /* TUNE, DECAY: the next hit has them (heard at once) */
                int8_t *p = k == 1 ? &drum_tune[drum_lane] : &drum_dec[drum_lane];
                int32_t lo = k == 1 ? DRUM_TUNE_MIN : DRUM_DEC_MIN, hi = k == 1 ? DRUM_TUNE_MAX : DRUM_DEC_MAX;
                int8_t nv = (int8_t)clamp(*p + s, lo, hi);
                if (nv != *p) {
                    *p = nv;
                    if (!song.playing)
                        audition_lane(drum_lane);
                }
            }
            if (k == 3) song.g[G_DRLVL] = (int16_t)clamp(song.g[G_DRLVL] + s, 0, 127);
        } else {
            if (k == 0) TDRUM->p[P_E0] = (int16_t)clamp(TDRUM->p[P_E0] + s, 0, DRUM_KITS - 1);
            if (k == 1) song.g[G_DRLVL] = (int16_t)clamp(song.g[G_DRLVL] + s, 0, 127);
            if (k == 2) song.g[G_DRREV] = (int16_t)clamp(song.g[G_DRREV] + s, 0, 127);
            if (k == 3) TDRUM->p[P_PAN] = (int16_t)clamp(TDRUM->p[P_PAN] + s, -64, 63);
        }
    }
    if (!(fm1_in.buttons & (1u << panel.btn[B_OCTUP])) || drum_page < 3 ||
        !groove_confirm || groove_hold_sel != groove_sel) groove_hold_t0 = 0;
    if (groove_hold_t0 && fm1_ms - (groove_hold_t0 & ~1u) >= 700u) {
        groove_hold_t0 = 0;
        if (song.playing || transport_req == 1 || song.rec || rec_wait || ft_on) {
            groove_confirm = 0; ui_message("STOP FIRST");
        } else if (drum_groove_apply_shaped(groove_sel, &groove_shape)) {
            groove_confirm = 0; drum_cursor = 0; ui_message("GROOVE APPLIED");
        }
    }

}

/* ---------------------------------------------------------------- REC --- */
/* a big seven-segment digit, TE style: w x h, segments t thick */
UI_SMALL static void te_rec_digit(int32_t x,uint32_t d)
{
    static const uint8_t SEG[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
    static const uint8_t R[7][4]={{7,0,18,7},{25,7,7,21},{25,28,7,21},{7,49,18,7},{0,28,7,21},{0,7,7,21},{7,25,18,7}};
    uint32_t m=SEG[d%10u];
    for(uint32_t i=0;i<7u;i++) if(m & (1u<<i))
        cv_rect_c_white(x+R[i][0],4+R[i][1],R[i][2],R[i][3]);
}

static uint8_t rec_shown;
/* the four tracks, compact, from y0: the one that records is framed in red */
UI_SMALL static void rec_rows(uint32_t rt, uint32_t take, uint32_t y0, uint32_t *cache)
{
    uint32_t sig = rt * 3u + rec_wait + take * 5u + drum_kit() * 977u + y0, i, j;
    for (i = 0; i < NTRK; i++) {
        char b[16];
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        sig = studio_hash(sig, b) + (uint32_t)trk[i].p[P_SLEN] * 31u;
        for (j = 0; j < NSTEP; j++) sig = sig * 3u + (uint32_t)trk_step_on(&trk[i], j);
    }
    if (!ui.force && sig == *cache)
        return;
    *cache = sig;
    cv_band(76);
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, 64), armed = i == rt;
        int32_t y = (int32_t)i * 19;
        char b[16];
        if (armed) cv_rect(0, y, 240, 18, TE_RED), cv_rect_c_black(1, y + 1, 238, 16);
        cv_rect(4, y + 3, 12, 12, armed ? TE_COL[i] : TE_DIM[i]);
        if (i == TRK_DRUM) str_cpy(b, DRUM_KIT_NAMES[drum_kit()], sizeof b);
        else trk_short_name(i, b);
        b[10] = 0;
        cv_text_s(22, y + 1, b, armed ? C_WHITE : TE_G3);
        for (j = 0; j < 16u; j++) {
            uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k, on = 0;
            if (z == a) z = a + 1u;
            for (k = a; k < z && k < NSTEP; k++) if (trk_step_on(t, k)) on = 1;
            cv_rect(106 + (int32_t)j * 8, y + 5, 6, 8, on ? (armed ? TE_COL[i] : TE_DIM[i]) : TE_G1);
        }
    }
    cv_band_blit(y0);
}

/* REC armed (stopped: waiting for the first note, or PLAY), its count-in, or a free take running
 * (seq.c). Armed, the dials say how it records: KNOB 1 MODE free / tempo (an empty project),
 * KNOB 2 LENGTH (1, 2 or 4 bars), KNOB 3 START note / count, KNOB 4 QUANT on / off (2.7) (ui_input.c rec_knobs). The four tracks
 * stay in view; the REC LED blinks while armed, is lit during the take. */
UI_SMALL static void rec_screen_draw(void)
{
    static uint32_t body,rows,foot,dials;
    static uint8_t layout;
    uint32_t take=ft_on,empty=take||project_empty(),rt=take?ft_trk%NTRK:song.sel;
    uint32_t blink=(fm1_ms/250u)&1u,secs=take?ft_t*CTL/FS:0u,bars=0,bpm=0;
    uint32_t lay=take?1u:2u,free=empty&&!rec_tempo&&!take&&!progression.active,count=ci_on;
    if(take) bars=ft_fit(ft_t,&bpm);
    if(!rec_shown||lay!=layout) {lcd_fill(0,0,240,240,C_BLACK);ui.force=1;}
    rec_shown=1;layout=(uint8_t)lay;
    if(ui.force) draw_frame();
    draw_head();
    if(take) {
        uint32_t sig=1000003u+rt*7919u+blink*31u+secs*131u+bars*17u+bpm*3u;
        if(ui.force||sig!=body) {
            char b[24];uint32_t n=secs>99u?99u:secs;
            body=sig;cv_band(98);
            te_disc(18,30,9,blink?TE_RED:TE_DIM[3]);
            if(n>=10u) te_rec_digit(36,n/10u);
            te_rec_digit(76,n%10u);
            cv_text_te_g3(112,44,"S");
            cv_rect_te_g2(132,6,1,54);
            if(bars) {
                fmt_int(b,(int32_t)bars);
                cv_text_l(146,4,b,TE_COL[rt]);
                cv_text_te_g3(146+text_w_l(b)+6,18,bars==1u?"BAR":"BARS");
                fmt_int(b,(int32_t)bpm);
                cv_text_c_white(146,42,b);
                cv_text_te_g3(146+text_w_s(b)+4,42,"BPM");
            } else cv_text_l(146,4,"-",TE_G3);
            te_text_c(120,80,"PRESS REC ON THE 1",TE_RED);
            cv_band_blit(26);
        }
        rec_rows(rt,take,124u,&rows);
    } else {
        static const char *const L1[]={"PLAY FREELY","PLAY A NOTE","PRESS PLAY"};
        static const char *const L2[]={"THEN REC ON THE 1","IT STARTS THE LOOP","4 CLICKS, THEN REC"};
        uint32_t m=free?0u:rec_count?2u:1u;
        uint32_t sig=2000003u+empty*7u+free*11u+rec_count*13u+count*17u+ci_beat*19u+blink*31u;
        static char v[3][10];
        static const char *const LAB_E[]={"MODE","LENGTH","START","QUANT"};
        static const char *const LAB_F[]={"MODE","","","QUANT"};
        static const char *const LAB_N[]={"","LENGTH","START","QUANT"};
        const char *val[]={v[0],v[1],v[2],rec_quant_off?"OFF":"ON"};
        int32_t ratio[4];uint32_t len=(uint32_t)clamp(TSEL->p[P_SLEN],1,64);
        str_cpy(v[0],(rec_tempo||progression.active)?"TEMPO":"FREE",sizeof v[0]);
        fmt_int(v[1],(int32_t)(len%16u?len:len/16u));
        str_cpy(v[1]+str_len(v[1]),len%16u?" ST":len==16?" BAR":" BARS",6);
        str_cpy(v[2],rec_count?"COUNT":"NOTE",sizeof v[2]);
        ratio[0]=(rec_tempo||progression.active)?1000:0;ratio[1]=(int32_t)(len-1u)*1000/63;
        ratio[2]=rec_count?1000:0;ratio[3]=rec_quant_off?0:1000;
        const char *const *lab=!empty?LAB_N:free?LAB_F:LAB_E;
        for(uint32_t k=0;k<4;k++) if(!lab[k][0]) val[k]="";
        te_dials(lab,val,ratio,rt*7u+count*3u,&dials);
        rec_rows(rt,0u,74u,&rows);
        if(ui.force||sig!=body) {
            body=sig;cv_band(44);
            if(count) {
                char b[4];fmt_int(b,4u-(ci_beat>3u?3u:ci_beat));
                cv_text_s(8,12,"COUNT IN",TE_RED);
                cv_text_l(176,4,b,C_WHITE);
            } else {
                cv_text_c_white(8,4,L1[m]);
                cv_text_te_g4(8,24,L2[m]);
            }
            cv_band_blit(154);
        }
    }
    uint32_t fs=lay+rt*7u+count*37u+free*13u;
    if(ui.force||fs!=foot) {
        foot=fs;cv_band(38);
        cv_text_s(4,0,take?"FREE TAKE":count?"COUNT IN":"REC READY",TE_RED);
        char b[3]={'T',(char)('1'+rt),0};cv_text_s(220,0,b,TE_COL[rt]);
        cv_text_te_g4(4,20,take?"REC CLOSE / PLAY DROP":count?"REC: CANCEL":free?"TEMPO FOLLOWS YOU":"PLAY GO / REC CANCEL");
        cv_band_blit(202);
    }
}
