/* SPDX-License-Identifier: GPL-3.0-only */
/* Move a selected synth onset and its following ties without destroying neighbors. */
UI_SMALL static void step_octave(int32_t amount)
{
    track_t *t = TSEL;
    uint32_t first = ui.cursor, i, low = 127, high = 0;
    int32_t delta;
    if (note_pattern_linked(t)) {ui_message("USE EDIT TRANSPOSE");return;}
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
    seq_edit_changed(t);
    fm1_irq_on();
    sync_reload = 1; ui.force = 1;
}

UI_SMALL static void step_move(int32_t delta)
{
    track_t *t = TSEL;
    uint32_t first = ui.cursor, end, len = trk_len(t), i;
    int32_t dest;
    if (note_pattern_linked(t)) {ui_message("USE NOTE ACTIONS");return;}
    if (song.rec || rec_wait || ft_on) { ui_message("STOP RECORDING"); return; }
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
        dest=clamp((int32_t)ui.cursor+delta,dest,(int32_t)end-1);
        goto moved;
    }
    dest = clamp((int32_t)first + delta, 0, (int32_t)(len - (end - first)));
    delta = dest - (int32_t)first;
    if (!delta) return;
    if (t->step[dest].time == ST_NOTE) {
        uint32_t stop = (uint32_t)dest + 1u, j;
        while (stop < len && t->step[stop].time == ST_TIE) stop++;
        if (end-first != 1u && end-first != stop-(uint32_t)dest) {
            ui_message("TIE LENGTH DIFFERS"); return;
        }
        for (i=0;i<NLOCK;i++) if (t->lock[i].step>=first && t->lock[i].step<end) {
            ui_message("STEP HAS LOCKS"); return;
        }
        step_t merged=t->step[dest];
        for (i=0;i<t->step[first].n;i++) {
            uint8_t note=t->step[first].note[i];
            for (j=0;j<merged.n && merged.note[j]!=note;j++) {}
            if (j<merged.n) continue;
            if (merged.n==4u) { ui_message("CHORD FULL"); return; }
            merged.note[merged.n++]=note;
        }
        fm1_irq_off();
        undo_mark(t,ui.tie_sess);
        if (!groove_undo_matches()) starter_undo_capture(t);
        t->step[dest]=merged;
        for(i=first;i<end;i++) {step_clear(&t->step[i]);t->micro[i]=0;step_fill_set(t,i,0);}
        goto moved;
    }
    for (i = (uint32_t)dest; i < (uint32_t)dest + end - first; i++) {
        const step_t *s = &t->step[i];
        if (i >= first && i < end) continue;
        /* A rest may retain its former pitch/velocity for later editing.
         * Those fields, timing and conditions are inert without a note.
         * Parameter locks still act on empty steps and must be protected. */
        if (s->time != ST_REST) {
            ui_message("STEP OCCUPIED"); return;
        }
        for (uint32_t k=0;k<NLOCK;k++) if (t->lock[k].step==i) {
            ui_message("STEP HAS LOCKS"); return;
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
    dest=(int32_t)ui.cursor+delta;
moved:
    seq_edit_changed(t);
    fm1_irq_on();
    cursor_set(dest);
    sync_reload = 1; ui.force = 1;
}
