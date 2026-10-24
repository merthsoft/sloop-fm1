# Upstream legato and independent note ties

## Implementation status

Implemented in the selected 2.8 integration: partial held-note recording,
bit-7 legato playback and safe STEP/preview/web/Android transport. Independent
continuations remain complementary. See [integration completion](UPSTREAM-2.8-INTEGRATION.md).
The source review and acceptance plan below describe the pre-integration state.
FREE TAKE and MIDI-file duration authoring are still follow-on work.

## Decision

Keep independent note continuations and add upstream legato recording as a
complement. `SF_LEG` cannot replace independent ties without losing the
guitar-style sustain across later attacks that prompted this feature.

The representations are already aligned: MerthSloop `SF_TIE(k)` is an alias for
upstream `SF_CONT(k)`, using bits 3–6 of the same ten-byte `step_t`. Upstream
itself retains these bits alongside `SF_LEG` in bit 7. There is no competing
per-note storage format to migrate or duplicate.

This is a source review against local upstream `main` at `b77bf55` and the
MerthSloop working tree with the review integration. No recording/playback,
protocol or persistence changes are implemented by this document, and no new
target size or performance measurements are claimed.

## What each feature means

| Mechanism | Meaning | Appropriate use |
| --- | --- | --- |
| `ST_TIE` | Continue the preceding sounding set as a whole | A held chord across otherwise empty cells |
| `SF_CONT(k)` / `SF_TIE(k)` | This pitch continues from the preceding step without a new attack; other pitches in this cell may attack or end | Fingerpicked guitar, overlapping polyphonic notes, independent note lengths |
| `SF_LEG` | A new step was played while the previous note was still physically held; hand the voice to the new pitch with the voice mode's legato behavior | MONO/LEGATO/UNISON recording, envelope continuity between successive pitches |
| `SF_SLIDE` | Explicit slide/glide behavior into the following step | Acid-style pitch slides; distinct from legato without a requested glide |

For example, to preserve a fingerpicked guitar figure in POLY:

| Step | Stored pitches | Incoming continuation flags | Audible result |
| --- | --- | --- | --- |
| 1 | C | none | Attack C |
| 2 | C, E | C | C sustains; attack E |
| 3 | C, E, G | C, E | C/E sustain; attack G |
| 4 | E, G | E, G | C ends; E/G continue |

One step-wide legato bit cannot identify which of C/E/G stays alive or which
note gets a fresh attack. Replacing continuations with it would lose that
information. Conversely, a mono C → E handoff is not a same-pitch C
continuation: it needs the new legato marker. Neither is a substitute for the
other.

## Source evidence

Upstream references below identify the reviewed commit, rather than a moving
branch. MerthSloop file references describe the working tree at review time.

