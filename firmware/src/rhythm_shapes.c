/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure ROM-to-pattern mapping, shared by drums and musical starters. */
#ifndef SLOOP_RHYTHM_SHAPES_C
#define SLOOP_RHYTHM_SHAPES_C
#include <stdint.h>
#define RHYTHM_PART_ALL 255u
typedef struct {
    int8_t rotate, offset, feel;
    uint8_t part, sync;
} rhythm_shape_t;

static uint32_t rhythm_shape_wrap(int32_t i, uint32_t len)
{
    int32_t r = i % (int32_t)len;
    return (uint32_t)(r < 0 ? r + (int32_t)len : r);
}
/* A disjoint adjacent swap, never a move onto another hit. The original
 * occupancy is authoritative, including before rotation. On partial bars a
 * wrap pair is skipped if its target is itself a beat start. */
static int rhythm_shape_pair(const rhythm_shape_t *s, uint32_t len,
                             uint32_t strong, uint64_t occupied, uint32_t beat)
{
    uint32_t before;
    if (!s->sync || beat < 2 || strong % beat) return 0;
    if (s->sync == 1 && (strong / beat) % 2) return 0;
    before = strong ? strong - 1 : len - 1;
    if (before % beat == 0) return 0;
    return ((occupied >> strong) & 1u) && !((occupied >> before) & 1u);
}
/* dest -> original source, for len 1..64. Invalid lengths return zero without
 * reading masks or dividing. Source metadata must travel with its event. */
static uint32_t rhythm_shape_source(const rhythm_shape_t *s, uint32_t len,
                                   uint32_t dest, uint32_t part,
                                   uint64_t occupied, uint32_t beat)
{
    uint32_t at, next;
    int32_t shift;
    if (!len || len > 64) return 0;
    if (!s) return dest % len;
    shift = s->rotate;
    if (s->part == RHYTHM_PART_ALL || s->part == part) shift += s->offset;
    at = rhythm_shape_wrap((int32_t)(dest % len) - shift, len);
    if (rhythm_shape_pair(s, len, at, occupied, beat)) return at ? at - 1 : len - 1;
    next = at + 1 == len ? 0 : at + 1;
    if (rhythm_shape_pair(s, len, next, occupied, beat)) return next;
    return at;
}
static int8_t rhythm_shape_micro(const rhythm_shape_t *s, int32_t original)
{
    int32_t m = original + (s ? s->feel : 0);
    return (int8_t)(m < -32 ? -32 : m > 31 ? 31 : m);
}
#endif
