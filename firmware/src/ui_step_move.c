/* SPDX-License-Identifier: GPL-3.0-only */
/* Move a selected synth onset and its following ties without destroying neighbors. */
static void step_octave(int32_t amount)
{
    track_t *t = TSEL;
    uint32_t first = ui.cursor, i, low = 127, high = 0;
    int32_t delta;
    if (song.rec || rec_wait || ft_on) { ui_message("STOP RECORDING"); return; }
    while (first && t->step[first].time == ST_TIE) first--;
    step_t *s = &t->step[first];
    if (s->time != ST_NOTE || !s->n) { ui_message("SELECT A NOTE"); return; }
    for (i = 0; i < s->n; i++) {
        if (s->note[i] < low) low = s->note[i];
        if (s->note[i] > high) high = s->note[i];
    }
    delta = clamp(amount, -(int32_t)(low / 12u), (int32_t)((127u - high) / 12u)) * 12;
    if (!delta) { ui_message("OCTAVE LIMIT"); return; }
    fm1_irq_off();
    undo_mark(t, ui.tie_sess);
    for (i = 0; i < s->n; i++) s->note[i] = (uint8_t)(s->note[i] + delta);
    last_note = s->note[0];
    fm1_irq_on();
    sync_reload = 1; ui.force = 1;
}

static void step_move(int32_t delta)
{
    track_t *t = TSEL;
    uint32_t first = ui.cursor, end, len = trk_len(t), i;
    int32_t dest;
    if (song.playing || transport_req || song.rec || rec_wait || ft_on) {
        ui_message("STOP FIRST"); return;
    }
    while (first && t->step[first].time == ST_TIE) first--;
    if (t->step[first].time != ST_NOTE || !t->step[first].n) {
        ui_message("SELECT A NOTE"); return;
    }
    for (end = first + 1u; end < len && t->step[end].time == ST_TIE; end++) {}
    if (delta > 0 && (uint32_t)delta < end - first) {
        /* Move the onset into its own sustain without extending the old end. */
        dest = (int32_t)first + delta;
        fm1_irq_off();
        undo_mark(t, ui.tie_sess);
        if (!groove_undo_matches()) starter_undo_capture(t);
        t->step[dest] = t->step[first];
        t->micro[dest] = t->micro[first];
        step_fill_set(t, (uint32_t)dest, step_fill(t, first));
        /* Onset locks take precedence over same-parameter locks at its target. */
        for (i = 0; i < NLOCK; i++) if (t->lock[i].step == (uint32_t)dest)
            for (uint32_t j = 0; j < NLOCK; j++)
                if (t->lock[j].step == first && t->lock[j].param == t->lock[i].param)
                    t->lock[i].step = LOCK_FREE;
        for (i = 0; i < NLOCK; i++) {
            if (t->lock[i].step == first) t->lock[i].step = (uint8_t)dest;
            else if (t->lock[i].step > first && t->lock[i].step < (uint32_t)dest)
                t->lock[i].step = LOCK_FREE;
        }
        for (i = first; i < (uint32_t)dest; i++) {
            step_clear(&t->step[i]); t->micro[i] = 0; step_fill_set(t, i, 0);
        }
        fm1_irq_on();
        cursor_set(clamp((int32_t)ui.cursor + delta, dest, (int32_t)end - 1));
        sync_reload = 1; ui.force = 1;
        return;
    }
    dest = clamp((int32_t)first + delta, 0, (int32_t)(len - (end - first)));
    delta = dest - (int32_t)first;
    if (!delta) return;
    for (i = (uint32_t)dest; i < (uint32_t)dest + end - first; i++) {
        const step_t *s = &t->step[i];
        if (i >= first && i < end) continue;
        if (s->time != ST_REST || s->n || s->flags || s->vel || s->lvl || s->rat ||
            t->micro[i] || step_fill(t, i) || step_locked(t, i)) {
            ui_message("STEP OCCUPIED"); return;
        }
    }
    fm1_irq_off();
    undo_mark(t, ui.tie_sess);
    if (!groove_undo_matches()) starter_undo_capture(t);
    /* Move in memmove order so overlapping tie chains need no pattern buffer. */
    for (i = 0; i < end - first; i++) {
        uint32_t src = delta > 0 ? end - 1u - i : first + i;
        uint32_t dst = (uint32_t)((int32_t)src + delta);
        t->step[dst] = t->step[src];
        t->micro[dst] = t->micro[src];
        step_fill_set(t, dst, step_fill(t, src));
        step_clear(&t->step[src]);
        t->micro[src] = 0; step_fill_set(t, src, 0);
    }
    for (i = 0; i < NLOCK; i++)
        if (t->lock[i].step >= first && t->lock[i].step < end)
            t->lock[i].step = (uint8_t)((int32_t)t->lock[i].step + delta);
    fm1_irq_on();
    cursor_set((int32_t)ui.cursor + delta);
    sync_reload = 1; ui.force = 1;
}
