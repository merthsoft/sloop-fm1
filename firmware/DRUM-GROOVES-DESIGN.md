# Hardware drum starters

The drum track's existing DRUMS screen has GRID, KIT and GROOVE views. Tap EDIT or SEQ to cycle views, or turn SELECT. GROOVE is a read-only browser until OCT+ applies it. OCT- toggles a looping beat preview while stopped; when replacement confirmation is open it cancels and returns to GRID. A nonempty pattern (including invisible timing, conditions or locks) requires a second OCT+ press to confirm replacement. Changing the selected starter or leaving its view cancels confirmation. PLAY still controls transport; applying while playing, starting, recording or armed displays STOP FIRST.

The four knobs select starter, preview step, preview sound and audition the selected hit. Audition uses the selected kit and the starter's hit level; an empty preview cell is silent. The screen shows name, length, division, five canonical instrument rows, selected cell and explicit OCT button hints. GRID and KIT keep their existing four-knob mappings. Existing held layers remain available.

`drum_grooves.c`, included by `ui.c`, contains sixteen immutable ROM mask templates: FOUR FLOOR, HOUSE OFFBEAT, BACKBEAT, HALF TIME, BREAKBEAT, SHUFFLE, SMALL FILL, TECHNO DRIVE, DISCO, HIP HOP, DNB TWO STEP, UK GARAGE, REGGAETON, BOSSA, AMEN BREAK and AMEN HALF. Canonical lanes are kick 0, snare 2, clap 3, closed hat 4 and open hat 5. Most straight starters have sixteen sixteenth-note steps; AMEN BREAK is a four-bar, 64-step Amen-inspired kit transcription and AMEN HALF is its two-bar, 32-step opening phrase. These are editable kit hits, not sampled audio. The preview grid follows the selected step in 16-step windows; SHUFFLE has twelve eighth-note triplet steps (four beats). Accents and quiet snare ghosts are explicit; template steps have no ratchets. SMALL FILL is a repeating starter, not an automatic one-shot fill.

Apply replaces all 64 ordinary drum steps, clears per-step nudge, conditions and all locks, sets LEN and DIV, and sets local SWG to zero so template timing is predictable. Global swing stays selected. Kit, drum level/reverb/pan, tempo, key/scale and every synth track remain unchanged. There are no parameter IDs or new persisted bytes. Saved projects contain ordinary fully editable drum steps.

The existing undo holds steps and LEN. A 188-byte supplemental snapshot keeps micro timing, locks, conditions, DIV, local SWG and sequence-active state for a groove replacement. It is associated with the exact existing undo session and drum track; another undo mark supersedes it automatically. `undo_swap()` exchanges both snapshots under the existing IRQ guard. Groove undo/redo also requires stopped transport. This does not change the preexisting metadata limitations of ordinary step-edit undo.

Lifecycle integration: `proj_apply` invalidates `undo.valid` and the UI project adoption hook clears `groove_confirm`. This also invalidates the supplemental snapshot and prevents stale undo across project loads. Browsing selection itself is process-only state. No `seq.c`, `core.h`, persisted format, version, build script or editor changes are required.

Verification: `tests/drum_grooves_test.c` uses the real UI harness and project serializer. It checks template bounds, accent and known kick/snare positions, browsing/cancel/running refusal, record arm and pending start refusal, replacement cleanup, full undo/redo restoration, synth/sound preservation, ordinary project roundtrip and framebuffer bounds for every starter. `tests/run_drum_grooves.sh` runs it with a host compiler. The existing `ui_pages_test.c` suite checks retained drum gestures and 20,000 frames of randomized UI use.

## Phone command 74, schema 1

`editor_drum_grooves.c` exposes the same ROM bank and `drum_groove_apply`, independently of selected track. Parent integration includes it after editor reply helpers and calls `ed_drum_grooves_handle(cmd, a, na)` before ordinary dispatch. It returns 1 if command 74 was handled, including malformed requests; ordinary editor framing/send remains in `editor.c`.

All bytes are MIDI-safe 7-bit bytes. Replies begin operation/status. Status is OK/applied 0, malformed 1, busy 2, confirmation-required 3.

| Operation | Request payload | Successful response payload |
| --- | --- | --- |
| Capabilities | `[0]` | `[0,0,1,16,1]`: schema, bank count, apply/undo capability flag |
| List | `[1]` | `[1,0,16, entries...]`; each entry is `[id,length,division,name ASCII NUL]` |
| Apply | `[2,id,confirm]` | `[2,0,id]` |

Bank IDs are 0..15 in the ROM array order, names bounded to 20 characters, bank bounded to 16 entries. Division uses the existing `N_SDIV` enum. Apply requires exactly three bytes and confirmation 0 or 1. Any existing hits or step metadata require explicit flag 1. Refusals echo `[2,status,id]`, using ID 127 when omitted; malformed queries reply `[op,1]`. No success is returned before actual apply. Queries change no track, browsing selection or physical page. Successful apply clears pending hardware confirmation and preview cursor and forces UI/editor refresh; it retains physical page and browsed starter. No tempo, kit, mixer or synth changes occur. `tests/editor_drum_grooves_test.c` uses the actual UI/engine harness to verify exact replies, malformed requests, nonmutating queries, busy/confirmation guards, applying from synth selection and shared undo/redo. The standalone runner includes both suites.

## Non-destructive beat preview

OCT- starts/stops a looping preview at the current global tempo with the selected kit.
A separate bounded audio-block clock and copied ROM masks trigger drum hits; no track
steps, metadata, transport, recording, project bytes or undo state are replaced.
Preview is stopped-only and stops on apply/confirmation, bank selection, page exit, HOME,
menu, track switch, STOP, playback/recording, panic and project adoption. Existing drum
tails may decay after stopping. Preview ignores swing so the template spacing is explicit.
Seven 64-bit masks plus clock/state occupy 72 bytes. It is not a phone command; phone
command 74 continues to discover/apply all sixteen entries without an app update.
Tests cover every bank/list entry, all long-pattern screen windows, fourth-bar hits and
ghosts, complete Amen looping, project preservation and preview cleanup/refusal.