- Upstream [`core.h`, lines 186–199](https://github.com/isod89/sloop-fm1/blob/b77bf55/firmware/src/core.h#L186-L199)
  defines both `SF_CONT0 = 8` / four continuation bits and `SF_LEG = 0x80`.
  Its comment explicitly associates the latter with MONO/LEGATO/UNISON parts.
- Upstream [`seq.c`, lines 655–741](https://github.com/isod89/sloop-fm1/blob/b77bf55/firmware/src/seq.c#L655-L741)
  records overlapping POLY notes with continuation bits, and separately sets
  `SF_LEG` for a held, non-ratcheted, non-POLY transition to another pitch.
- Upstream [`seq.c`, lines 755–834](https://github.com/isod89/sloop-fm1/blob/b77bf55/firmware/src/seq.c#L755-L834)
  handles partial releases and moving continuation end markers; a chord does
  not simply remain held until its final key is released.
- Upstream [`seq.c`, lines 2354–2433](https://github.com/isod89/sloop-fm1/blob/b77bf55/firmware/src/seq.c#L2354-L2433)
  recognizes continuation/legato look-ahead and retains a separate
  continuation reconciliation path. Legato alone does not force glide;
  explicit slide and normal voice/glide settings remain meaningful.
- MerthSloop [core definitions](../../firmware/src/core.h),
  [note-chain helpers](../../firmware/src/note_ties.c), and
  [sequencer](../../firmware/src/seq.c) already use the same continuation bits.
  Our `seq_tie_hold`/`seq_gate_release` additionally retain only the pitches
  needed by upcoming continuations at the ordinary gate boundary.

## Recording gaps worth integrating

MerthSloop already has upstream's earlier ability to overdub onto a whole-step
TIE by materializing a mixed attack/continuation cell. It does not yet have
the later complete held-note ownership and `SF_LEG` work in the reviewed main.
In particular, current `rec_release` keeps the recorded hold alive while any
of its notes remain down; upstream's later `hold_restart`, `rh_held` and
`hold_marker_clear` paths distinguish notes released from those still held.
Upstream also covers an onset landing in the next grid cell before that cell
has become a TIE.

Port those semantics into our recording path instead of replacing the
sequencer wholesale. Preserve these MerthSloop behaviors:

- Independent gate protection and per-note attack velocity, ratchets on fresh
  attacks only, orphan continuation behavior, and balanced MIDI ownership.
- Separate recording snap, `rh_start_abs` ordering, swing-aware
  `musical_units(REC_LAT, ...)`, and snapped-onset catch-up through past cells.
  Do not copy upstream latency arithmetic without adapting our musical clock.
- Live-edit invalidation (`seq_edit_changed`) and recording skip ownership;
  the recent correctness fix prevents edits from ratcheting an untracked pitch.
- Our direct versus SNOTE harmony routing and live progression output. The
  latter are distinct from envelope legato and require explicit coverage.
- Native selective length/chain operations, undo, move/merge behavior and
  display, rather than replacing the editor with upstream's whole-step model.

The FREE TAKE close path currently creates whole-step ties up to the next
onset. Improving ordinary overdub does not by itself fix FREE TAKE or MIDI
import duration authoring. Keep those as explicit follow-on scope unless their
event-to-step conversion is included and tested in the same implementation.

## Storage and protocol consequences

**Project storage:** bit 7 fits in the existing byte; `step_t` stays ten bytes.
`project_capture` and `project_apply` copy complete step arrays, and historical
step conversion copies `flags`. No new song partition, voice buffer, or larger
project is necessary to retain the marker. An older implementation can ignore
it on playback; current MerthSloop does. Raw archive restoration and semantic
editor round-trips are different operations and must both be checked.

**Seven-bit editor wire:** simply adding `SF_LEG` to a mask is insufficient.
`editor.c:ed_b` emits `v & 0x7f`, and `ed_step_in` accepts only accent, slide and
continuations. Thus the current STEP reply silently drops bit 7, and a STEP
write cannot restore it. Reviewed upstream has this same seven-bit transport
limitation; copying its firmware flag does not solve our editors' round-trip.

Choose and document a capability-negotiated STEP extension carrying the legato
bit in seven-bit-safe data. Keep existing STEP lengths/fields for old clients.
An opaque backup must retain original bytes; an old semantic editor should not
silently rewrite a legato step and claim faithful preservation. A protocol
extension should specify omitted-extension behavior and reject unsupported
lossy edits or preserve their existing marker deliberately.

Audit the entire path, including:

- `web/editor.html`: STEP writers/mock currently mask `0x7B`; editors that
  reconstruct flags from accent/slide controls can lose continuations too.
- `firmware/host/preview.c`: input sanitization currently masks off bit 7.
- `firmware/src/scene_transaction.c`: scene records validate `0x7B` and have
  a separate schema, so a STEP extension does not automatically fix scenes.
- Android `StepFlags`, `HardwareSteps`, scene validation and archive validation:
  currently model/accept only `0x7B`. Extend model, codec and validation together;
  schema-version any serialized shape that actually changes.
- Sorting/deletion/length/transpose, merge, undo, import and progression
  generation: retain or intentionally recompute step-level legato independently
  from the four slot-level continuation flags. Define mixed flag precedence;
  upstream checks continuations first, and that is the sensible starting point.

Do not relocate `SF_LEG` into reserved bit 2 just to fit the old wire: it would
break upstream byte compatibility and conflict with the historical preset tie
marker. Use the upstream persisted bit and solve transport explicitly.

## Next implementation and acceptance scope

1. Port partial held-note recording and ownership helpers while retaining our
   existing continuation playback and snap/clock corrections.
2. Add `SF_LEG` recording/playback for non-POLY handoffs, without importing
   upstream's boolean hold handling over our per-pitch gate masks. Explicitly
   distinguish envelope continuity, ordinary glide and explicit SLIDE.
3. Add the negotiated wire representation and full firmware/preview/web/Android
   round-trip support before advertising end-to-end editable legato projects.
4. Exercise live recording, then separately decide whether FREE TAKE and MIDI
   import duration authoring ship in this batch or remain tracked follow-ons.
5. Measure linked flash/static RAM and run existing tie, recording, voice/MIDI,
   native UI and preview parity tests before packaging.

New regressions should earn their keep: record the C/E/G figure above with
staggered releases; record a fresh attack of a previously sustained pitch;
compare MONO/LEGATO/UNISON and POLY; distinguish legato with GLIDE off from
explicit SLIDE; test early/late quantized onsets, eighth/quarter recording snap,
loop wrap, capacity, disarm/STOP and editor changes while recording. Assert
voice/envelope identity and MIDI event balance, not only stored flags. Verify
save/restart/load and untouched STEP/scene/web/Android round-trips of bit 7.

The first controls batch (device settings, drum/editor preferences, resonance
and punch fade) does not depend on this recording redesign. The planned HOME
section badge is independent too.
