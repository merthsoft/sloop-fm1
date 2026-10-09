#include "harmony_owners.h"
#include <string.h>
static int same(harmony_owner_id a, harmony_owner_id b)
{ return a.source == b.source && a.channel == b.channel && a.trigger == b.trigger; }
static int contains(const uint8_t *p, unsigned n, uint8_t pitch)
{ unsigned i; for (i = 0; i < n; ++i) if (p[i] == pitch) return 1; return 0; }
void harmony_owners_init(harmony_owners *s) { memset(s, 0, sizeof(*s)); }
void harmony_owners_clear(harmony_owners *s)
{ uint32_t rejected = s->rejected; harmony_owners_init(s); s->rejected = rejected; }
int harmony_owner_replace(harmony_owners *s, harmony_owner_id id,
                          const uint8_t *p, unsigned n, harmony_delta *d)
{
    unsigned i, j, slot = HARMONY_OWNER_CAP, free_slot = HARMONY_OWNER_CAP;
    uint8_t next[4], nn = 0;
    harmony_owner *o;
    memset(d, 0, sizeof(*d));
    if (n > 4 || (n && !p) || id.source > HARMONY_SEQ || id.channel > 15 || id.trigger > 127)
        goto reject;
    for (i = 0; i < n; ++i) {
        if (p[i] > 127) goto reject;
        if (!contains(next, nn, p[i])) next[nn++] = p[i];
    }
    for (i = 0; i < HARMONY_OWNER_CAP; ++i) {
        if (s->owners[i].active && same(s->owners[i].id, id)) slot = i;
        if (!s->owners[i].active && free_slot == HARMONY_OWNER_CAP) free_slot = i;
    }
    if (slot == HARMONY_OWNER_CAP) {
        if (!nn) return 1; /* unknown release is harmless, even when full */
        slot = free_slot;
        if (slot == HARMONY_OWNER_CAP) goto reject;
    }
    o = &s->owners[slot];
    /* Compute net membership before committing: no transient off for common tones. */
    for (i = 0; i < 128; ++i) {
        unsigned before = s->membership[i];
        unsigned after = before + contains(next, nn, (uint8_t)i)
            - (o->active && contains(o->pitches, o->count, (uint8_t)i));
        if (before && !after) d->off[d->off_count++] = (uint8_t)i;
        if (!before && after) d->on[d->on_count++] = (uint8_t)i;
        s->membership[i] = (uint8_t)after;
    }
    o->id = id; o->active = nn != 0; o->count = nn;
    for (j = 0; j < nn; ++j) o->pitches[j] = next[j];
    return 1;
reject:
    if (s->rejected != UINT32_MAX) ++s->rejected;
    return 0;
}
void harmony_owner_release(harmony_owners *s, harmony_owner_id id, harmony_delta *d)
{ (void)harmony_owner_replace(s, id, 0, 0, d); }
