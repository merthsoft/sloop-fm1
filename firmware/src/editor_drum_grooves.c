/* SPDX-License-Identifier: GPL-3.0-only */
/* Command 74, schema 1. Included by editor.c after reply helpers.
 * Requests: [0]; [1]; [2,id,overwrite(0/1)].
 * Replies: [0,status,1,bank_count,1(capability flag: apply with undo)];
 * [1,status,count, (id,len,div,name ASCII NUL)*]; [2,status,id].
 * Malformed apply replies retain ID if supplied, otherwise 127.
 * Errors omit trailing capabilities/list data. Bank <=24, name <=20.
 * Status: 0 OK, 1 malformed, 2 busy, 3 overwrite confirmation needed.
 * IDs follow the immutable firmware ROM bank; browsing is read-only. */
#define ED_DRUM_GROOVES 74u
static int ed_drum_grooves_handle(uint32_t cmd, const uint8_t *a, uint32_t na)
{
    uint32_t op = na ? a[0] : 127u, id = na > 1 ? a[1] : 127u, i;
    if (cmd != ED_DRUM_GROOVES) return 0;
    ed_b(op);
    if (!na || op > 2 || (!op && na != 1) ||
        (op == 1 && na != 1) ||
        (op == 2 && (na != 3 || id >= NDRUM_GROOVES || a[2] > 1))) {
        ed_b(1); if (op == 2) ed_b(id); return 1;
    }
    if (!op) {
        ed_b(0); ed_b(1); ed_b(NDRUM_GROOVES); ed_b(1); return 1;
    }
    if (op == 1) {
        ed_b(0); ed_b(NDRUM_GROOVES);
        for (i = 0; i < NDRUM_GROOVES; i++) {
            ed_b(i); ed_b(DRUM_GROOVES[i].len); ed_b(DRUM_GROOVES[i].div);
            ed_str(DRUM_GROOVES[i].name, 20);
        }
        return 1;
    }
    if (song.playing || transport_req == 1 || song.rec || rec_wait || ft_on) {
        ed_b(2); ed_b(id); return 1;
    }
    if (!a[2] && drum_groove_has_content()) { ed_b(3); ed_b(id); return 1; }
    if (!drum_groove_apply(id)) { ed_b(2); ed_b(id); return 1; }
    groove_confirm = 0; groove_cursor = 0; drum_cursor = 0;
    ui.force = 1;
    ed_b(0); ed_b(id);
    return 1;
}
