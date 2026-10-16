/* SPDX-License-Identifier: GPL-3.0-only */
/* ROM starters. Masks cover up to 64 steps, bit zero is step 1.
 * Canonical lanes: kick 0, snare 2, clap 3, closed hat 4, open hat 5.
 * No kit, mixer, song or synth state belongs to a starter. */
#include "starter_pattern.h"
/* Masks keep their natural width; aligned arrays are read only while building
 * or auditioning a groove, never decoded on a normal drum hit. */
typedef struct {
    const char *name;
    const void *mask;
    uint8_t len, div;
} drum_groove_t;
#define GROOVE_ROM(X) \
    X(g00, "FOUR FLOOR", uint16_t, 16, 2, 0x1111, 0x1010, 0, 0x5555, 0, 0x1111, 0) \
    X(g01, "HOUSE OFFBEAT", uint16_t, 16, 2, 0x1111, 0, 0x1010, 0x1111, 0x4444, 0x1111, 0) \
    X(g02, "BACKBEAT", uint16_t, 16, 2, 0x0101, 0x1010, 0, 0x5555, 0, 0x1111, 0) \
    X(g03, "HALF TIME", uint16_t, 16, 2, 0x0401, 0x0100, 0, 0x5555, 0, 0x0101, 0) \
    X(g04, "BREAKBEAT", uint16_t, 16, 2, 0x0541, 0x9010, 0, 0x5555, 0x4000, 0x1111, 0x8000) \
    X(g05, "SHUFFLE", uint16_t, 12, 4, 0x0249, 0x0208, 0, 0x0B6D, 0, 0x0249, 0) \
    X(g06, "SMALL FILL", uint16_t, 16, 2, 0x0111, 0xF010, 0, 0x1555, 0, 0x1111, 0x2000) \
    X(g07, "TECHNO DRIVE", uint16_t, 16, 2, 0x1111, 0, 0x1010, 0xBBBB, 0x4444, 0x1111, 0) \
    X(g08, "DISCO", uint16_t, 16, 2, 0x1111, 0x1010, 0, 0xFFFF, 0x4444, 0x1111, 0) \
    X(g09, "HIP HOP", uint16_t, 16, 2, 0x0841, 0x1010, 0, 0x5555, 0x4000, 0x1011, 0) \
    X(g10, "DNB TWO STEP", uint16_t, 16, 2, 0x0401, 0x1010, 0, 0xFFFF, 0x0040, 0x1011, 0) \
    X(g11, "UK GARAGE", uint16_t, 16, 2, 0x0441, 0x1010, 0, 0xAA55, 0x4004, 0x1011, 0) \
    X(g12, "REGGAETON", uint16_t, 16, 2, 0x1111, 0x4848, 0, 0x5555, 0, 0x1111, 0) \
    X(g13, "BOSSA", uint16_t, 16, 2, 0x4181, 0x4924, 0, 0x5555, 0, 0x0101, 0x0800) \
    X(g14, "AMEN BREAK", uint64_t, 64, 2, 0x0001010105010541ULL, 0x1040B41090109010ULL, 0, 0x5555555555555555ULL, 0x4000000040000000ULL, 0x1040101010101011ULL, 0x0000A40080008000ULL) \
    X(g15, "AMEN HALF", uint32_t, 32, 2, 0x05010541ULL, 0x90109010ULL, 0, 0x55555555ULL, 0x40004000ULL, 0x10101011ULL, 0x80008000ULL) \
    X(g16, "FUNK POCKET", uint16_t, 16, 2, 0x4449ULL, 0x9290ULL, 0x0ULL, 0xF7F7ULL, 0x808ULL, 0x1111ULL, 0x8280ULL) \
    X(g17, "BROKEN HOUSE", uint32_t, 32, 2, 0x41090441ULL, 0x10101010ULL, 0x10001000ULL, 0x25252525ULL, 0x80808080ULL, 0x1010101ULL, 0x0ULL) \
    X(g18, "AFRO CLAVE", uint16_t, 16, 2, 0x441ULL, 0x1010ULL, 0x1449ULL, 0x5555ULL, 0x8080ULL, 0x441ULL, 0x0ULL) \
    X(g19, "ELECTRO", uint16_t, 16, 2, 0x509ULL, 0x1010ULL, 0x1000ULL, 0xB515ULL, 0x4040ULL, 0x101ULL, 0x0ULL) \
    X(g20, "BOOM BAP", uint32_t, 32, 2, 0x4410481ULL, 0x90101010ULL, 0x0ULL, 0xD555D555ULL, 0x800080ULL, 0x10011001ULL, 0x80000000ULL) \
    X(g21, "JUNGLE GHOST", uint32_t, 32, 2, 0x2090409ULL, 0x50905090ULL, 0x0ULL, 0xFFBFFFBFULL, 0x400040ULL, 0x10101010ULL, 0x40804080ULL) \
    X(g22, "FIVE STEP", uint32_t, 20, 2, 0x11111ULL, 0x41010ULL, 0x0ULL, 0x55555ULL, 0x88080ULL, 0x10101ULL, 0x40000ULL) \
    X(g23, "SEVEN SKIP", uint16_t, 14, 2, 0x441ULL, 0x1010ULL, 0x0ULL, 0x1555ULL, 0x2200ULL, 0x441ULL, 0x0ULL)
