/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint8_t reply[600]; /* matches production reply capacity */
static uint32_t reply_n;
static void ed_b(uint32_t v) { assert(reply_n < sizeof reply); reply[reply_n++] = (uint8_t)(v & 127u); }
static void ed_str(const char *s, uint32_t max) { uint32_t i; for (i = 0; s[i] && i < max; i++) ed_b((uint8_t)s[i]); ed_b(0); }
#include "../firmware/src/editor_drum_grooves.c"
static void command(const uint8_t *a, uint32_t n, uint32_t status)
{
    reply_n = 0; assert(ed_drum_grooves_handle(74, a, n));
    assert(reply_n >= 2 && reply[0] == (n ? a[0] : 127) && reply[1] == status);
}
int main(void)
{
    static track_t before, synth;
    uint8_t cap[] = {0}, list[] = {1}, apply[] = {2, 0, 0};
    uint32_t i;
    panel = PANEL_DEFAULT; host_tracks_init(); song.sel = 0;
    before = *TDRUM; synth = trk[0]; groove_sel = 3; drum_page = 1;
    command(cap, 1, 0); assert(reply_n == 5 && reply[2] == 1 && reply[3] == NDRUM_GROOVES && reply[4] == 1);
    command(list, 1, 0); assert(reply[2] == NDRUM_GROOVES);
    { uint32_t pos = 3; for (i = 0; i < NDRUM_GROOVES; i++) { assert(reply[pos++] == i); assert(reply[pos++] == DRUM_GROOVES[i].len); assert(reply[pos++] == DRUM_GROOVES[i].div); assert(!strcmp((char *)&reply[pos], DRUM_GROOVES[i].name)); pos += strlen((char *)&reply[pos]) + 1; } assert(pos == reply_n); }
    check(!memcmp(&before, TDRUM, sizeof before) && !memcmp(&synth, &trk[0], sizeof synth) && groove_sel == 3 && drum_page == 1, "phone queries preserve pattern, selection and hardware browser");
    command(NULL, 0, 1); command(cap, 0, 1);
    { uint8_t bad[] = {0, 0, 0, 0}; command(bad, 2, 1); bad[0] = 3; command(bad, 1, 1); }
    { uint8_t bad[] = {1, 0, 0}; command(bad, 2, 1); command(bad, 3, 1); }
    command(apply, 1, 1); command(apply, 2, 1);
    { uint8_t bad[] = {2, 0, 0, 0}; command(bad, 4, 1); }
    apply[1] = NDRUM_GROOVES; command(apply, 3, 1); assert(reply[2] == NDRUM_GROOVES);
    apply[1] = 0; apply[2] = 2; command(apply, 3, 1); apply[2] = 0;
    check(!memcmp(&before, TDRUM, sizeof before) && !undo.valid, "malformed phone commands have no mutations");
    TDRUM->micro[63] = 11; before = *TDRUM;
    command(apply, 3, 3); assert(reply_n == 3 && !reply[2]);
    check(!memcmp(&before, TDRUM, sizeof before), "metadata-only pattern requires explicit phone confirmation");
    apply[2] = 1;
    song.playing = 1; command(apply, 3, 2); song.playing = 0;
    transport_req = 1; command(apply, 3, 2); transport_req = 0;
    rec_wait = 1; command(apply, 3, 2); rec_wait = 0;
    song.rec = 1; command(apply, 3, 2); song.rec = 0;
    ft_on = 1; command(apply, 3, 2); ft_on = 0;
    check(!memcmp(&before, TDRUM, sizeof before), "busy phone apply never changes pattern");
    groove_confirm = 1; command(apply, 3, 0);
    check(song.sel == 0 && !memcmp(&synth, &trk[0], sizeof synth) && !groove_confirm && groove_sel == 3 && drum_page == 1 && sync_reload && ui.force, "phone apply targets drums while synth selected and refreshes UI");
    assert(undo_swap(0));
    check(!memcmp(before.dstep, TDRUM->dstep, sizeof before.dstep) && !memcmp(before.micro, TDRUM->micro, sizeof before.micro), "phone replacement shares complete hardware undo");
    assert(undo_swap(1));
    for (i = 0; i < NSTEP; i++) { dstep_t st = drum_groove_step(0, i); assert(!memcmp(&st, &TDRUM->dstep[i], sizeof st)); }
    reply_n = 0; assert(!ed_drum_grooves_handle(75, cap, 1) && !reply_n);
    printf("editor drum grooves: %s\n", fails ? "FAIL" : "PASS"); return fails;
}
