/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after drums.c and seq.c. No transport, undo or selection ownership. */
#ifndef DRUM_HIT_ACTIONS_C
#define DRUM_HIT_ACTIONS_C
enum { DRUM_HIT_OK = 1, DRUM_HIT_NOOP = 0, DRUM_HIT_BOUNDS = -1,
       DRUM_HIT_EMPTY = -2, DRUM_HIT_OCCUPIED = -3 };

static int drum_hit_delete(track_t *t, uint32_t lane, uint32_t source, int write)
{
    if (!t || lane >= DRUM_LANES || source >= NSTEP || source >= trk_len(t))
        return DRUM_HIT_BOUNDS;
    if (!dstep_has(&t->dstep[source], lane)) return DRUM_HIT_EMPTY;
    if (write) dstep_clr(&t->dstep[source], lane);
    return DRUM_HIT_OK;
}

static int drum_hit_place(track_t *t, uint32_t lane, uint32_t source,
                          uint32_t dest, int move, int write)
{
    int result = drum_hit_delete(t, lane, source, 0);
    if (result < 0) return result;
    if (dest >= NSTEP || dest >= trk_len(t)) return DRUM_HIT_BOUNDS;
    if (move && source == dest) return DRUM_HIT_NOOP;
    if (dstep_has(&t->dstep[dest], lane)) return DRUM_HIT_OCCUPIED;
    if (write) {
        dstep_set(&t->dstep[dest], lane, dstep_lvl(&t->dstep[source], lane),
                  dstep_rat(&t->dstep[source], lane));
        if (move) dstep_clr(&t->dstep[source], lane);
    }
    return DRUM_HIT_OK;
}
#endif
