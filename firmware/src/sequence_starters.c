/* SPDX-License-Identifier: GPL-3.0-only */
/* ROM musical starters: four scale-degree bars, no patch or song state.
 * Included after drum_grooves.c in ui.c; shares its supplemental undo. */
#include "harmony_voicing.h"
#include "starter_pattern.h"
typedef struct {
    const char *name;
    uint16_t hits;
    uint8_t degree[4], seventh;
} sequence_starter_t;
static const sequence_starter_t SEQUENCE_STARTERS[] = {
    {"POP FOUR",       0x0001, {0,4,5,3}, 0},
    {"CLASSIC TURN",   0x0001, {0,5,3,4}, 0},
    {"TWO FIVE ONE",   0x0101, {1,4,0,0}, 1},
    {"MINOR JOURNEY",  0x0001, {0,5,2,6}, 0},
    {"SOUL SEVENTHS",  0x0441, {0,5,1,4}, 1},
    {"DORIAN POCKET",  0x1249, {0,3,0,6}, 1},
    {"HOUSE PUSH",     0x4444, {0,5,3,4}, 0},
    {"BROKEN TWO FIVE",0x0949, {1,4,0,5}, 1},
    {"FUNK SIDE STEP", 0x4925, {0,1,3,1}, 1},
    {"FIFTHS WALK",    0x1111, {5,1,4,0}, 0},
    {"SEVENTH SKIP",   0x2249, {0,6,3,4}, 1},
    {"FLOATING LYDIAN",0x0401, {0,1,4,1}, 1},
    {"GOSPEL TURN",   0x0101, {0,3,0,4}, 1},
    {"MINOR DESCENT", 0x0001, {0,6,5,4}, 0},
    {"SIX TWO FIVE",  0x0101, {5,1,4,0}, 1},
    {"SOUL DETOUR",   0x0449, {1,3,6,0}, 1},
    {"DEEP TWO CHORD",0x1111, {0,3,0,3}, 1},
    {"DISCO LIFT",    0x5555, {0,5,1,4}, 0},
    {"GARAGE SKIPS",  0x2449, {0,2,5,3}, 1},
    {"LATIN TURN",    0x0925, {0,3,4,0}, 0},
    {"ODD POCKET",    0x1249, {0,1,5,4}, 0},
    {"SUSPENSE",      0x0101, {0,6,0,1}, 0},
    {"RISING STEPS",  0x2222, {0,1,2,3}, 0},
    {"FALLING HOME",  0x0909, {3,2,1,0}, 1}
};
#define NSEQUENCE_STARTERS NELEM(SEQUENCE_STARTERS)

/* Independent one-bar attack masks, reused over the four harmony bars. */
static const struct { const char *name; uint16_t hits; } SEQUENCE_RHYTHMS[] = {
    {"ORIGINAL",0}, {"BAR",0x0001}, {"HALVES",0x0101},
    {"QUARTERS",0x1111}, {"EIGHTHS",0x5555}, {"SIXTEENTHS",0xFFFF},
    {"OFFBEATS",0x4444}, {"SOUL",0x0441}, {"DORIAN",0x1249},
    {"BROKEN",0x0949}, {"FUNK",0x4925}, {"SKIPS",0x2249},
    {"FLOAT",0x0401}, {"GARAGE",0x2449}, {"LATIN",0x0925},
    {"PUSHES",0x2222}, {"FALLING",0x0909}, {"SOUL DETOUR",0x0449}
};
#define NSEQUENCE_RHYTHMS NELEM(SEQUENCE_RHYTHMS)
enum { STARTER_CHORD, STARTER_BASS, STARTER_ARP };
static rhythm_shape_t sequence_shape = {0,0,0,255,0};
static uint8_t sequence_sel, sequence_root, sequence_scale = 1, sequence_mode;
static int8_t sequence_octave;
static uint8_t sequence_vlead;
static uint8_t sequence_rhythm;
static struct {
    uint32_t phase, gate_off;
    uint8_t active, first, step, track, notes[4], n;
    uint8_t starter, root, scale, mode, hold, vlead, rhythm;
    int8_t octave;
    rhythm_shape_t shape;
} sequence_preview;

static uint32_t sequence_degree_note(uint32_t degree, uint32_t root, uint32_t scale, int32_t octave)
{
    uint32_t mask = SCALE_MASK[scale % NSCALES], count = 0, pc;
    /* Count only present tones; pentatonic/diatonic scales need 5/7 iterations. */
    for (pc = mask; pc; pc &= pc - 1u) count++;
    octave += (int32_t)(degree / count);
    degree %= count;
    for (pc = 0; pc < 12; pc++) if ((mask >> pc) & 1u) {
        if (!degree) break;
        degree--;
    }
    return (uint32_t)clamp(60 + (int32_t)root + 12 * octave + (int32_t)pc, 0, 127);
}

