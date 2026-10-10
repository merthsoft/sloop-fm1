/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SLOOP_STARTER_PATTERN_H
#define SLOOP_STARTER_PATTERN_H
/* Both starter libraries replace metadata after taking their pattern undo. */
static void starter_pattern_metadata(track_t *t, uint32_t len, uint32_t div)
{
    memset(t->fill, 0, sizeof t->fill);
    memset(t->lock, 0, sizeof t->lock);
    for (uint32_t i = 0; i < NLOCK; i++) t->lock[i].step = LOCK_FREE;
    t->p[P_SLEN] = (int16_t)len; t->p[P_SDIV] = (int16_t)div; t->p[P_SSWING] = 0;
    t->seq_active = 1;
}
#endif
