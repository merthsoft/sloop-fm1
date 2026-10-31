/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca user interface. Four columns map to KNOB 1..4. Rendering is lazy:
 * every element remembers what it last drew and is redrawn only on change. */
#include "version.h"
/* Shared cold clamp: audio DSP keeps its inline version. */
UI_SMALL static int32_t ui_clamp(int32_t v,int32_t lo,int32_t hi) {return clamp(v,lo,hi);}
#undef clamp
#define clamp ui_clamp

static void browse_end(int keep);
static void project_save(uint32_t slot);
static void arrangement_save(void);
static void panel_setup(void);
static void project_load(uint32_t slot);
static int project_undo_available(void);
static int project_undo_swap(int redo);
static void project_new(void);
static void project_erase(uint32_t slot);        /* SAVE > PROJECT > ERASE (2.6) */
static int project_used(uint32_t slot);
static int up_used(uint32_t k);              /* user presets: upreset.c */
static int up_load(uint32_t k);
static int up_load_to(track_t *t, uint32_t k);
static int up_arp_of(uint32_t k, int16_t *a);
static uint32_t up_count(void);
static uint32_t up_nth(uint32_t n);
static uint32_t up_rank(uint32_t slot);
static int32_t up_pc_index(uint32_t bank, uint32_t program, uint32_t factories);
static void up_name(uint32_t k, char *b);
static void up_slot_label(char *b, uint32_t k);
static void up_ui(uint32_t op, uint32_t k);
UI_SMALL static uint32_t user_of(const track_t *t)    /* user preset slot its sound came from, UP_SLOTS = none */
{
    return t->user && up_used(t->user - 1u) ? t->user - 1u : UP_SLOTS;
}
static uint32_t up_gen;                      /* bumped on every user bank change (redraws) */
static uint8_t sync_reload;                  /* engine / preset / project / user preset loaded: editor RELOAD push */

#define ACC C_HI                   /* amber everywhere; white is the only accent */
#define VAL(c) ((c) == ui.hot_col && ui.hot_t ? C_WHITE : TE_COL[(c) & 3u])   /* LIVE: one colour per knob */
#define RATIO(d, v) ((d)->max > (d)->min ? ((int32_t)(v) - (d)->min) * 1000 / ((d)->max - (d)->min) : -1)
/* layout: four 60 px columns, 4 px inset */
/* Terminus 8x16 (S) and 16x32 (L) */
#define Y_HEAD 0
#define H_HEAD 20
#define Y_LABEL 26
#define Y_VALUE 44
#define Y_GAUGE 64
#define Y_SEP_END 70
#define Y_GRAPH 74
#define H_GRAPH 124
#define G_OY 24                       /* graphs draw in the lower 100 px; the focus readout sits on top */
#define Y_FOOT 202
#define H_FOOT 38

static struct {
    uint8_t home;
    uint8_t page;                /* index into PAGES */
    uint8_t fam_last[FAM_COUNT]; /* last page used per family */
    uint8_t bank;                /* SEQ: 16-step bank (follows the cursor) */
    uint8_t cursor;              /* SEQ: step being edited (STEP page KNOB 1 moves it) */
    int8_t roll_zoom;            /* transient viewport: -1/0/1/2 = 8/16/32/64 steps */
    uint8_t roll_pitch;          /* NOTES identity, 0 = empty/unselected */
    uint32_t roll_project;       /* loaded project generation owning that identity */
    uint8_t note_context;        /* preserve identity only between NOTES and its actions */
    uint8_t entry_open;          /* SEQ: keys held since the first press of this entry */
    uint8_t tie_start, tie_voice;/* native tie gesture: source step, 0 ALL / 1..4 voice */
    uint32_t tie_sess;           /* STEP: one undo snapshot per OCT tie/rest-paint gesture */
    uint8_t hot_col, hot_t;      /* column whose knob was just turned (drawn white) */
    uint8_t menu;                /* 0 off, 1 list, 2 about (HOME held) */
    uint8_t menu_sel;
    uint32_t menu_sig, home_t0;  /* HOME press time (btn_hold) */
    uint8_t force;               /* full redraw pending */
    uint8_t msg_t;               /* transient message frames */
    uint8_t bpm_t;               /* frames the BPM stays highlighted after a SELECT turn */
    uint8_t arm, arm_t;          /* destructive action armed: param id, frames left to confirm */
    uint32_t rec_t0;             /* REC press time (btn_hold) */
    uint8_t confirm;             /* 1 = "clear the sequence?" (REC held on SEQ / ARP), 2 = "clear track n?" (TRACKS) */
    uint8_t confirm_trk;         /* the track the dialog clears */
    uint8_t uslot;               /* SAVE > USER: the selected user preset slot */
    /* layers (ui_layers.c): a function button held, the keys and knobs do something else */
    uint8_t layer;               /* the layer drawn (LY_*), LY_PLAY = none */
    uint8_t layer_btn;           /* its button (B_*), NB = none */
    uint8_t layer_used;          /* a key / knob / OCT was used while it was held: no tap on release */
    uint32_t layer_t0;           /* its press time (ms) */
    uint8_t step_page;           /* SEQ layer: the 16 steps shown (page x 16) */
    uint16_t step_held;          /* SEQ layer: the step keys held (white key index) */
    uint32_t step_sess;          /* SEQ layer: the undo session of this hold */
    uint8_t lock_par;            /* SEQ layer, a step held + KNOB 4: the parameter it locks (the last sound
                                  * parameter a knob changed on a page of the selected track; P_ED_FLT at boot) */
    uint8_t hold_kind;           /* a hold to confirm: 1 = clear the track (REC), 2 = save (SAVE) */
    uint32_t hold_t0;            /* (ms) */
    uint8_t hold_trk;
    uint32_t tap_ms[4];          /* tap tempo: the last taps */
    uint8_t tap_n;
    char msg[24];
    uint32_t enc_t[NE];
    /* drawn-state cache */
    char col[4][32];
    char focus_l[8], focus_v[8], focus_u[8];   /* the touched column, shown large */
    char big_l[4][8], big_v[4][10], big_u[4][8];   /* the four columns, for the big values (2.4: pages without a graph) */
    uint16_t big_c[4];
    uint32_t graph_sig, head_sig, foot_sig, frame;
    uint8_t graph_top;           /* the graph strip's top G_OY rows hold something */
} ui;

UI_SMALL static const page_t *cur_page(void) { return &PAGES[ui.page]; }
#include "ui_note_select.c"
UI_SMALL static int notes_page(void) { return cur_page()->scope==SC_STEP && cur_page()->id[0]==4u; }
static struct { uint8_t row,dest,hit,lane,at,track;uint32_t epoch; } note_action;
UI_SMALL static void actions_open(uint32_t hit,uint32_t lane,uint32_t at)
{
    note_action.row=0;note_action.dest=note_action.at=(uint8_t)at;
    note_action.hit=(uint8_t)hit;note_action.lane=(uint8_t)lane;
    note_action.track=song.sel;note_action.epoch=project_revision;ui.force=1;
    kb_grid=2;
}
static void actions_draw(void);
static int actions_on(void);
static int actions_input(uint32_t pressed,uint32_t home);
UI_SMALL static piano_selection_t roll_selection(void)
{
    if(ui.roll_project!=project_revision) ui.roll_pitch=0;
    return (piano_selection_t){ui.cursor,ui.roll_pitch};
}
UI_SMALL static void roll_select(piano_selection_t s)
{
    ui.cursor=s.step;ui.bank=(uint8_t)(s.step/16u);ui.roll_pitch=s.pitch;
    ui.roll_project=project_revision;ui.entry_open=0;
}
static int32_t accel(uint32_t role, int32_t s, int32_t range);   /* ui_input.c */
static void draw_head(void);
static void draw_frame(void);
static void graph_home(void);
static uint32_t steps_hash(const track_t *t);
static uint32_t trk_level(uint32_t c);
static void layer_screen_draw(void);                            /* ui_layers.c */
static void hold_screen_draw(void);
static uint8_t layer_shown;
enum { BT_NONE, BT_TAP, BT_HOLD };
static void sequence_screen_open(void);
static void sequence_screen_close(void);