static uint32_t sequence_hits(uint32_t id, uint32_t mode, uint32_t rhythm)
{
    if (rhythm && rhythm < NSEQUENCE_RHYTHMS) return SEQUENCE_RHYTHMS[rhythm].hits;
    return SEQUENCE_STARTERS[id % NSEQUENCE_STARTERS].hits | (mode == STARTER_ARP ? 0x5555u : 0u);
}
static step_t sequence_starter_step_rhythm(uint32_t id, uint32_t i, uint32_t root,
                                   uint32_t scale, int32_t octave, uint32_t mode,
                                   const rhythm_shape_t *shape, uint32_t rhythm)
{
    const sequence_starter_t *p = &SEQUENCE_STARTERS[id % NSEQUENCE_STARTERS];
    /* ORIGINAL ARP adds an eighth-note pulse to the starter's syncopated attacks.
     * Sparse chord rhythms must still walk the chord rather than repeat its root. */
    uint32_t hits = sequence_hits(id,mode,rhythm);
    uint64_t occupied = (uint64_t)hits | ((uint64_t)hits << 16) |
                        ((uint64_t)hits << 32) | ((uint64_t)hits << 48);
    uint32_t src, degree, j, count;
    step_t s = {{0},0,ST_REST,0,0,0,0};
    if (i >= NSTEP) return s;
    src = rhythm_shape_source(shape, NSTEP, i, 0, occupied, 4);
    if (!(occupied & ((uint64_t)1u << src))) {
        if (mode == STARTER_CHORD && hits == 1u) s.time = ST_TIE;
        return s;
    }
    degree = p->degree[src / 16u];
    s.time = ST_NOTE; s.vel = (src % 4u) ? 92 : 108;
    if (!(src % 4u)) s.flags = SF_ACCENT;
    if (mode == STARTER_BASS) { octave--; s.n = 1; }
    else if (mode == STARTER_ARP) {
        uint32_t attack = 0;
        for (j = 0; j < src % 16u; j++) attack += (hits >> j) & 1u;
        degree += (attack % (p->seventh ? 4u : 3u)) * 2u;
        s.n = 1;
    }
    else s.n = p->seventh ? 4 : 3;
    count = s.n; s.n = 0;
    for (j = 0; j < count; j++) {
        uint8_t note = (uint8_t)sequence_degree_note(degree + 2u * j, root, scale, octave);
        if (!s.n || s.note[s.n-1] != note) s.note[s.n++] = note;
    }
    return s;
}
static step_t sequence_starter_step(uint32_t id,uint32_t i,uint32_t root,uint32_t scale,
                                   int32_t octave,uint32_t mode,const rhythm_shape_t *shape)
{ return sequence_starter_step_rhythm(id,i,root,scale,octave,mode,shape,0); }

/* Anchor bar one at the requested octave and derive later inversions from it.
 * No live chord history is changed, and repeated preview loops cannot drift. */
static step_t sequence_starter_voiced_rhythm(uint32_t id, uint32_t i, uint32_t root,
                                     uint32_t scale, int32_t octave, uint32_t mode,
                                     const rhythm_shape_t *shape, uint32_t lead, uint32_t rhythm)
{
    step_t s = sequence_starter_step_rhythm(id,i,root,scale,octave,mode,shape,rhythm);
    const sequence_starter_t *p = &SEQUENCE_STARTERS[id % NSEQUENCE_STARTERS];
    uint32_t hits = sequence_hits(id,mode,rhythm);
    uint64_t occupied = (uint64_t)hits | ((uint64_t)hits << 16) |
                        ((uint64_t)hits << 32) | ((uint64_t)hits << 48);
    uint32_t src, bar, j, n = 0, pn = 0;
    uint8_t prev[4], chord[4];
    if (!lead || mode == STARTER_BASS || s.time != ST_NOTE || i >= NSTEP) return s;
    src = rhythm_shape_source(shape,NSTEP,i,0,occupied,4);
    for (bar = 0; bar <= src / 16u; bar++) {
        n = 0;
        for (j = 0; j < (p->seventh ? 4u : 3u); j++) {
            uint8_t note = (uint8_t)sequence_degree_note(p->degree[bar]+2u*j,root,scale,octave);
            if (!n || note > chord[n-1]) chord[n++]=note;
        }
        if (pn) harmony_voice_lead(chord,n,prev,pn,0);
        memcpy(prev,chord,n); pn=n;
    }
    if (mode == STARTER_CHORD) { s.n=(uint8_t)n; memcpy(s.note,chord,n); }
    else {
        uint32_t attack = 0;
        for(j=0;j<src%16u;j++) attack+=(hits>>j)&1u;
        s.note[0]=chord[attack%n];
    }
    return s;
}
static step_t sequence_starter_voiced(uint32_t id,uint32_t i,uint32_t root,uint32_t scale,
                                     int32_t octave,uint32_t mode,const rhythm_shape_t *shape,uint32_t lead)
{ return sequence_starter_voiced_rhythm(id,i,root,scale,octave,mode,shape,lead,0); }

