/* SPDX-License-Identifier: GPL-3.0-only */
/* Move a selected synth onset and its following ties without destroying neighbors. */
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