UI_SMALL static uint32_t page_first(uint32_t fam)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == fam)
            return i;
    return 0;
}

/* transient message in the top bar: a + b */
UI_SMALL static void ui_say(const char *a, const char *b)
{
    uint32_t n;
    str_cpy(ui.msg, a, sizeof ui.msg);
    n = str_len(ui.msg);
    str_cpy(ui.msg + n, b, sizeof ui.msg - n);
    ui.msg_t = 40;
}

UI_SMALL static void ui_message(const char *s) { ui_say(s, ""); }
UI_SMALL static void note_edit_error(int result)
{ui_message(result==-2?"CHORD FULL":result==-3?"SELECT A NOTE":result==-4?"SET VOICE TO POLY":"NOTE AHEAD");}

UI_SMALL static void page_entered(void)
{
    if(cur_page()->graph!=GR_BROWSE)browse_end(0);
    ui.tie_voice=0;ui.tie_start=255;
    uint32_t context=!ui.home && (notes_page() || cur_page()->scope==SC_ACTION);
    if(!context || !ui.note_context) {
        ui.roll_pitch=0;ui.roll_project=project_revision;
        if(context && !is_drum(TSEL)) roll_select(piano_selection_at(TSEL,ui.cursor));
    }
    ui.note_context=(uint8_t)context;
    kb_grid=cur_page()->scope==SC_ACTION?2u:0u;
    if(cur_page()->scope==SC_ACTION)actions_open(0,0,ui.cursor);
    groove_preview.active = 0;
    const page_t *pg = cur_page();
    if (pg->scope == SC_STARTER && song.sel < NPART) sequence_screen_open();
    else sequence_screen_close();
    song.seq_mode = !ui.home && pg->fam == FAM_SEQ;
    if (!ui.home && pg->fam == FAM_GLO && pg->id[0] == G_CH1 && song.g[G_ROUTE]) {   /* (2.6) GLO > MIDI while MIDI 2 >
                                                                                   * IN sets the channels aside */
        if (song.g[G_ROUTE] == 1)
            ui_message("IN = CLOCK: NO NOTES");
        else {
            char b[10];
            str_cpy(b, "IN CH ", sizeof b);
            fmt_int(b + 6, song.g[G_ROUTE] - 1);
            ui_say(b, ": SEL TRK ONLY");
        }
    }
    ui.entry_open = 0;
    ui.hot_t = 0;                                /* the white value / focus box was the old page's */
    ui.force = 1;
}

/* (2.6) GLO > MIDI: two tracks never share a channel (it would play the first only, the other silent): a channel
 * turned (a knob, the editor, a backup) onto one another track has goes on to the next free one, the way it was
 * turned, and says whose it was. Every frame (ui_input) */
UI_SMALL static void midi_chan_fix(void)
{
    static const uint8_t ID[4] = {G_CH1, G_CH2, G_CH3, G_DRCH};
    static int16_t was[4] = {-1, -1, -1, -1};
    uint32_t k, j;
    for (k = 0; k < 4u; k++) {
        int32_t v = song.g[ID[k]], dir, owner = -1;
        if (was[k] < 0 || v == was[k] || !v) {
            was[k] = (int16_t)v;
            continue;
        }
        dir = v > was[k] ? 1 : -1;
        for (;;) {
            int32_t o = -1;
            for (j = 0; j < 4u; j++)
                if (j != k && song.g[ID[j]] == v)
                    o = (int32_t)j;
            if (o < 0)
                break;
            if (owner < 0)
                owner = o;
            v += dir;
            if (v < 1) {                                /* (down past 1: OFF, never taken) */
                v = 0;
                break;
            }
            if (v > 16) {                               /* (none free up there: as it was) */
                v = was[k];
                break;
            }
        }
        if (owner >= 0) {
            char b[8];
            str_cpy(b, "CH ", sizeof b);
            fmt_int(b + 3, song.g[ID[k]]);
            ui_say(b, owner == 3 ? " = DRUMS" : owner == 0 ? " = TRACK 1" : owner == 1 ? " = TRACK 2" : " = TRACK 3");
            song.g[ID[k]] = (int16_t)v;
        }
        was[k] = (int16_t)v;
    }
}

UI_SMALL static int step_on(const step_t *st) { return st->time == ST_NOTE && st->n; }
/* LEN (SLOOP 2.6, after Felucca 1.5 by Leo Kuroshita, Discussions #178, #173): a NOTE step's length is the step and
 * the TIE steps after it (inside LEN, no wrap); SEQ > STEP's KNOB 3 on a note sets it (ui_input.c step_edit). Longer:
 * the steps after it become clean TIEs (a REST or an empty step; it stops at the next step holding notes, which stays,
 * and at LEN); shorter: its last TIEs become RESTs (one holding notes keeps them). The steps' locks and nudges stay;
 * the turns on one step are one undo (ui_input.c). Returns the length it got */
UI_SMALL static uint32_t note_len(const track_t *t, uint32_t i)
{
    step_t s;note_effective(t,i,&s);uint32_t n=1;
    for (uint32_t k=0;k<s.n;k++) {
        uint32_t v=note_chain_len(t,i,s.note[k]);if(v>n)n=v;
    }
    return n;
}
UI_SMALL static uint32_t note_voice_len(const track_t *t,uint32_t i,uint32_t voice)
{
    step_t s;note_effective(t,i,&s);
    return voice && voice<=s.n?note_chain_len(t,i,s.note[voice-1]):note_len(t,i);
}
UI_SMALL static uint32_t note_edit_target(uint32_t *voice)
{
    if(notes_page() && !(fm1_in.buttons&((1u<<panel.btn[B_OCTUP])|(1u<<panel.btn[B_OCTDN])))) {
        int slot=piano_selection_slot(TSEL,roll_selection());
        *voice=slot>=0?(uint32_t)slot+1u:0;return ui.cursor;
    }
    uint32_t i=ui.tie_start<trk_len(TSEL)?ui.tie_start:ui.cursor;*voice=0;
    if (ui.tie_voice && ui.tie_start<trk_len(TSEL)) {
        step_t s;note_effective(TSEL,ui.tie_start,&s);
        if (ui.tie_voice<=s.n) {
            uint32_t pitch=s.note[ui.tie_voice-1];i=note_origin(TSEL,ui.tie_start,pitch);
            note_effective(TSEL,i,&s);*voice=note_slot(&s,pitch)+1;
        }
    }
    while (i && TSEL->step[i].time==ST_TIE) i--;
    return i;
}
UI_SMALL static uint32_t note_set_voice_len(track_t *t,uint32_t i,uint32_t voice,uint32_t want)
{
    step_t s;note_effective(t,i,&s);
    uint32_t n=note_voice_len(t,i,voice);want=(uint32_t)clamp(want,1,trk_len(t)-i);
    if (want>n) {
        if (note_extend(t,i,i+want-1,voice,0)) return n;
        note_extend(t,i,i+want-1,voice,1);
    } else if (want<n) {
        if (!voice && !note_pattern_linked(t)) {
            for (uint32_t k=i+want;k<i+n;k++) {
                if (!t->step[k].n) memset(&t->step[k],0,sizeof(step_t));
                t->step[k].time=ST_REST;
            }
        } else for (uint32_t k=0;k<s.n;k++) if (!voice || k+1==voice) {
            uint32_t end=note_chain_len(t,i,s.note[k]);
            if (end>want) note_cut(t,i+want,end-want,s.note[k]);
        }
    }
    return want;
}
UI_SMALL static uint32_t note_set_len(track_t *t,uint32_t i,uint32_t want)
{return note_set_voice_len(t,i,0,want);}
/* a synth step's velocity as it plays (seq.c step_vel: ACC 127, else its VEL, 96 when 0, through its first note's level) */
UI_SMALL static uint32_t step_vel_shown(const step_t *st)
{
    uint32_t base = (st->flags & SF_ACCENT) ? 127u : (st->vel ? st->vel : 96u);
    return lvl_vel(st->lvl & 3u, base);
}
/* step i of track t has something to play (synth: notes, drums: a lane) */
UI_SMALL static int trk_step_on(const track_t *t, uint32_t i)
{
    return is_drum(t) ? dstep_mask(&t->dstep[i % NSTEP]) != 0u : step_on(&t->step[i % NSTEP]);
}