static void sequence_preview_stop(void)
{
    uint32_t j;
    sequence_preview.active = 0;
    for (j = 0; j < sequence_preview.n; j++)
        trk_note_off(&trk[sequence_preview.track], sequence_preview.notes[j]);
    sequence_preview.n = 0;
}

/* Called BEFORE keyboard_block: newly pressed live notes own their note-ons. */
static void sequence_preview_block(uint32_t n)
{
    uint32_t interval, j, fire = 0;
    step_t s;
    if (!sequence_preview.active) return;
    if (song.playing || transport_req || song.rec || rec_wait || ft_on || panic_req ||
        fm1_in.notes || song.sel != sequence_preview.track) {
        sequence_preview_stop(); return;
    }
    interval = div_units(2);
    if (sequence_preview.first) {
        int32_t shift = rhythm_shape_micro(&sequence_preview.shape,0) * (int32_t)interval / 64;
        if (shift < 0) shift += (int32_t)interval;
        if (sequence_preview.phase >= (uint32_t)shift) {
            sequence_preview.phase -= (uint32_t)shift; fire = 1;
        }
    } else if (sequence_preview.phase >= interval) {
        sequence_preview.phase -= interval;
        sequence_preview.step = (uint8_t)((sequence_preview.step + 1u) % NSTEP);
        fire = 1;
    }
    if (fire) {
        sequence_preview.first = 0;
        s = sequence_starter_voiced_rhythm(sequence_preview.starter, sequence_preview.step,
            sequence_preview.root, sequence_preview.scale, sequence_preview.octave,
            sequence_preview.mode, &sequence_preview.shape, sequence_preview.vlead, sequence_preview.rhythm);
        if (s.time != ST_TIE) {
            for (j = 0; j < sequence_preview.n; j++)
                trk_note_off(&trk[sequence_preview.track], sequence_preview.notes[j]);
            sequence_preview.n = s.n;
            for (j = 0; j < s.n; j++) {
                sequence_preview.notes[j] = s.note[j];
                trk_note_on(&trk[sequence_preview.track], s.note[j], (s.flags & SF_ACCENT) ? 127u : s.vel);
            }
        }
        s = sequence_starter_step_rhythm(sequence_preview.starter,(sequence_preview.step+1u)%NSTEP,
            sequence_preview.root,sequence_preview.scale,sequence_preview.octave,
            sequence_preview.mode,&sequence_preview.shape,sequence_preview.rhythm);
        sequence_preview.hold = s.time == ST_TIE;
        sequence_preview.gate_off = interval * (uint32_t)trk[sequence_preview.track].p[P_SGATE] / 128u;
    }
    if (!sequence_preview.hold && sequence_preview.n && sequence_preview.phase >= sequence_preview.gate_off) {
        for (j=0;j<sequence_preview.n;j++)
            trk_note_off(&trk[sequence_preview.track],sequence_preview.notes[j]);
        sequence_preview.n=0;
    }
    sequence_preview.phase += n * (uint32_t)song.g[G_BPM];
}

static int sequence_starter_has_content(const track_t *t)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        if (t->step[i].n || t->micro[i] || step_fill(t,i)) return 1;
    for (i = 0; i < NLOCK; i++) if (t->lock[i].step != LOCK_FREE) return 1;
    return 0;
}

static int sequence_starter_apply(void)
{
    uint32_t i;
    track_t *t;
    if (song.sel >= NPART || song.playing || transport_req || song.rec || rec_wait || ft_on) return 0;
    t = &trk[song.sel];
    fm1_irq_off();
    sequence_preview_stop();
    undo_mark(t, (undo_sess += 4u) | 3u);
    starter_undo_capture(t);
    for (i = 0; i < NSTEP; i++) {
        t->step[i] = sequence_starter_voiced_rhythm(sequence_sel,i,sequence_root,sequence_scale,
                                         sequence_octave,sequence_mode,&sequence_shape,sequence_vlead,sequence_rhythm);
        t->micro[i] = rhythm_shape_micro(&sequence_shape,0);
    }
    starter_pattern_metadata(t, NSTEP, 2);
    fm1_irq_on();
    ui.step_sess = 0; sync_reload = 1; ui.force = 1;
    return 1;
}