#define GROOVE_MASKS(id,name,type,len,div,...) static const type id##_mask[]={__VA_ARGS__};
GROOVE_ROM(GROOVE_MASKS)
#undef GROOVE_MASKS
#define GROOVE_INFO(id,name,type,len,div,...) {name,id##_mask,len,div},
static const drum_groove_t DRUM_GROOVES[]={ GROOVE_ROM(GROOVE_INFO) };
#undef GROOVE_INFO
#undef GROOVE_ROM
static uint64_t drum_groove_mask(const drum_groove_t *p,uint32_t lane)
{
    if(p->len<=16) return ((const uint16_t *)p->mask)[lane];
    if(p->len<=32) return ((const uint32_t *)p->mask)[lane];
    return ((const uint64_t *)p->mask)[lane];
}
#define NDRUM_GROOVES (sizeof DRUM_GROOVES / sizeof DRUM_GROOVES[0])
_Static_assert(NDRUM_GROOVES <= 24, "groove reply fits editor capacity");
static rhythm_shape_t groove_shape = {0, 0, 0, RHYTHM_PART_ALL, 0};
static uint8_t groove_preview_id;
static dstep_t drum_groove_shaped_step(uint32_t g, uint32_t i, const rhythm_shape_t *shape)
{
    const drum_groove_t *p = &DRUM_GROOVES[g % NDRUM_GROOVES];
    static const uint8_t lanes[5] = {0, 2, 3, 4, 5};
    dstep_t s = {{0}, {0}, {0}};
    uint32_t k;
    uint64_t bit;
    if (i >= p->len) return s;
    for (k = 0; k < 5; k++) {
        uint64_t mask=drum_groove_mask(p,k);
        uint32_t source = rhythm_shape_source(shape, p->len, i, lanes[k], mask,
                                             BEAT_U / div_units(p->div));
        bit = (uint64_t)1u << source;
        if (mask & bit)
            dstep_set(&s, lanes[k], (k == 1 && (drum_groove_mask(p,6) & bit)) ? LV_GHOST :
                      (drum_groove_mask(p,5) & bit) ? LV_HARD : LV_NORM, 0);
    }
    return s;
}
static dstep_t drum_groove_step(uint32_t g, uint32_t i)
{ return drum_groove_shaped_step(g, i, 0); }
static dstep_t drum_groove_preview_step(uint32_t i)
{ return drum_groove_shaped_step(groove_preview_id, i, &groove_shape); }

/* Supplement the existing one-level undo only when its session matches.
 * Any later undo_mark automatically supersedes this snapshot. */
static struct {
    uint32_t sess;
    int8_t micro[NSTEP];
    plock_t lock[NLOCK];
    uint8_t fill[NSTEP / 4], active, trk;
    int16_t div, swing;
} groove_undo;
static int groove_undo_matches(void)
{ return groove_undo.sess && undo.valid && undo.trk == groove_undo.trk && undo.sess == groove_undo.sess; }
static void groove_undo_swap(void)
{
    uint32_t i;
    track_t *t = &trk[groove_undo.trk];
    for (i = 0; i < NSTEP; i++) { int8_t x = t->micro[i]; t->micro[i] = groove_undo.micro[i]; groove_undo.micro[i] = x; }
    for (i = 0; i < NLOCK; i++) { plock_t x = t->lock[i]; t->lock[i] = groove_undo.lock[i]; groove_undo.lock[i] = x; }
    for (i = 0; i < sizeof groove_undo.fill; i++) { uint8_t x = t->fill[i]; t->fill[i] = groove_undo.fill[i]; groove_undo.fill[i] = x; }
    { int16_t x = t->p[P_SDIV]; t->p[P_SDIV] = groove_undo.div; groove_undo.div = x; }
    { int16_t x = t->p[P_SSWING]; t->p[P_SSWING] = groove_undo.swing; groove_undo.swing = x; }
    { uint8_t x = t->seq_active; t->seq_active = groove_undo.active; groove_undo.active = x; }
}
static int drum_groove_has_content(void)
{
    uint32_t i;
    for (i = 0; i < NSTEP; i++) if (dstep_mask(&TDRUM->dstep[i]) || TDRUM->micro[i] || step_fill(TDRUM, i)) return 1;
    for (i = 0; i < NLOCK; i++) if (TDRUM->lock[i].step != LOCK_FREE) return 1;
    return 0;
}
static void starter_undo_capture(track_t *t)
{
    groove_undo.trk = (uint8_t)(t - trk);
    groove_undo.sess = undo.sess;
    memcpy(groove_undo.micro, t->micro, sizeof groove_undo.micro);
    memcpy(groove_undo.lock, t->lock, sizeof groove_undo.lock);
    memcpy(groove_undo.fill, t->fill, sizeof groove_undo.fill);
    groove_undo.div = t->p[P_SDIV]; groove_undo.swing = t->p[P_SSWING]; groove_undo.active = t->seq_active;
}
static int drum_groove_apply_shaped(uint32_t g, const rhythm_shape_t *shape)
{
    const drum_groove_t *p;
    uint32_t i;
    if (song.playing || transport_req == 1 || song.rec || rec_wait || ft_on) return 0;
    if (g >= NDRUM_GROOVES) return 0;
    groove_preview.active = 0;
    p = &DRUM_GROOVES[g];
    fm1_irq_off();
    undo_mark(TDRUM, (undo_sess += 4u) | 3u);
    starter_undo_capture(TDRUM);
    for (i = 0; i < NSTEP; i++) {
        TDRUM->dstep[i] = drum_groove_shaped_step(g, i, shape);
        TDRUM->micro[i] = i < p->len ? rhythm_shape_micro(shape, 0) : 0;
    }
    starter_pattern_metadata(TDRUM, p->len, p->div);
    fm1_irq_on();
    ui.step_sess = 0; sync_reload = 1; ui.force = 1;
    return 1;
}
static int drum_groove_apply(uint32_t g)
{ return drum_groove_apply_shaped(g, 0); }