UI_SMALL static void step_clear(step_t *st)
{
    memset(st, 0, sizeof *st);
    st->time = ST_REST;
}

#include "rhythm_shapes.c"
#include "drum_grooves.c"
#include "sequence_starters.c"

/* undo / redo (EDIT + OCT- / OCT+): the marked pattern and the one now swap places */
UI_SMALL static int undo_swap(int redo)
{
    track_t *t;
    int16_t len;
    if (project_undo_available())
        return project_undo_swap(redo);
    if (!undo.valid || (uint32_t)!!redo != undo.undone)
        return 0;
    t = &trk[undo.trk % NTRK];
    fm1_irq_off();
    {
        uint32_t i;
        for (i = 0; i < NSTEP; i++) {
            step_t x = t->step[i];
            int8_t m = t->micro[i];
            t->step[i] = undo.st[i];
            undo.st[i] = x;
            t->micro[i] = undo.micro[i];                 /* (2.7) and its nudge */
            undo.micro[i] = m;
        }
    }
    len = t->p[P_SLEN];
    t->p[P_SLEN] = undo.len;
    undo.len = len;
    if (groove_undo_matches()) groove_undo_swap();
    seq_edit_changed(t);
    undo.undone = (uint8_t)!redo;
    fm1_irq_on();
    sync_reload = 1;
    ui.force = 1;
    return 1;
}

/* SEQ cursor: wraps inside the pattern length, the bank follows, a step entry ends */
UI_SMALL static uint32_t roll_span(void)
{
    uint32_t n=8u<<(uint32_t)clamp((int32_t)ui.roll_zoom+1,0,3);
    return n<trk_len(TSEL)?n:trk_len(TSEL);
}
UI_SMALL static void cursor_set(int32_t c)
{
    int32_t len = TSEL->p[P_SLEN] > 0 ? TSEL->p[P_SLEN] : 1;
    ui.cursor = (uint8_t)((c % len + len) % len);
    ui.bank = (uint8_t)(ui.cursor / 16u);
    ui.entry_open = 0;
    ui.roll_pitch=0;
    if (!(fm1_in.buttons&(1u<<panel.btn[B_OCTUP]))) {ui.tie_voice=0;ui.tie_start=255;}
}

UI_SMALL static void cursor_fix(void)                           /* LEN got shorter: onto the last step */
{
    if (ui.cursor >= (uint32_t)TSEL->p[P_SLEN])
        cursor_set(TSEL->p[P_SLEN] - 1);
}

UI_SMALL static void note_name(char *b, uint32_t n)
{
    str_cpy(b, N_NOTE[n % 12u], 4);
    fmt_int(b + str_len(b), (int32_t)(n / 12u) - 1);
}

/* Spell seven-note scales by letter degree; other scales use the key's accidentals. */
UI_SMALL static void scale_pitch_name(char *b, const track_t *t, uint32_t n)
{
    static const char *const flat[] = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
    static const uint8_t relative[] = {0,0,3,10,5,0,3,3,8,7,1,3,3,0,0,0};
    static const uint8_t sharp_letter[] = {0,0,1,1,2,3,3,4,4,5,5,6};
    static const uint8_t flat_letter[] = {0,1,1,2,2,3,4,4,5,5,6,6};
    static const uint8_t natural[] = {0,2,4,5,7,9,11};
    uint32_t scale = (uint32_t)clamp(t->p[P_SCALE], 0, NSCALES - 1);
    if (!scale && t->p[P_CHORD]) scale = 2;
    uint32_t root = (uint32_t)t->p[P_ROOT] % 12u, pc = n % 12u;
    uint32_t flats = (0x56au >> ((root + (scale < NELEM(relative) ? relative[scale] : 0u)) % 12u)) & 1u;
    if (scale == 1 && root == 6) flats = 0; /* ROOT calls this key F#, not Gb. */
    uint32_t mask = SCALE_MASK[scale], count = 0, degree = 0, offset = (pc + 12u - root) % 12u;
    for (uint32_t i = 0; i < 12; i++) { count += (mask >> i) & 1u; if (i < offset) degree += (mask >> i) & 1u; }
    if (count == 7 && ((mask >> offset) & 1u)) {
        uint32_t letter = ((flats ? flat_letter[root] : sharp_letter[root]) + degree) % 7u;
        uint32_t delta = (pc + 12u - natural[letter]) % 12u;
        if (delta == 0 || delta == 1 || delta == 11) {
            b[0] = "CDEFGAB"[letter]; b[1] = delta == 1 ? '#' : delta == 11 ? 'b' : 0; b[2] = 0;
            return;
        }
    }
    str_cpy(b, flats ? flat[pc] : N_NOTE[pc], 4);
}
UI_SMALL static void scale_note_name(char *b, const track_t *t, uint32_t n)
{
    scale_pitch_name(b, t, n);
    int32_t octave = (int32_t)(n / 12u) - 1;
    if (b[0] == 'B' && b[1] == '#') octave--;
    if (b[0] == 'C' && b[1] == 'b') octave++;
    fmt_int(b + str_len(b), octave);
}

UI_SMALL static void open_family(uint32_t fam)
{
    if (!ui.home && cur_page()->fam == fam) {          /* same button again: next page */
        uint32_t i = ui.page + 1u;
        while (i < NPAGES && PAGES[i].fam == fam && !page_shown(&PAGES[i]))
            i++;                                        /* (not a page of another engine: EDIT > FILTER) */
        if (i >= NPAGES || PAGES[i].fam != fam)
            i = page_first(fam);
        ui.page = (uint8_t)i;
    } else {
        ui.page = ui.fam_last[fam] && PAGES[ui.fam_last[fam]].fam == fam && page_shown(&PAGES[ui.fam_last[fam]])
                      ? ui.fam_last[fam] : (uint8_t)page_first(fam);
    }
    ui.fam_last[fam] = ui.page;
    ui.home = 0;
    page_entered();
}

/* SELECT on a page: the previous / next page of its family, as tapping the button again but both ways,
 * stopping at the ends. Not the screens of their own (SONG, DRUMS) nor a family of one page (TRACKS):
 * 0 then, and SELECT is the tempo there */
UI_SMALL static int page_walk(int32_t s)
{
    const page_t *pg = cur_page();
    uint32_t i, fam = pg->fam, n = 0;
    int32_t cur = -1, to;
    uint8_t idx[16];                               /* (2.7: GLO has 9 pages with MIDI 3) */
    if (ui.home || pg->scope == SC_DRUM)
        return 0;
    for (i = 0; i < NPAGES && n < 16u; i++) {
        if (PAGES[i].fam != fam || PAGES[i].scope == SC_DRUM || !page_shown(&PAGES[i]))
            continue;
        if (i == ui.page)
            cur = (int32_t)n;
        idx[n++] = (uint8_t)i;
    }
    if (n < 2u || cur < 0)
        return 0;
    to = clamp(cur + s, 0, (int32_t)n - 1);
    if (to != cur) {
        ui.page = idx[to];
        ui.fam_last[fam] = ui.page;
        page_entered();
    }
    return 1;
}

/* the page shown is one the selected track has (2.6: EDIT > FILTER, ANALOG only): after a track or engine change,
 * from the previous page of the family that it has (a hidden page is never the first one), once a frame */
UI_SMALL static void page_fix(void)
{
    uint32_t i = ui.page, fam = PAGES[i].fam;
    if (ui.home || page_shown(&PAGES[i]))
        return;
    while (i > 0u && PAGES[i - 1u].fam == fam && !page_shown(&PAGES[i - 1u]))
        i--;
    ui.page = (uint8_t)(i > 0u && PAGES[i - 1u].fam == fam ? i - 1u : page_first(fam));
    ui.fam_last[fam] = ui.page;
    page_entered();
}

