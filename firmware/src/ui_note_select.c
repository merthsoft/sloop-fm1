/* SPDX-License-Identifier: GPL-3.0-only */
/* NOTES selection model, shared by the production UI and host regressions.
 * Selection is an attack's step and pitch, never a retained chord slot.
 * A zero pitch represents an empty cell. No continuation is a second attack. */
typedef struct { uint8_t step, pitch; } piano_selection_t;

UI_SMALL static int piano_selection_slot(const track_t *t, piano_selection_t s)
{
    if (s.step >= trk_len(t) || !s.pitch) return -1;
    const step_t *st = &t->step[s.step];
    uint32_t k = note_slot(st, s.pitch);
    return st->time == ST_NOTE && k < st->n && !(st->flags & SF_TIE(k)) ? (int)k : -1;
}

/* Return each playable attack in stored order; genuinely empty cells get one
 * navigation stop. Mixed cells contribute only their new attacks. */
UI_SMALL static uint32_t piano_selection_choices(const track_t *t, uint32_t at, uint8_t pitch[4])
{
    step_t st;
    note_effective(t, at, &st);
    uint32_t n = 0;
    for (uint32_t k = 0; k < st.n; k++)
        if (!(st.flags & SF_TIE(k))) pitch[n++] = st.note[k];
    if (!st.n) { pitch[0] = 0; n = 1; }
    return n;
}

UI_SMALL static piano_selection_t piano_selection_at(const track_t *t, uint32_t at)
{
    at = (uint32_t)clamp((int32_t)at, 0, trk_len(t)-1);
    step_t st;
    note_effective(t, at, &st);
    for (uint32_t k = 0; k < st.n; k++)
        if (!(st.flags & SF_TIE(k))) return (piano_selection_t){(uint8_t)at, st.note[k]};
    if (st.n) return (piano_selection_t){(uint8_t)note_origin(t, at, st.note[0]), st.note[0]};
    return (piano_selection_t){(uint8_t)at, 0};
}

/* Bounded two-pass walk, at most 64*4 choices, no pattern staging buffer.
 * Invalidated selections are safe navigation anchors, never editing targets. */
UI_SMALL static piano_selection_t piano_selection_walk(const track_t *t, piano_selection_t s, int32_t delta)
{
    uint32_t count = 0, rank = 0, found = 0;
    uint8_t pitch[4];
    for (uint32_t at = 0; at < trk_len(t); at++) {
        uint32_t n = piano_selection_choices(t, at, pitch);
        for (uint32_t k = 0; k < n; k++, count++) {
            if (!found && at < s.step) rank = count + 1;
            if (at == s.step && pitch[k] == s.pitch) { rank = count; found = 1; }
        }
    }
    /* A malformed leading continuation can contain no attack at all. */
    if (!count) return (piano_selection_t){0, 0};
    int32_t target = clamp((int32_t)rank + clamp(delta, -256, 256), 0, count-1);
    for (uint32_t at = 0; at < trk_len(t); at++) {
        uint32_t n = piano_selection_choices(t, at, pitch);
        if (target < (int32_t)n) return (piano_selection_t){(uint8_t)at, pitch[target]};
        target -= (int32_t)n;
    }
    return (piano_selection_t){0, 0};
}
