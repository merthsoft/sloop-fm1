/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Explicit project/section saves and the deferred live queue. Included after
 * project serialization; isolated so host tests exercise the real flash path. */
/* Explicit saves bypass the background autosave's idle/rate limits, but only
 * once stopped. Keep the same upstream startup object and project format. */
UI_SMALL static int autosave_commit(const project_t *p)
{
#if FELUCCA_FLASH
    if (p->sum!=autosave_hash && st_save(OBJ_AUTOSAVE, p, sizeof *p)) return -1;
    autosave_hash = p->sum;
    autosave_ms = fm1_ms;
    settings_save();
    if(snapshot_pending())return -1;
#else
    (void)p;
#endif
    return 0;
}

UI_SMALL static void project_save(uint32_t slot)
{
    project_t *p = &proj_slot[slot & 3u];
#if FELUCCA_ARRANGER
    if (song.playing || transport_req) { ui_message("STOP BEFORE SAVE"); return; }
#endif
    proj_capture(p);
    snapshot_capture(&snapshot_sections[slot&3u],p);
#if FELUCCA_FLASH
    if (flash_ok) {
        if (st_save(OBJ_PROJECT0 + (slot & 3u), p, sizeof *p) || autosave_commit(p)) {
            sec_dirty |= (uint8_t)(1u << (slot & 3u));
            song_dirty = 1;
            ui_message("SAVE ERROR: RETRYING");
        } else {
            sec_dirty &= (uint8_t)~(1u << (slot & 3u));
            settings_save();
            if(settings_later) ui_message("SETTINGS SAVE RETRYING");
            else ui_say("SAVED SECTION ", SEC_NAME[slot & 3u]);
        }
        return;
    }
#endif
    ui_message("SAVED (RAM)");
}

#if FELUCCA_ARRANGER
UI_SMALL static void section_store(uint32_t s)
{
    s &= 3u;
    if (!song.playing && !transport_req) {
        project_save(s);                             /* explicit stopped save is durable now */
        live_sec = (int8_t)s;
        return;
    }
    fm1_irq_off();                                      /* (the audio ISR may be applying a section) */
    proj_capture(&proj_slot[s]);
    snapshot_capture(&snapshot_sections[s],&proj_slot[s]);
    live_sec = (int8_t)s;
    fm1_irq_on();
    sec_dirty |= (uint8_t)(1u << s);
    song_dirty = 1;                                  /* include device-local swing preference */
    ui_message("QUEUED: STOP TO SAVE");
}

UI_SMALL static void sections_write(void)                        /* the dirty sections and song into flash */
{
    uint32_t i;
#if FELUCCA_FLASH
    uint8_t pending = sec_dirty;
    if (flash_ok)
        for (i = 0; i < 4u; i++)
            if (((sec_dirty >> i) & 1u) && st_save(OBJ_PROJECT0 + i, &proj_slot[i], sizeof proj_slot[i]) == 0)
                sec_dirty &= (uint8_t)~(1u << i);       /* (a failed write stays dirty: tried again later) */
    /* Resume the working loop at stop, including edits made after the queued
     * section snapshot. Failed startup writes retain the queue for retry. */
    if (flash_ok && pending && !sec_dirty && !arrangement_clock.running) {
        proj_capture(&autosave_buf);
        if (autosave_commit(&autosave_buf)) sec_dirty = pending;
    }
    if (!flash_ok)
#endif
        sec_dirty = 0;
    (void)i;
    if (song_dirty || fav_dirty || settings_later) {                      /* (2.7: the favourites too) */
        song_dirty = 0;
        settings_save();
    }
}

#endif /* FELUCCA_ARRANGER */