UI_SMALL static void go_home(void)
{
    browse_end(0);
    ui.note_context=0;ui.roll_pitch=0;kb_grid=0;
    groove_preview.active = 0;
    sequence_screen_close();
#if FELUCCA_ARRANGER
    ui.home = 0;
    ui.page = (uint8_t)page_first(FAM_TRK);
#else
    ui.home = 1;
#endif
    ui.entry_open = 0;
    ui.hot_t = 0;
    song.seq_mode = 0;
    ui.force = 1;
}

/* ------------------------------------------------------- track setup --- */
/* LIVE: no factory sequence patterns. Loading a sound (factory or user preset) never
 * writes the sequencer: every pattern is the one the player records or enters. */

/* the parts' sounds at power-on (engine, preset): bass, pad, lead */
static const uint8_t TRK_DEF[NPART][2] = {{0, 0}, {1, 0}, {4, 5}};   /* ANALOG 808 BOOM, DIGITAL RHODES, SAMPLE LOFI FLUTE */
UI_SMALL static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? TRK_DEF[i][0] : 0u; }

UI_SMALL static int seq_is_empty(const track_t *t) { return track_empty(t); }

/* SEL page is per synth; the held layer and LEDs show that same track's key.
 * Drums have no musical scale controls, so retain synth 1 as their fallback. */
UI_SMALL static const track_t *ui_scale_track(void) { return is_drum(TSEL) ? &trk[0] : TSEL; }

UI_SMALL static void track_defaults_steps(track_t *t) { steps_clear(t); }

/* what loading a sound (factory or user preset) leaves alone: the mix (LEVEL, PAN, MUTE:
 * the TRACKS faders, and the track FILTER), the pattern parameters (LEN, DIV, SWING, GATE) and the key the part plays
 * in (ROOT, SCALE, QNT, CHORD: the song's; SEL + key sets the root of every part). The SLICER is
 * part of the sound: a factory preset turns it OFF (its defaults), a user preset brings its own */
UI_SMALL static int param_kept(uint32_t i)
{
    return i == P_LEVEL || i == P_PAN || i == P_MUTE || (i >= P_SLEN && i <= P_SGATE) ||
           (i >= P_ROOT && i <= P_QUANT) || i == P_CHORD || i == P_TFLT || i == P_STRUM || i == P_VLEAD || i == P_AHOLD;
}

/* preset pi of the engine the track asked for: the whole sound (not the pattern parameters) */
/* SLOOP 2.7: a sound loaded while the track's arpeggiator runs keeps it: its settings (MODE … ORDER, never written: the
 * audio ISR never sees them change) and the notes it holds (seq.c arp_keep_req: the panic of this track leaves them).
 * With the arp OFF, a preset brings its own arp, as before */
UI_SMALL static int arp_param(uint32_t i) { return i >= P_AMODE && i <= P_AORDER; }
UI_SMALL static int arp_is_sounds(const track_t *t)
{
    int16_t a[8];
    uint32_t i, k = user_of(t);
    if (k < UP_SLOTS) {
        if (up_arp_of(k, a))
            return 0;
    } else {
        const engine_t *e = ENGINES[t->eng_req % NENGINES];
        const preset_t *pr;
        if (t->user || !e->npresets)
            return 0;
        pr = &e->presets[t->preset % e->npresets];
        for (i = 0; i < 8u; i++)
            a[i] = (int16_t)(i < 4u && pr->arp[i] ? pr->arp[i] - 1 : TP[P_AMODE + i].def);
    }
    for (i = 0; i < 8u; i++)
        if (t->p[P_AMODE + i] != a[i])
            return 0;
    return 1;
}
UI_SMALL static void apply_preset_sound(track_t *t, uint32_t pi, int keep_arp)
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    uint32_t i;
    int kbd_hold = !t->p[P_AMODE] && t->p[P_AHOLD];   /* (2.7: KBD HOLD stays on) */
    if (is_drum(t))
        return;
    if (keep_arp)
        arp_keep_req |= (uint8_t)(1u << trk_index(t));   /* (before the panic below) */
    sound_change(t);                                  /* (2.6: what sounds fades out with the old sound, no click) */
    panic_req |= (uint8_t)(1u << trk_index(t));       /* MONO/POLY may change: release what sounds */
    t->user = 0;
    if (t == TSEL)
        sync_reload = 1;
    if (!e->npresets)
        return;
    pi %= e->npresets;
    t->preset = (uint8_t)pi;
    for (i = 0; i < P_E0; i++)                        /* the rest of the sound to its defaults: a preset */
        if (!param_kept(i) && !(keep_arp && arp_param(i)) && !(kbd_hold && i == P_AHOLD))
            t->p[i] = TP[i].def;                     /* sounds the same after any edit (not the pattern, not the mix) */
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = (int16_t)e->presets[pi].e[i];
    t->p[P_ATK] = e->presets[pi].env[0];
    t->p[P_DEC] = e->presets[pi].env[1];
    t->p[P_SUS] = e->presets[pi].env[2];
    t->p[P_REL] = e->presets[pi].env[3];
    t->p[P_ED_FLT] = e->presets[pi].fenv;
    t->p[P_ED_FX] = preset_trim(t->eng_req % NENGINES, pi);  /* level-matched (tools/level_presets.py) */
    t->p[P_VOICE] = e->presets[pi].mono ? V_LEGATO : V_POLY;   /* mono presets keep the legato feel */
    {   /* the rest of the patch: sends, arpeggiator (never a pattern: LIVE) */
        static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
        const preset_t *pr = &e->presets[pi];
        for (i = 0; i < 4u; i++) {
            t->p[P_DIST + i] = (int16_t)(pr->fx[i] ? pr->fx[i] - 1 : FX_DEF[i]);
            if (!keep_arp)
                t->p[P_AMODE + i] = (int16_t)(pr->arp[i] ? pr->arp[i] - 1 : TP[P_AMODE + i].def);
        }
        preset_extras(t->p, pr);                     /* glide, pitch / LFO modulation, voice mode */
    }
    fm6_track_loaded(t);                             /* FM6: the preset's patch (its PTCH) */
}

UI_SMALL static void apply_preset_to(track_t *t, uint32_t pi)
{
    apply_preset_sound(t, pi, t->p[P_AMODE] && !arp_is_sounds(t));
}

/* the engine's defaults and its first preset. With the audio IRQ off: the ISR sees the old engine with
 * its values or the new one with its own (voice.c engine_block), never one with the other's */
UI_SMALL static void set_engine_of(track_t *t, uint32_t ei)
{
    const engine_t *e = ENGINES[ei % NENGINES];
    uint32_t i;
    int keep_arp = t->p[P_AMODE] && !arp_is_sounds(t);
    if (is_drum(t))
        return;
    fm1_irq_off();
    locks_restore(t);                                 /* (a lock in force: its base first, then the new sound) */
    sound_change(t);                                  /* (2.6: the sound as it is, before anything new is written) */
    t->eng_req = (uint8_t)(ei % NENGINES);
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = e->edit[i].def;
    apply_preset_sound(t, 0, keep_arp);
    fm1_irq_on();
}

static const uint8_t TRACK_FX[] = {P_DIST,P_CHOR,P_DLY,P_REV,P_ITYPE,P_IA,P_IB,P_IC,P_IMIX,
                                 P_SLCR,P_SLPAT,P_SLRATE,P_SLDEPTH,P_TFLT};
/* Track playing preferences survive browsing, but are not part of FX RESET. */
static const uint8_t TRACK_PLAY[] = {P_VOICE,P_GLIDE,P_GLMODE,P_PRIO,P_ALLOC};
#define TRACK_BROWSE_COUNT (NELEM(TRACK_FX) + NELEM(TRACK_PLAY))
UI_SMALL static void track_fx_values(track_t *t, int16_t *v, uint32_t restore)
{
    if (!restore) locks_restore(t);
    for (uint32_t i=0;i<TRACK_BROWSE_COUNT;i++) {
        uint32_t id=i<NELEM(TRACK_FX)?TRACK_FX[i]:TRACK_PLAY[i-NELEM(TRACK_FX)];
        if (restore) t->p[id]=v[i]; else v[i]=t->p[id];
    }
}
UI_SMALL static void apply_preset(uint32_t pi)
{
    int16_t fx[TRACK_BROWSE_COUNT];track_fx_values(TSEL,fx,0);
    apply_preset_to(TSEL,pi);track_fx_values(TSEL,fx,1);
}
UI_SMALL static void set_engine(uint32_t ei)
{
    int16_t fx[TRACK_BROWSE_COUNT];track_fx_values(TSEL,fx,0);
    set_engine_of(TSEL,ei);track_fx_values(TSEL,fx,1);
}
UI_SMALL static void track_fx_reset(void)
{
    for(uint32_t i=0;i<NELEM(TRACK_FX);i++) TSEL->p[TRACK_FX[i]]=TP[TRACK_FX[i]].def;
    sync_reload=1;ui_message("TRACK FX RESET");ui.force=1;
}

