#ifndef UI_SMALL
#define UI_SMALL
#endif
/* SPDX-License-Identifier: GPL-3.0-only */
/* ROM musical starters: four scale-degree bars, no patch or song state.
 * Included after drum_grooves.c in ui.c; shares its supplemental undo. */
#include "harmony_voicing.h"
#include "starter_pattern.h"
typedef struct {
    const char *name;
    uint16_t hits;
    uint16_t degrees; /* four three-bit degrees; bit 12 adds the seventh */
} sequence_starter_t;
#define SEQ_DEGREES(a,b,c,d,seventh) ((a)|((b)<<3)|((c)<<6)|((d)<<9)|((seventh)<<12))
static const sequence_starter_t SEQUENCE_STARTERS[] = {
    {"POP FOUR",       0x0001, SEQ_DEGREES(0,4,5,3,0)},
    {"CLASSIC TURN",   0x0001, SEQ_DEGREES(0,5,3,4,0)},
    {"TWO FIVE ONE",   0x0101, SEQ_DEGREES(1,4,0,0,1)},
    {"MINOR JOURNEY",  0x0001, SEQ_DEGREES(0,5,2,6,0)},
    {"SOUL SEVENTHS",  0x0441, SEQ_DEGREES(0,5,1,4,1)},
    {"DORIAN POCKET",  0x1249, SEQ_DEGREES(0,3,0,6,1)},
    {"HOUSE PUSH",     0x4444, SEQ_DEGREES(0,5,3,4,0)},
    {"BROKEN TWO FIVE",0x0949, 6689},
    {"FUNK SIDE STEP", 0x4925, SEQ_DEGREES(0,1,3,1,1)},
    {"FIFTHS WALK",    0x1111, SEQ_DEGREES(5,1,4,0,0)},
    {"SEVENTH SKIP",   0x2249, SEQ_DEGREES(0,6,3,4,1)},
    {"FLOATING LYDIAN",0x0401, 4872},
    {"GOSPEL TURN",   0x0101, SEQ_DEGREES(0,3,0,4,1)},
    {"MINOR DESCENT", 0x0001, SEQ_DEGREES(0,6,5,4,0)},
    {"SIX TWO FIVE",  0x0101, SEQ_DEGREES(5,1,4,0,1)},
    {"SOUL DETOUR",   0x0449, SEQ_DEGREES(1,3,6,0,1)},
    {"DEEP TWO CHORD",0x1111, 5656},
    {"DISCO LIFT",    0x5555, SEQ_DEGREES(0,5,1,4,0)},
    {"GARAGE SKIPS",  0x2449, SEQ_DEGREES(0,2,5,3,1)},
    {"LATIN TURN",    0x0925, SEQ_DEGREES(0,3,4,0,0)},
    {"ODD POCKET",    0x1249, SEQ_DEGREES(0,1,5,4,0)},
    {"SUSPENSE",      0x0101, SEQ_DEGREES(0,6,0,1,0)},
    {"RISING STEPS",  0x2222, SEQ_DEGREES(0,1,2,3,0)},
    {"FALLING HOME",  0x0909, SEQ_DEGREES(3,2,1,0,1)}
};
#define NSEQUENCE_STARTERS NELEM(SEQUENCE_STARTERS)

/* A native draft; ordinary applied steps use the existing project format. */
static uint8_t sequence_custom_degree[8]={0,4,5,3,0,0,0,0};
static uint8_t sequence_custom_count=4, sequence_custom_span=2;
static inline __attribute__((always_inline)) uint32_t sequence_span(uint32_t id)
{ return id==NSEQUENCE_STARTERS ? 4u<<sequence_custom_span : 16u; }
static inline __attribute__((always_inline)) uint32_t sequence_length(uint32_t id)
{ return id==NSEQUENCE_STARTERS ? sequence_custom_count*sequence_span(id) : NSTEP; }
static uint32_t sequence_degree(uint32_t id,uint32_t slot)
{ return id==NSEQUENCE_STARTERS ? sequence_custom_degree[slot] : (SEQUENCE_STARTERS[id%NSEQUENCE_STARTERS].degrees>>(slot*3u))&7u; }
static uint64_t sequence_occupied(uint32_t id,uint32_t hits)
{
    uint32_t span=sequence_span(id),mask=hits&((1u<<span)-1u);
    if(span==4) mask|=mask<<4;
    if(span<=8) mask|=mask<<8;
    mask|=mask<<16;
    return (uint64_t)mask|((uint64_t)mask<<32);
}

