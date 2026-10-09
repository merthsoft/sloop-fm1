/* SPDX-License-Identifier: GPL-3.0-only */
/* ROM musical starters: four scale-degree bars, no patch or song state.
 * Included after drum_grooves.c in ui.c; shares its supplemental undo. */
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
    {"FLOATING LYDIAN",0x0401, {0,1,4,1}, 1}
};
#define NSEQUENCE_STARTERS NELEM(SEQUENCE_STARTERS)
enum { STARTER_CHORD, STARTER_BASS, STARTER_ARP };
static rhythm_shape_t sequence_shape = {0,0,0,255,0};
static uint8_t sequence_sel, sequence_root, sequence_scale = 1, sequence_mode;
static struct {
    uint32_t phase, gate_off;
    uint8_t active, first, step, track, notes[4], n;
    uint8_t starter, root, scale, mode, hold;
    int8_t octave;
    rhythm_shape_t shape;
} sequence_preview;

static uint32_t sequence_degree_note(uint32_t degree, uint32_t root, uint32_t scale, int32_t octave)
{
    uint32_t mask = SCALE_MASK[scale % NSCALES], count = 0, pc;
    for (pc = 0; pc < 12; pc++) count += (mask >> pc) & 1u;
    octave += (int32_t)(degree / count);
    degree %= count;
    for (pc = 0; pc < 12; pc++) if ((mask >> pc) & 1u) {
        if (!degree) break;
        degree--;
    }
    return (uint32_t)clamp(60 + (int32_t)root + 12 * octave + (int32_t)pc, 0, 127);
}

static step_t sequence_starter_step(uint32_t id, uint32_t i, uint32_t root,
                                   uint32_t scale, int32_t octave, uint32_t mode,
                                   const rhythm_shape_t *shape)
{
    const sequence_starter_t *p = &SEQUENCE_STARTERS[id % NSEQUENCE_STARTERS];
    uint64_t occupied = (uint64_t)p->hits | ((uint64_t)p->hits << 16) |
                        ((uint64_t)p->hits << 32) | ((uint64_t)p->hits << 48);
    uint32_t src, degree, j, count;
    step_t s = {{0},0,ST_REST,0,0,0,0};
    if (i >= NSTEP) return s;
    src = rhythm_shape_source(shape, NSTEP, i, 0, occupied, 4);
    if (!(occupied & ((uint64_t)1u << src))) {
        if (mode == STARTER_CHORD && p->hits == 1u) s.time = ST_TIE;
        return s;
    }
    degree = p->degree[src / 16u];
    s.time = ST_NOTE; s.vel = (src % 4u) ? 92 : 108;
    if (!(src % 4u)) s.flags = SF_ACCENT;
    if (mode == STARTER_BASS) { octave--; s.n = 1; }
    else if (mode == STARTER_ARP) { degree += (src % 16u / 2u % 3u) * 2u; s.n = 1; }
    else s.n = p->seventh ? 4 : 3;
    count = s.n; s.n = 0;
    for (j = 0; j < count; j++) {
        uint8_t note = (uint8_t)sequence_degree_note(degree + 2u * j, root, scale, octave);
        if (!s.n || s.note[s.n-1] != note) s.note[s.n++] = note;
    }
    return s;
}

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
        s = sequence_starter_step(sequence_preview.starter, sequence_preview.step,
            sequence_preview.root, sequence_preview.scale, sequence_preview.octave,
            sequence_preview.mode, &sequence_preview.shape);
        if (s.time != ST_TIE) {
            for (j = 0; j < sequence_preview.n; j++)
                trk_note_off(&trk[sequence_preview.track], sequence_preview.notes[j]);
            sequence_preview.n = s.n;
            for (j = 0; j < s.n; j++) {
                sequence_preview.notes[j] = s.note[j];
                trk_note_on(&trk[sequence_preview.track], s.note[j], (s.flags & SF_ACCENT) ? 127u : s.vel);
            }
        }
        s = sequence_starter_step(sequence_preview.starter,(sequence_preview.step+1u)%NSTEP,
            sequence_preview.root,sequence_preview.scale,sequence_preview.octave,
            sequence_preview.mode,&sequence_preview.shape);
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
        t->step[i] = sequence_starter_step(sequence_sel,i,sequence_root,sequence_scale,
                                         song.octave,sequence_mode,&sequence_shape);
        t->micro[i] = rhythm_shape_micro(&sequence_shape,0);
    }
    memset(t->fill,0,sizeof t->fill);
    memset(t->lock,0,sizeof t->lock);
    for (i = 0; i < NLOCK; i++) t->lock[i].step = LOCK_FREE;
    t->p[P_SLEN] = NSTEP; t->p[P_SDIV] = 2; t->p[P_SSWING] = 0;
    t->seq_active = 1;
    fm1_irq_on();
    ui.step_sess = 0; sync_reload = 1; ui.force = 1;
    return 1;
}