UI_SMALL static void track_defaults(track_t *t)
{
    uint32_t i;
    t->chord_inv=t->chord_spread=0;
    for (i = 0; i < P_E0; i++)
        t->p[i] = TP[i].def;
    track_defaults_steps(t);
}

/* switch engine (its defaults + first preset) and say so */
UI_SMALL static void select_engine(uint32_t e)
{
    if (is_drum(TSEL))
        return;
    set_engine(e);
    ui_say("ENGINE ", ENGINES[TSEL->eng_req]->name);
    ui.force = 1;
}

/* the factory presets as one list by kind (basses, keys, organs, pads, leads, plucks and bells, stabs,
 * the rest), then the used user presets: the PRESETS knob and the PRESETS page browse it. By name: an
 * engine's preset table may change order; tests/ui_pages_test.c checks every preset is here once */
enum { BK_BASS, BK_KEYS, BK_ORGAN, BK_PAD, BK_LEAD, BK_PLUCK, BK_STAB, BK_FX, BK_CHIP, BK_INST, BK_LOFI };
static const char *const BANK_KIND[] = {"BASS", "KEYS", "ORGN", "PAD", "LEAD", "PLCK", "STAB", "FX", "CHIP", "INST", "LOFI"};
_Static_assert(NENGINES<=16,"preset browser engine index fits four bits");
/* Both indices fit a nibble; no alignment padding in this cold ROM lookup. */
static const struct __attribute__((packed)) { uint8_t kind:4, e:4; } BANK[] = {
    {BK_BASS, 0} /* 808 BOOM */, {BK_BASS, 0} /* 808 DIRTY */, {BK_BASS, 0} /* 808 SLIDE */, {BK_BASS, 0} /* SUB BASS */,
    {BK_BASS, 0} /* PLUGG BASS */, {BK_BASS, 0} /* REESE */, {BK_BASS, 0} /* WOBBLE */, {BK_BASS, 0} /* ACID 303 */,
    {BK_BASS, 1} /* FM BASS */, {BK_BASS, 2} /* CZ BASS */, {BK_BASS, 6} /* FAT BASS */, {BK_BASS, 0} /* FUNK BASS */,
    {BK_BASS, 5} /* WOW BASS */, {BK_BASS, 3} /* GB BASS */, {BK_BASS, 4} /* UP BASS */, {BK_BASS, 4} /* DEEP BASS */,
    {BK_BASS, ENGI_FM6} /* ROUND BASS */, {BK_BASS, 1} /* BASS GTR */, {BK_BASS, 0} /* MONO BASS */,
    {BK_BASS, ENGI_PHYS} /* PLUCK BASS */, {BK_BASS, 3} /* NES BASS */, {BK_BASS, 0} /* SAW BASS */,
    {BK_KEYS, 1} /* RHODES */, {BK_KEYS, 1} /* DX RHODES */, {BK_KEYS, 1} /* WURLI */, {BK_KEYS, 1} /* M1 PIANO */,
    {BK_KEYS, 1} /* AFRO KEYS */, {BK_KEYS, 4} /* GRAND PNO */, {BK_KEYS, 4} /* DUSTY PNO */, {BK_KEYS, 4} /* LOFI KEYS */,
    {BK_KEYS, 2} /* SOFT KEYS */, {BK_KEYS, 1} /* CLAV */, {BK_KEYS, ENGI_FM6} /* TINE EP */, {BK_KEYS, 1} /* FM GRAND */,
    {BK_ORGAN, 7} /* SOUL ORGAN */, {BK_ORGAN, 7} /* GOSPEL */, {BK_ORGAN, 7} /* JAZZ ORGAN */,
    {BK_ORGAN, 7} /* DIRTY B3 */, {BK_ORGAN, 7} /* HOUSE ORGN */, {BK_ORGAN, ENGI_FM6} /* DRAWBARS */,
    {BK_ORGAN, 6} /* ACCORDION */, {BK_ORGAN, 7} /* ROCK ORGAN */, {BK_ORGAN, 7} /* CHAPEL */,
    {BK_ORGAN, 7} /* REGGAE ORG */, {BK_PAD, 0} /* WARM PAD */, {BK_PAD, 6} /* SAW PAD */, {BK_PAD, 1} /* GLASS PAD */,
    {BK_PAD, 0} /* DARK STR */, {BK_PAD, 2} /* CZ STRING */, {BK_PAD, 0} /* ATMOS PAD */, {BK_PAD, 8} /* LOFI CLOUD */,
    {BK_PAD, 8} /* VIBE HAZE */, {BK_PAD, 5} /* CHOIR AAH */, {BK_PAD, 5} /* SOUL OOH */, {BK_PAD, ENGI_FM6} /* SOFT PAD */,
    {BK_PAD, 6} /* ENSEMBLE */, {BK_PAD, 8} /* STR CLOUD */, {BK_PAD, 8} /* SHIMMER */, {BK_PAD, 8} /* DRONE */,
    {BK_PAD, ENGI_PHYS} /* BOWED MTL */, {BK_PAD, ENGI_NOISE} /* WIND */, {BK_PAD, ENGI_PHYS} /* CELLO BOW */,
    {BK_PAD, ENGI_PHYS} /* GLASS BOWL */, {BK_PAD, ENGI_NOISE} /* OCEAN */, {BK_PAD, 2} /* CZ PAD */,
    {BK_PAD, 5} /* VOX PAD */, {BK_PAD, 8} /* PNO FREEZE */, {BK_PAD, 8} /* HORN CLOUD */, {BK_PAD, 0} /* SINE PAD */,
    {BK_PAD, 6} /* PULSE PAD */, {BK_LEAD, 0} /* SUPERSAW */, {BK_LEAD, 0} /* G-FUNK LD */, {BK_LEAD, 6} /* SYNC LEAD */,
    {BK_LEAD, 6} /* HOOVER */, {BK_LEAD, 5} /* TALKBOX */, {BK_LEAD, 3} /* GAME LEAD */, {BK_LEAD, 4} /* LOFI FLUTE */,
    {BK_LEAD, 8} /* FLUTE DUST */, {BK_LEAD, 0} /* FAT LEAD */, {BK_LEAD, 0} /* PAN FLUTE */, {BK_LEAD, 5} /* HARMONICA */,
    {BK_LEAD, ENGI_NOISE} /* NZ ARCADE */, {BK_LEAD, 3} /* WAVE LEAD */, {BK_LEAD, 2} /* CZ LEAD */,
    {BK_LEAD, 5} /* ROBOT */, {BK_LEAD, 0} /* PWM LEAD */, {BK_LEAD, 6} /* SQR LEAD */, {BK_PLUCK, 0} /* TRAP PLUCK */,
    {BK_PLUCK, 2} /* RESO PLUCK */, {BK_PLUCK, 1} /* PLUGG BELL */, {BK_PLUCK, 1} /* TRAP BELL */,
    {BK_PLUCK, 1} /* MUSIC BOX */, {BK_PLUCK, 1} /* KALIMBA */, {BK_PLUCK, 1} /* MARIMBA */, {BK_PLUCK, 4} /* VIBES */,
    {BK_PLUCK, 3} /* 8BIT ARP */, {BK_PLUCK, ENGI_FM6} /* GLASS BELL */, {BK_PLUCK, ENGI_FM6} /* WOOD BARS */,
    {BK_PLUCK, ENGI_FM6} /* NYLON PICK */, {BK_PLUCK, 1} /* HARP */, {BK_PLUCK, 1} /* STEEL DRUM */,
    {BK_PLUCK, 1} /* GLOCKEN */, {BK_PLUCK, 1} /* CELESTA */, {BK_PLUCK, 1} /* XYLOPHONE */,
    {BK_PLUCK, ENGI_PHYS} /* STR PLUCK */, {BK_PLUCK, ENGI_PHYS} /* SITAR */, {BK_PLUCK, ENGI_PHYS} /* PHYS HARP */,
    {BK_PLUCK, ENGI_PHYS} /* BELL TREE */, {BK_PLUCK, ENGI_PHYS} /* MODAL BAR */, {BK_PLUCK, ENGI_PHYS} /* THUMB PNO */,
    {BK_PLUCK, ENGI_PHYS} /* NYLON GTR */, {BK_PLUCK, ENGI_PHYS} /* STEEL GTR */, {BK_PLUCK, ENGI_PHYS} /* MUTED GTR */,
    {BK_PLUCK, ENGI_PHYS} /* KOTO */, {BK_PLUCK, ENGI_PHYS} /* BANJO */, {BK_PLUCK, ENGI_PHYS} /* TANPURA */,
    {BK_PLUCK, ENGI_PHYS} /* CHIMES */, {BK_PLUCK, ENGI_PHYS} /* BIG BELL */, {BK_PLUCK, ENGI_PHYS} /* VIBRA BAR */,
    {BK_PLUCK, ENGI_PHYS} /* STEEL PAN */, {BK_PLUCK, ENGI_PHYS} /* WOOD BLOCK */, {BK_PLUCK, 3} /* 1BIT BEEP */,
    {BK_PLUCK, 2} /* CZ RESO */, {BK_PLUCK, 1} /* TUBE BELL */, {BK_PLUCK, 1} /* FM PLUCK */, {BK_STAB, 6} /* MIN STAB */,
    {BK_STAB, 6} /* MIN7 STAB */, {BK_STAB, 6} /* RAVE STAB */, {BK_STAB, 6} /* DUB CHORD */, {BK_STAB, 0} /* SYN BRASS */,
    {BK_STAB, 2} /* CZ BRASS */, {BK_STAB, 4} /* HORN STAB */, {BK_STAB, 4} /* STRING STB */,
    {BK_STAB, ENGI_FM6} /* BRASS SECT */, {BK_STAB, 6} /* 80S POLY */, {BK_STAB, 3} /* CHIP CHORD */,
    {BK_STAB, 6} /* POWER STAB */, {BK_FX, 4} /* SCRATCH */, {BK_FX, 4} /* GM KIT */, {BK_FX, ENGI_PHYS} /* HAND DRUM */,
    {BK_FX, ENGI_PHYS} /* MEMB TOMS */, {BK_FX, ENGI_NOISE} /* RAIN */, {BK_FX, ENGI_NOISE} /* NZ METAL */,
    {BK_FX, ENGI_PHYS} /* TABLA */, {BK_FX, ENGI_PHYS} /* CONGA */, {BK_FX, ENGI_PHYS} /* TIMPANI */,
    {BK_FX, ENGI_NOISE} /* VINYL */, {BK_FX, ENGI_NOISE} /* HISS */, {BK_FX, ENGI_NOISE} /* RISER */,
    {BK_FX, ENGI_NOISE} /* NZ SNARE */, {BK_FX, ENGI_NOISE} /* BITCRUSH */, {BK_FX, ENGI_NOISE} /* RADIO */,
    {BK_FX, 3} /* CHIP NOISE */,
#if FELUCCA_SLICE
    {BK_FX, ENGI_SLICE} /* BREAK 16 */, {BK_FX, ENGI_SLICE} /* CHOP 8 */, {BK_FX, ENGI_SLICE} /* REVERSE */,
    {BK_FX, ENGI_SLICE} /* USR SLICE */,
#endif
    /* Append collections: existing factory/favourite identities stay unchanged. */
    {BK_CHIP, 3} /* NES PLS12 */, {BK_CHIP, 3} /* NES PLS50 */, {BK_CHIP, 3} /* NES PLUCK */, {BK_CHIP, 3} /* NES TRI */,
    {BK_CHIP, 3} /* GB WAVE BS */, {BK_CHIP, 3} /* GB HOLLOW */, {BK_CHIP, 3} /* GB ORGAN */, {BK_CHIP, 3} /* GB ARP */,
    {BK_INST, 2} /* VIOLIN */, {BK_INST, 1} /* CLARINET */, {BK_INST, 2} /* OBOE */, {BK_INST, 1} /* BASSOON */,
    {BK_INST, ENGI_PHYS} /* HARPSICHORD */, {BK_INST, ENGI_PHYS} /* OUD */,
    {BK_LOFI, 3}, {BK_LOFI, 3}, {BK_LOFI, 3}, {BK_LOFI, 3},
    {BK_CHIP, 3} /* NES REED */, {BK_CHIP, 3} /* NES BUBBLE */,
    {BK_CHIP, 3} /* GB MALLET */, {BK_CHIP, 3} /* GB VOX */,
    {BK_CHIP, 1} /* MD BASS */, {BK_CHIP, 1} /* MD BRASS */,
    {BK_LOFI, 3} /* WORN EP */, {BK_LOFI, 3} /* DUST ORGAN */,
    {BK_LEAD,2} /* REV LEAD */, {BK_INST,2} /* REV GLASS */,
    {BK_PAD,0} /* REV STR */, {BK_LOFI,3} /* REV HAZE */, {BK_LEAD,2} /* REV REED */,
    {BK_INST,0} /* SYN VIOLIN */,
    {BK_PLUCK,1} /* REV METAL */,
    {BK_PLUCK,1} /* REV HARP */,
    {BK_PAD,5} /* REV CHOIR */,
    {BK_CHIP,3} /* GB RAZOR */,
    {BK_CHIP,3} /* NES SPARK */,
    {BK_CHIP,1} /* MD CRYSTAL */,
    {BK_CHIP,1} /* MD PICK */,
    {BK_PAD,0} /* NIGHT PAD */,
    {BK_INST,2} /* SLASH STR */,
    {BK_PAD,5} /* HEX CHOIR */,
    {BK_FX,1} /* DREAD BELL */,
    {BK_PAD,2} /* ORBIT PAD */,
    {BK_KEYS,1} /* STAR KEYS */,
    {BK_PAD,5} /* LUNAR VOX */,
    {BK_PLUCK,10} /* ISLAND PAN */,
    {BK_PLUCK,10} /* DESERT OUD */,
    {BK_ORGAN,7} /* DUSK ORGAN */,
    {BK_CHIP,3} /* PIXEL BASS */,
    {BK_CHIP,3} /* SAVE BELL */,
    {BK_CHIP,2} /* CRT LEAD */,
    {BK_PAD,8} /* GLASS RAIN */,
    {BK_KEYS,7} /* TOY ORGAN */,
    {BK_FX,5} /* ROBOT TALK */,
};
#define NBANK (sizeof BANK / sizeof BANK[0])
/* Resolved against engine preset names; keep this order for stored favourites and PC. */
static const uint8_t bank_pi[] = {
    0, 1, 3, 2, 5, 6, 7, 4, 12, 0, 0, 8, 3, 1, 1, 10,
    2, 18, 18, 12, 3, 20, 0, 1, 2, 4, 5, 0, 8, 9, 1, 3,
    0, 21, 0, 1, 2, 3, 4, 6, 8, 5, 6, 7, 13, 5, 11, 14,
    2, 15, 0, 1, 0, 1, 4, 9, 3, 4, 5, 3, 0, 15, 20, 4,
    6, 6, 6, 7, 21, 13, 12, 9, 6, 7, 2, 0, 5, 2, 17, 16,
    4, 2, 6, 5, 5, 19, 11, 10, 3, 10, 6, 7, 9, 8, 2, 2,
    1, 5, 7, 17, 13, 14, 15, 16, 2, 7, 8, 0, 1, 4, 9, 10,
    11, 13, 14, 16, 17, 18, 19, 21, 22, 5, 7, 19, 20, 1, 2, 3,
    4, 11, 4, 3, 4, 3, 10, 4, 12, 6, 7, 5, 6, 1, 3, 23,
    24, 25, 5, 6, 7, 8, 9, 10, 7,
#if FELUCCA_SLICE
    0, 1, 2, 3,
#endif
    8, 9, 10, 11, 12, 13, 14, 15, 8, 22, 9, 23, 26, 27, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 24, 25,
    10,11,
    22,26,12,
    23,26,27,7,27,28,28,29,
    24,13,8,30,
    14,31,9,28,29,8,29,30,15,8,9,10,
};
_Static_assert(NELEM(bank_pi) == NBANK, "every browser entry has an engine-local index");
static void bank_resolve(void) {} /* Existing host fixtures may call this. */
UI_SMALL static uint32_t base_pos(uint32_t *total)            /* list index of the selected track's preset (no favourites) */
{
    uint32_t i, cur = 0;
    for (i = 0; i < NBANK; i++)
        if (BANK[i].e == TSEL->eng_req && bank_pi[i] == TSEL->preset)
            cur = i;
    if (user_of(TSEL) < UP_SLOTS)
        cur = NBANK + up_rank(user_of(TSEL));
    *total = NBANK + up_count();
    return cur;
}