/* Independent one-bar attack masks, reused over the four harmony bars. */
static const struct __attribute__((packed)) { const char *name; uint16_t hits; } SEQUENCE_RHYTHMS[] = {
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
    uint32_t mask=SCALE_MASK[scale%NSCALES],pc=0;
    for(;;pc++) if((mask>>(pc%12u))&1u) {
        if(!degree) break;
        degree--;
    }
    return (uint32_t)clamp(60+(int32_t)root+12*octave+(int32_t)pc,0,127);
}

static uint32_t sequence_hits(uint32_t id, uint32_t mode, uint32_t rhythm)
{
    if (rhythm && rhythm < NSEQUENCE_RHYTHMS) return SEQUENCE_RHYTHMS[rhythm].hits;
    return (id==NSEQUENCE_STARTERS?1u:SEQUENCE_STARTERS[id % NSEQUENCE_STARTERS].hits) | (mode == STARTER_ARP ? 0x5555u : 0u);
}
/* One mapper for plain and voice-led rendering: preview, phone and Apply agree. */
static step_t sequence_starter_voiced_rhythm(uint32_t id,uint32_t i,uint32_t root,
        uint32_t scale,int32_t octave,uint32_t mode,const rhythm_shape_t *shape,
        uint32_t lead,uint32_t rhythm)
{
    step_t s={{0},0,ST_REST,0,0,0,0};
    uint32_t len=sequence_length(id),span=sequence_span(id),hits=sequence_hits(id,mode,rhythm);
    if(i>=len) return s;
    uint64_t occupied=sequence_occupied(id,hits);
    uint32_t src=rhythm_shape_source(shape,len,i,0,occupied,4);
    if(!(occupied&((uint64_t)1u<<src))) {
        if(mode==STARTER_CHORD && hits==1u) s.time=ST_TIE;
        return s;
    }
    s.time=ST_NOTE;s.vel=src%4u?92:108;
    if(!(src%4u)) s.flags=SF_ACCENT;
    uint32_t bar=src/span,j,n=0,pn=0;
    uint32_t count=id!=NSEQUENCE_STARTERS && (SEQUENCE_STARTERS[id%NSEQUENCE_STARTERS].degrees&4096u)?4u:3u;
    uint32_t attack=0;
    if(mode==STARTER_ARP) for(j=0;j<src%span;j++) attack+=(hits>>j)&1u;
    if(mode==STARTER_BASS || (mode==STARTER_ARP && !lead)) {
        uint32_t degree=sequence_degree(id,bar);
        if(mode==STARTER_BASS) octave--; else degree+=2u*(attack%count);
        s.n=1;s.note[0]=(uint8_t)sequence_degree_note(degree,root,scale,octave);
        return s;
    }
    uint8_t chord[4];
    for(uint32_t at=lead?0:bar;at<=bar;at++) {
        n=0;
        for(j=0;j<count;j++) {
            uint8_t note=(uint8_t)sequence_degree_note(sequence_degree(id,at)+2u*j,root,scale,octave);
            if(!n || note>chord[n-1]) chord[n++]=note;
        }
        if(pn) harmony_voice_lead(chord,n,s.note,pn,0);
        memcpy(s.note,chord,n);pn=n;
    }
    if(mode==STARTER_ARP) {s.n=1;s.note[0]=chord[attack%n];}
    else s.n=(uint8_t)n;
    return s;
}
static step_t sequence_starter_step_rhythm(uint32_t id,uint32_t i,uint32_t root,
        uint32_t scale,int32_t octave,uint32_t mode,const rhythm_shape_t *shape,uint32_t rhythm)
{ return sequence_starter_voiced_rhythm(id,i,root,scale,octave,mode,shape,0,rhythm); }
static step_t sequence_starter_step(uint32_t id,uint32_t i,uint32_t root,uint32_t scale,
        int32_t octave,uint32_t mode,const rhythm_shape_t *shape)
{ return sequence_starter_step_rhythm(id,i,root,scale,octave,mode,shape,0); }
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
    interval = swing_step_units(2,song.g[G_SWING],sequence_preview.step);
    if (sequence_preview.first) {
        int32_t shift = rhythm_shape_micro(&sequence_preview.shape,0) * (int32_t)interval / 64;
        if (shift < 0) shift += (int32_t)interval;
        if (sequence_preview.phase >= (uint32_t)shift) {
            sequence_preview.phase -= (uint32_t)shift; fire = 1;
        }
    } else if (sequence_preview.phase >= (interval=swing_preview_units(2,song.g[G_SWING],sequence_preview.step,sequence_preview.shape.feel))) {
        sequence_preview.phase -= interval;
        sequence_preview.step = (uint8_t)((sequence_preview.step + 1u) % sequence_length(sequence_preview.starter));
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
        s = sequence_starter_step_rhythm(sequence_preview.starter,(sequence_preview.step+1u)%sequence_length(sequence_preview.starter),
            sequence_preview.root,sequence_preview.scale,sequence_preview.octave,
            sequence_preview.mode,&sequence_preview.shape,sequence_preview.rhythm);
        sequence_preview.hold = s.time == ST_TIE;
        interval=swing_step_units(2,song.g[G_SWING],sequence_preview.step);
        sequence_preview.gate_off = interval * (uint32_t)trk[sequence_preview.track].p[P_SGATE] / 128u;
    }
    if (!sequence_preview.hold && sequence_preview.n && sequence_preview.phase >= sequence_preview.gate_off) {
        for (j=0;j<sequence_preview.n;j++)
            trk_note_off(&trk[sequence_preview.track],sequence_preview.notes[j]);
        sequence_preview.n=0;
    }
    sequence_preview.phase += n * (uint32_t)song.g[G_BPM];
}