/* list index n (< total) -> engine, *k its preset; NENGINES = user preset, *k its slot (no favourites) */
UI_SMALL static uint32_t base_at(uint32_t n, uint32_t *k)
{
    if (n >= NBANK) {
        *k = up_nth(n - NBANK);
        return NENGINES;
    }
    *k = bank_pi[n] == 0xFF ? 0u : bank_pi[n];
    return BANK[n].e;
}
UI_SMALL static const char *base_kind(uint32_t n) { return n < NBANK ? BANK_KIND[BANK[n].kind] : "USER"; }

/* SLOOP 2.7: favourites (PRESETS page, KNOB 4): up to 16 sounds at the top of the PRESETS list, kept in the FM-1's
 * settings (project.c, the settings' side record). An entry: a factory sound, its BANK index; a user preset, FAV_USER |
 * its slot (an erased one is left out of the list). Program Change keeps the list without them (pc_poll) */
#define FAV_N 16u
#define FAV_NONE 0xFFFFu
#define FAV_USER 0x8000u
static uint16_t fav[FAV_N] = {FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE,
                              FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE, FAV_NONE};
static uint8_t fav_dirty;                            /* changed since the settings were written */
static int8_t fav_at = -1;                           /* the favourite the selected track's sound was loaded from */
UI_SMALL static uint32_t fav_list(uint32_t *b)                /* the favourites as base list indices, in order */
{
    uint32_t i, n = 0;
    for (i = 0; i < FAV_N; i++) {
        uint32_t f = fav[i];
        if (f == FAV_NONE)
            continue;
        if (f & FAV_USER) {
            if (!up_used(f & 0xFFu))
                continue;
            b[n++] = NBANK + up_rank(f & 0xFFu);
        } else if (f < NBANK) {
            b[n++] = f;
        }
    }
    return n;
}
UI_SMALL static uint32_t fav_key(void)                        /* the selected track's sound as a favourite entry */
{
    uint32_t i;
    if (user_of(TSEL) < UP_SLOTS)
        return FAV_USER | user_of(TSEL);
    for (i = 0; i < NBANK; i++)
        if (BANK[i].e == TSEL->eng_req && bank_pi[i] == TSEL->preset)
            return i;
    return FAV_NONE;
}
UI_SMALL static int fav_has(uint32_t key)
{
    uint32_t i;
    for (i = 0; i < FAV_N; i++)
        if (key != FAV_NONE && fav[i] == key)
            return 1;
    return 0;
}
UI_SMALL static void fav_toggle(int add)                      /* KNOB 4 on the PRESETS page: right adds, left removes */
{
    uint32_t key = fav_key(), i;
    if (key == FAV_NONE || is_drum(TSEL))
        return;
    if (add) {
        if (fav_has(key)) {
            ui_message("FAV: ALREADY");
            return;
        }
        for (i = 0; i < FAV_N && fav[i] != FAV_NONE; i++)
            ;
        if (i == FAV_N) {
            ui_message("FAVS FULL (16)");
            return;
        }
        fav[i] = (uint16_t)key;
        ui_message("FAV ADDED");
    } else {
        for (i = 0; i < FAV_N && fav[i] != key; i++)
            ;
        if (i == FAV_N)
            return;
        for (; i + 1u < FAV_N; i++)                  /* (the order kept) */
            fav[i] = fav[i + 1u];
        fav[FAV_N - 1u] = FAV_NONE;
        fav_at = -1;
        ui_message("FAV REMOVED");
    }
    fav_dirty = 1;
    ui.force = 1;
}
/* (2.7) the settings' side record: the theme (40 bytes, zero: none), then the favourites (ui.c fav) with a magic and a
 * CRC. A 2.7 beta wrote the theme alone (the rest erased: no favourites); an older SLOOP never reads it */
#define FAVS_MAGIC 0x31564146u                      /* "FAV1" */
typedef struct {
    theme_t th;
    uint32_t fmagic;
    uint16_t fav[FAV_N];
    uint32_t fcrc;
    uint32_t fork_magic, fork_flags;                 /* upstream 80-byte prefix is unchanged */
} side_rec_t;
#define FORK_SETTINGS_MAGIC 0x3153524du               /* MRS1 */
UI_SMALL static void side_fill(side_rec_t *r)                /* what the settings' side record holds now */
{
    memset(r, 0, sizeof *r);
    if (theme_valid)
        r->th = theme;
    r->fmagic = FAVS_MAGIC;
    memcpy(r->fav, fav, sizeof fav);
    r->fcrc = st_crc32(&r->fmagic, 4u + sizeof r->fav);
    r->fork_magic = FORK_SETTINGS_MAGIC;
    r->fork_flags = fork_settings_word();
}
UI_SMALL static void side_take(side_rec_t *r)                /* a side record read back: what checks out (theme, favourites) */
{
    if (r->fork_magic == FORK_SETTINGS_MAGIC)
        fork_settings_from_word(r->fork_flags);
    if (theme_check(&r->th)) {
        theme = r->th;
        theme_valid = 1;
    }
    if (r->fmagic == FAVS_MAGIC && r->fcrc == st_crc32(&r->fmagic, 4u + sizeof r->fav))
        memcpy(fav, r->fav, sizeof fav);
}
/* the PRESETS list: the favourites, then every factory sound by kind, then the user presets */
UI_SMALL static uint32_t preset_pos(uint32_t *total)
{
    uint32_t b[FAV_N], nf = fav_list(b), cur = base_pos(total);
    *total += nf;
    if (fav_at >= 0 && (uint32_t)fav_at < nf && b[fav_at] == cur)
        return (uint32_t)fav_at;
    return nf + cur;
}
UI_SMALL static uint32_t preset_at(uint32_t n, uint32_t *k)
{
    uint32_t b[FAV_N], nf = fav_list(b);
    return base_at(n < nf ? b[n] : n - nf, k);
}
UI_SMALL static const char *preset_kind(uint32_t n)
{
    uint32_t b[FAV_N], nf = fav_list(b);
    return n < nf ? "FAV" : base_kind(n - nf);
}

/* list index n into track t (2.6: MIDI Program Change, any track); 0 done, 1 no such sound */
UI_SMALL static int preset_go_to(track_t *t, uint32_t n)
{
    uint32_t k, total, e;
    int16_t fx[TRACK_BROWSE_COUNT];int result=0;
    if (is_drum(t))
        return 1;
    total = NBANK + up_count();
    if (n >= total)
        return 1;
    e = base_at(n, &k);
    track_fx_values(t,fx,0);
    if (e == NENGINES) result=up_load_to(t,k);
    else {
        if (e != t->eng_req) set_engine_of(t,e);
        apply_preset_to(t,k);
    }
    track_fx_values(t,fx,1);
    ui.force = 1;
    return result;
}
/* MIDI Program Change (2.6, after Felucca 1.5.1, PR #179 by @renebohne): seq.c queues it per track (the track its
 * channel plays, GLO > MIDI); here, in the main loop (a flash read, a sound loaded): a synth track takes the n-th sound
 * of the PRESETS list (0 the first factory sound, the user presets after the 153), the drum track kit n. A number past
 * the end does nothing (no wrap) */