UI_SMALL static int sequence_starter_has_content(const track_t *t)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++)
        if (t->step[i].n || t->micro[i] || step_fill(t,i)) return 1;
    for (i = 0; i < NLOCK; i++) if (t->lock[i].step != LOCK_FREE) return 1;
    return 0;
}

/* Ties hold a whole step, so independent layers share attacks and dynamics. */
UI_SMALL static int sequence_starter_add(track_t *t, step_t *out)
{
    const step_t empty = {{0}}, *old = &empty, *prev = &empty;
    step_t add = {{0}};
    uint32_t len = trk_len(t), i, j, k;
    for (i = len; i-- > 0;) if (t->step[i].time != ST_TIE) { old = &t->step[i]; break; }
    for (i = len; i-- > 0;) if (out[i].time != ST_TIE) { add = out[i]; break; }
    for (i = 0; i < len; i++) {
        uint32_t attack = t->step[i].time == ST_NOTE && t->step[i].n;
        attack |= out[i].time == ST_NOTE && out[i].n;
        if (t->step[i].time != ST_TIE) old = &t->step[i];
        if (out[i].time != ST_TIE) add = out[i];
        step_t s = old->time == ST_NOTE && old->n ? *old : add;
        s.n = 0;
        for (uint32_t part = 0; part < 2u; part++) {
            const step_t *src = part ? &add : old;
            if (src->time != ST_NOTE) continue;
            for (j = 0; j < src->n; j++) {
                for (k = 0; k < s.n && s.note[k] != src->note[j]; k++) ;
                if (k < s.n) continue;
                if (s.n == 4) return -1;
                s.note[s.n++] = src->note[j];
                s.lvl = (s.lvl & ~(3u << (k*2))) | (((src->lvl >> (j*2)) & 3u) << (k*2));
                s.rat = (s.rat & ~(3u << (k*2))) | (((src->rat >> (j*2)) & 3u) << (k*2));
            }
        }
        s.time = !s.n ? ST_REST : !attack && s.n == prev->n &&
            !memcmp(s.note, prev->note, s.n) ? ST_TIE : ST_NOTE;
        if (s.time == ST_TIE) s.flags = t->step[i].flags | out[i].flags;
        out[i] = s; prev = &out[i];
    }
    return 1;
}
UI_SMALL static int sequence_starter_apply_mode(uint32_t additive)
{
    uint32_t i;
    track_t *t;
    step_t steps[NSTEP];
    if (song.sel >= NPART || song.playing || transport_req || song.rec || rec_wait || ft_on) return 0;
    t = &trk[song.sel];
    for (i = 0; i < NSTEP; i++)
        steps[i] = sequence_starter_voiced_rhythm(sequence_sel,i,sequence_root,sequence_scale,
                        sequence_octave,sequence_mode,&sequence_shape,sequence_vlead,sequence_rhythm);
    if (additive && sequence_starter_add(t, steps) < 0) return -1;
    fm1_irq_off();
    sequence_preview_stop();
    undo_mark(t, (undo_sess += 4u) | 3u);
    starter_undo_capture(t);
    for (i = 0; i < (additive ? trk_len(t) : NSTEP); i++) {
        if (!additive || (t->step[i].time != ST_TIE && !t->step[i].n))
            t->micro[i] = rhythm_shape_micro(&sequence_shape,0);
        t->step[i] = steps[i];
    }
    if (!additive) starter_pattern_metadata(t, sequence_length(sequence_sel), 2);
    else t->seq_active = 1;
    fm1_irq_on();
    ui.step_sess = 0; sync_reload = 1; ui.force = 1;
    return 1;
}
UI_SMALL static int sequence_starter_apply(void) { return sequence_starter_apply_mode(0); }