UI_SMALL static void pc_poll(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        uint32_t r, bank, n;
        char b[12];
        fm1_irq_off();
        r = pc_req[i]; bank = pc_bank[i];
        if (r) pc_req[i] = 0;
        fm1_irq_on();
        if (!r)
            continue;
        browse_end(0); /* External Program Change ends a local audition first. */
        n = r - 1u;
        if (is_drum(&trk[i])) {
            if (n >= DRUM_KITS)
                continue;
            trk[i].p[P_E0] = (int16_t)n;
            if (&trk[i] == TSEL)
                sync_reload = 1;
        } else {
            int32_t index = up_pc_index(bank, n, NBANK);
            if (index < 0 || preset_go_to(&trk[i], (uint32_t)index)) continue;
            n = (uint32_t)index;
        }
        str_cpy(b, "T1 PROG ", sizeof b);
        b[1] = (char)('1' + i);
        fmt_int(b + 8, (int32_t)n);
        ui_message(b);
        ui.force = 1;
    }
}
UI_SMALL static void preset_go(uint32_t n)                    /* load list index n into the selected track */
{
    uint32_t k, e = preset_at(n, &k), b[FAV_N];
    if (is_drum(TSEL))
        return;                                      /* one GM kit: nothing to browse */
    fav_at = (int8_t)(n < fav_list(b) ? (int32_t)n : -1);
    if (e == NENGINES) {
        int16_t fx[TRACK_BROWSE_COUNT];
        track_fx_values(TSEL, fx, 0);
        up_load(k);
        track_fx_values(TSEL, fx, 1);
        return;
    }
    if (e != TSEL->eng_req)
        select_engine(e);
    apply_preset(k);
    ui.force = 1;
}

#include "ui_sound_browser.c"

/* HOME: what KNOB k edits: the engine's four main parameters; on the drum track
 * LEVEL and REV (GLO > DRUMS), PAN and LEN */
UI_SMALL static const param_desc_t *home_param(uint32_t k, int16_t **vp)
{
    static const uint8_t DRUM_HOME[4][2] = {{1, G_DRLVL}, {1, G_DRREV}, {0, P_PAN}, {0, P_SLEN}};
    uint32_t id;
    if (is_drum(TSEL)) {
        id = DRUM_HOME[k & 3u][1];
        if (DRUM_HOME[k & 3u][0]) {
            *vp = &song.g[id];
            return &GP[id];
        }
        *vp = &TSEL->p[id];
        return &TP[id];
    }
    id = ENGINES[TSEL->eng_req % NENGINES]->macro[k & 3u];
    *vp = &TSEL->p[id];
    return track_desc(TSEL, id);
}

/* select track i (KNOB 1 on TRACKS, the editor): its sound, pages and pattern from now on */
UI_SMALL static void track_select(uint32_t i)
{
    groove_preview.active = 0;
    if (i >= NTRK || i == song.sel)
        return;
    fm1_irq_off();progression_stop();fm1_irq_on();
    sequence_screen_close();
    browse_end(0);
    if (cur_page()->scope == SC_STARTER) go_home();
    song.sel = (uint8_t)i;
    ui.roll_pitch=0;ui.roll_project=project_revision;
    ui.note_context=0;
    ui.tie_start=255;ui.tie_voice=0;
    rec_follow(i);                                   /* LIVE: recording follows the selected track */
    ui.entry_open = 0;
    ui.cursor = 0;
    ui.bank = 0;
    sync_reload = 1;
    ui.force = 1;
}

/* ------------------------------------------------------- MIDI LEARN --- */
/* MIDI LEARN (SLOOP 2.6, after Felucca 1.5 by Leo Kuroshita, Discussion #170; the map: seq.c ml_tab). GLO held +
 * the black key F#3 turns it on and off. On: a knob turned on a track's page (the sound's pages, HOME, TRACKS' LEVEL
 * and PAN) picks its parameter on the selected track as it edits it; the next CC that comes (any channel IN hears) is
 * set to it, and stays until something else is learned on it or HOME > SYSTEM > LEARN CLEAR. Then turn the next knob.
 * One CC sets one parameter, a parameter has one CC: learning one again replaces both. The settings keep the map */
static struct { uint8_t on, pick, trk, id; } mlu;
UI_SMALL static uint32_t ml_code(uint32_t id) { return id >= P_E0 ? ML_E0 + id - P_E0 : id + 1u; }
UI_SMALL static void ml_put(uint32_t i, uint32_t e)       /* (the ISR reads the entry: never half of one) */
{
    fm1_irq_off();
    ml_tab[i % ML_N] = (uint16_t)e;
    fm1_irq_on();
}
UI_SMALL static uint32_t ml_count(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < ML_N; i++)
        n += (ml_tab[i] >> 9) != 0u;
    return n;
}
/* cc -> parameter id of track k, in place of what either had; 0 = no room, 2 = a CC that is never learned */
UI_SMALL static uint32_t ml_learn(uint32_t cc, uint32_t k, uint32_t id)
{
    uint32_t i, e;
    if (!ml_free_cc(cc) || k >= NTRK || id >= P_COUNT)
        return 2;
    for (i = 0; i < ML_N; i++)
        if ((e = ml_tab[i]) >> 9 && ((e & 127u) == cc || ((e >> 9) == ml_code(id) && ((e >> 7) & 3u) == k)))
            ml_put(i, 0);
    for (i = 0; i < ML_N; i++)
        if (!(ml_tab[i] >> 9)) {
            ml_put(i, cc | k << 7 | ml_code(id) << 9);
            return 1;
        }
    return 0;
}
UI_SMALL static int ml_clear_all(void)                    /* HOME > SYSTEM > LEARN CLEAR; 1 = there were */
{
    uint32_t i, n = ml_count();
    for (i = 0; i < ML_N; i++)
        ml_put(i, 0);
    if (n)
        settings_later = 1;                      /* (saved when quiet) */
    return n != 0u;
}
UI_SMALL static void ml_toggle(void)                      /* GLO + F#3 */
{
    mlu.on = (uint8_t)!mlu.on;
    mlu.pick = 0;
    ml_arm = 0;
    ml_heard = 0;
    ui_message(mlu.on ? "LEARN: TURN A KNOB" : "LEARN DONE");
    ui.force = 1;
}
/* a knob turned parameter id of track t: picked while learning (ui_input.c) */
UI_SMALL static void ml_knob(const track_t *t, uint32_t id)
{
    char b[16];
    if (!mlu.on || id >= P_COUNT)
        return;
    if (!mlu.pick || mlu.id != id || mlu.trk != (uint8_t)trk_index(t)) {
        mlu.trk = (uint8_t)trk_index(t);
        mlu.id = (uint8_t)id;
        b[0] = 'T';
        b[1] = (char)('1' + mlu.trk);
        b[2] = ' ';
        str_cpy(b + 3, track_desc(t, id)->label, 9);
        ui_say(b, ": SEND A CC");
    }
    mlu.pick = 1;
    ml_heard = 0;
    ml_arm = 1;
}
/* each frame: a CC heard for the picked parameter is learned */
UI_SMALL static void ml_poll(void)
{
    uint32_t cc = ml_heard, r;
    char a[16], b[16];
    if (!cc)
        return;
    ml_heard = 0;
    if (!mlu.on || !mlu.pick)
        return;
    cc--;
    r = ml_learn(cc, mlu.trk, mlu.id);
    if (r != 1u) {
        ui_message(r ? "CC NOT LEARNABLE" : "LEARN FULL");
        return;
    }
    str_cpy(a, "CC", 4);
    fmt_int(a + 2, (int32_t)cc);
    str_cpy(a + str_len(a), " = T", 5);
    b[0] = (char)('1' + mlu.trk);
    b[1] = ' ';
    str_cpy(b + 2, track_desc(&trk[mlu.trk % NTRK], mlu.id)->label, 9);
    ui_say(a, b);
    mlu.pick = 0;
    ml_arm = 0;
    settings_later = 1;                          /* (saved when quiet) */
}
