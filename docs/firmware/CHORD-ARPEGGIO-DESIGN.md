# FM1 shared harmony, chord, and arpeggio design

2026-10-09: Merthsoft.6 extracts the existing nearest-inversion policy into
`firmware/src/harmony_voicing.h`, shared by live chords and the optional native
sequence-starter VLEAD renderer. Starter voicing derives from a fixed first-bar
anchor and does not mutate live history. This is a shared voicing helper, not
implementation of the semantic harmony/follower architecture proposed below.
See [musical starters](SEQUENCE-STARTERS-DESIGN.md).

2026-10-08: **QNT = CHROM** adds literal piano-key chord roots without adding parameters or
changing project layout. Stored QNT values 0–2 retain prior behavior; appended value 3 bypasses
white-key remapping and black-key modifiers. OCT/TRN apply; ROOT does not transpose literal
keys. Fixed shapes retain semitone quality; scale-derived shapes transpose tonic scale quality
to each pressed root. CHR still uses the legacy minor fallback for scale-derived chords.
Web editor exposes the same option. Keyboard/record/release and project round-trip tests cover it.

Status: proposed shared-harmony architecture; incremental modes, a tested general ownership foundation, and a production opt-in sequencer-to-arp slice are implemented.
Date: 2026-10-07.
Scope: FM1 firmware, hardware controls, project storage, MIDI, and a shared semantic contract for editors. Android feature implementation is deferred; future editor snapshots must expose the same stable semantic roots and must not create divergent app-only harmony behavior.

Implemented incremental additions: scale-derived SUS2, ADD9, 6TH, and SHELL chord types; OUTIN, SHUF, and ROOTALT arp modes; ORD always uses press order. Existing stored mode IDs and project layouts remain unchanged. ROOTALT anchors at the lowest voiced pitch, not a semantic chord root. The shared-harmony architecture and implementation phases in this document remain proposed.

Also implemented: OCTAVE and fixed MAJOR, MINOR, DOM7, MAJ7, MIN7, DIM, AUG, HALFDIM, DIM7 qualities; DNUP, UPDNREP, INOUT, WALK, and PULSE arp modes. PULSE tracks up to 64 expanded generated tones, deduplicates pitches, and uses the existing shared voice and four-note recording limits. Its tones start together without strum. The MIDI output ring has room for simultaneous multi-track pulse release/retrigger bursts; the input ring remains unchanged. These additions append enum values without changing persisted parameter or step layouts.

## 1. Outcome

Make a chord a reusable musical source. The keyboard, incoming MIDI, or sequencer supplies harmony; chord playback, arpeggiators, and other synth tracks consume it with independent rhythm, register, and sound.

Example: track 1 sequences Am–F–C–G and plays pads. Track 2 follows the chord root as bass. Track 3 plays a syncopated root–fifth–third–octave phrase. Editing the progression updates all three parts. Recording can preserve the harmonic instructions or capture the resulting notes.

The first deliverable is sequencer-fed arpeggiation with reliable note ownership, explicit timing rules, and consistent MIDI output. Cross-track followers and richer harmonic editing build on that foundation.

## 2. Current firmware and constraints

The relevant implementation is in `src/seq.c`, `src/voice.c`, `src/core.h`, `src/params.c`, `src/ui_layers.c`, `src/project.c`, and `src/upreset.c`.

| Area | Current behavior | Design implication |
| --- | --- | --- |
| Chords | OFF, TRIAD, 7TH, 9TH, SUS4, POWER, SUS2, ADD9, 6TH, SHELL, OCTAVE, and fixed qualities listed above; up to four notes | Preserve the four-note voiced-chord limit initially |
| Live modifiers | Black keys alter third, seventh, suspension, ninth, and inversion | Keep these gestures and represent their result explicitly |
| Voicing | Automatic voice leading plus signed millisecond strum | Add register constraints and unify strum event timing |
| Arp | UP, DOWN, UPDOWN, RANDOM, ORDER, OUTSIDE, SHUFFLE, ROOTALT, DOWNUP, UPDOWN_REPEAT, INSIDE, WALK, PULSE; 1–4 octaves; gate, swing, probability, hold | Retain existing modes as the basic note-list player |
| Arp input | Held notes; local chord expansion feeds this list | Add sequencer and shared-harmony sources |
| MIDI input | Literal pitches enter `input_on`; local keys expand earlier | Chord-root MIDI input needs the same harmonic transform as local keys |
| MIDI output | Local keys always emit; generated notes emit in SEQ output mode | Source and generated output need independent routing |
| Expression | Merthsoft.7 preserves live and sequence attack velocity, including accents, across arp modes and routes | Additional expressive controls remain future work; see [implemented expression](ARP-EXPRESSION.md) |
| Probability | Failed hit does not call `arp_next` | Make phrase advancement on rests a deliberate policy |
| Note lifetime | Arp pitch list deduplicates; physical press count is separate | Shared pitches need source ownership |
| Strum | Internal note starts can be delayed; MIDI starts are emitted by callers immediately | One scheduled event must drive both destinations |
| Sequencer | 64 steps, four synth notes per step; `step_t` is 10 bytes | Keep existing literal-note storage and add optional metadata separately |
| Resources | Three synth parts, one drum track, eight shared sounding voices | Followers share the existing allocator; do not reserve extra audio voices |
| Storage | Versioned project formats; count-mapped common preset parameters | New settings require deliberate migration and capacity checks |

This is a proposed replacement architecture, not a claim that all existing behavior is defective. Legacy modes remain available while new semantics are introduced explicitly.

## 3. Goals and boundaries

Goals:

- Use the same harmony transformation for local keys, MIDI roots, sequencer triggers, and memory slots.
- Let recorded chords drive an arp without sounding a duplicate direct chord unless requested.
- Preserve harmonic identity independently of inversion and voice leading.
- Define chord changes, rests, latch, phase, and transport behavior precisely.
- Make internal audio, generated MIDI, and result recording consume the same scheduled events.
- Keep memory, event queues, and processing bounded in the audio/control path.
- Load existing projects and presets with their previous routing and musical behavior.

Non-goals for the initial release:

- Automatic chord recognition from arbitrary incoming polyphonic MIDI.
- More than four voiced chord tones, additional synth tracks, or a larger audio voice budget.
- Arbitrary follower graphs, generated-note feedback, or drum harmony following.
- A general composition language, Android feature implementation, or sample-accurate scheduling redesign.

## 4. Shared harmony model

Separate harmonic intent from its voiced pitches and from sounding-note ownership.

### Harmonic intent

A bounded chord descriptor contains:

- Root: absolute MIDI pitch or scale degree with octave.
- Construction: diatonic, explicit quality, or literal note set.
- Quality and modifier flags.
- Optional independent bass pitch/offset.
- Voicing settings: inversion, spread, register bounds, and voice-leading policy.
- Input velocity and source identity.

Scale-degree descriptors resolve using the source track's root and scale. Absolute roots stay absolute. Followers consume the resolved harmonic identity; their local scale does not silently reharmonize it.

### Resolved harmony snapshot

Each synth track can publish one immutable snapshot per committed change:

- Valid flag and monotonically changing generation identifier.
- Resolved root, optional bass, and available semantic chord roles.
- Up to four voiced pitches, their roles, and velocities.
- Provenance: source track, input kind, and whether semantic roles are known.

Root and role intervals survive inversion. A rootless four-note voicing may therefore still supply a root to a bass follower.

Literal recorded chords and arbitrary MIDI note sets have unknown semantic roles. They support indexed, lowest, and highest selectors. Do not guess their root or quality. Selecting ROOT on unknown harmony falls back to LOWEST and shows that fallback on the hardware UI.

### Pipeline

```text
local keys / MIDI / sequencer / chord memory
                  |
          source selection and ownership
                  |
       harmonic intent -> resolved snapshot
                  |
       +----------+-----------+
       |                      |
 local chord/arp         follower snapshot
       |                      |
       +---- timed performance events ----+
                                           |
                          audio / MIDI / result recording
```

Snapshots update atomically. A chord is submitted as a batch, so replacing a latched chord or changing a modifier cannot briefly publish an empty or partially assembled chord.

## 5. Source selection and cross-track following

Proposed source choices: LIVE, SEQ, LIVE+SEQ, FOLLOW 1, FOLLOW 2, FOLLOW 3.

- LIVE accepts local keys and routed MIDI. MIDI has a separate LITERAL / CHORD ROOT input setting.
- SEQ publishes each effective chord step after its locks, conditions, and microtiming are applied.
- LIVE+SEQ uses a live override while physical input is held. On final release, return to the latest sequencer snapshot. HOLD intentionally keeps that override until cleared or replaced.
- FOLLOW copies another track's published source snapshot and applies the follower's own register and performance settings.
- Direct chord sound is an independent OFF / ON setting. A source can publish harmony silently; mute affects sound, not publication.

The first sequencer-fed release accepts existing literal chord steps. Semantic role patterns become available when descriptor recording is added.

Followers choose BASS, CHORD, or ARP behavior. BASS selects explicit bass, otherwise known root, otherwise lowest pitch. It can use its own rhythm pattern. CHORD uses its own voicing and strum. ARP uses its own phrase, octave range, and gate.

Follow sources must be tracks with a local source, not other followers. Reject self-follow and follower chains in the UI and validate them on load. This prevents cycles and makes all three parts independent of track iteration order. Generated performance notes never feed harmony publication.

## 6. Timing and transport contract

Treat chord commit timing and phrase restart as separate controls.

| Control | Choices | Meaning |
| --- | --- | --- |
| CHANGE | NOW / STEP / BAR | Commit pending harmony immediately, at the next consumer arp boundary, or next bar |
| RESTART | CHANGE / FIRST / CONTINUE / BAR | Reset phrase on harmony change, first activation, never on chord changes, or each bar |
| SKIP | ADVANCE / RETRY | Probability silence advances the phrase, or retries the same selector next tick |
| HOLD | OFF / REPLACE / ADD | Release normally, replace a latched live chord, or accumulate live input |

Rules:

1. At a shared boundary: apply effective sequencer locks/conditions, update source harmony, resolve follower snapshots, commit due changes, apply restart rules, then emit performance events.
2. Release notes whose deadlines expire before starting replacement notes at the same tick. Explicit ties are the exception.
3. Multiple pending changes coalesce to the latest complete snapshot.
4. CHANGE=NOW affects pitch selection immediately but does not insert an extra arp hit between ticks. Immediate chord playback can sound at commit time.
5. While stopped, NOW and STEP remain usable on the local arp clock; BAR changes remain pending until transport starts. The UI indicates a pending change.
6. PLAY resets transport-synced phrase phase to the downbeat. Legacy STOP and panic clear latch state and release notes; preserve this behavior. A future opt-in transport policy could retain a HOLD snapshot, but it is not the compatibility default.
7. Changing arp rate takes effect at the next boundary of the new grid without replaying a hit. Queued note-offs retain their original deadlines.
8. Existing legacy timing remains a migration option; new projects use STEP change, FIRST restart, and ADVANCE skip.

Sequencer ST_NOTE supplies a new snapshot. ST_TIE retains it. ST_REST clears it. A step rejected by a fill condition acts as a rest; a recording replay-suppression decision must not suppress harmony publication. Sequence gate affects direct chord playback, not snapshot duration. HOLD does not override an explicit sequencer rest. A separate future REST=KEEP option can be added if musically useful.

## 7. Arp phrase and expression

Retain the current note-list modes. Add a bounded phrase mode with 1–16 cells, each containing:

- Selector: indexed tone, root, third, fifth, seventh, ninth, lowest, highest, or rest.
- Signed octave offset.
- Accent/velocity adjustment, gate override, tie flag, and ratchet count (1–4).

Semantic selectors resolve from harmonic intent, independent of the current inversion. Indexed/lowest/highest selectors resolve from voiced pitches. Unavailable semantic extensions produce a rest; the UI marks the cell unavailable. The unknown-root fallback described above is the sole initial exception.

Gate is limited to one cell interval initially. A tie extends the existing event without another note-on only when the next selected pitch matches; otherwise release and start the new pitch. Ratchets subdivide that cell after swing has determined its duration. Reject tie+ratchet combinations in the editor.

Velocity modes: FIXED, INPUT, PATTERN. INPUT uses the selected tone's input velocity where available, otherwise the chord's trigger velocity. Local fixed-velocity keys retain their current baseline. PATTERN applies bounded adjustments and clamps emitted note-on velocity to 1–127.

Probability is evaluated once per cell, before ratchet expansion. A skipped cell follows SKIP policy and ends an existing tie. User-programmed rests always advance. Random mode gets per-track PRNG state so activity on another track cannot alter its sequence. A saved seed supports repeatable playback and baking.

Euclidean fill is an editing operation that populates rests/hits in the phrase; it is not a second runtime scheduler. Additional direction modes can follow after this shared phrase engine is stable.

## 8. Chord expansion and voicing

Add explicit major, minor, dominant seventh, major seventh, diminished, augmented, sus2, and sixth qualities alongside existing diatonic construction.

Voicing controls include inversion, close/open spacing, register bounds, and voice-leading policy. Voice-leading candidates must remain in range. Minimize movement with deterministic tie-breaking; optional policies keep bass root or top voice. Reset voice-leading history on project load, panic, and explicit voicing reset, not on unrelated UI navigation.

Keep four voiced pitches. Extensions beyond capacity use a documented omission order: retain third and seventh where present, then requested extension, then root, then fifth. Explicit bass can be routed to a BASS follower without consuming the chord track's four-tone budget. Never silently expand the sequencer note format.

Strum supports milliseconds or musical divisions, ascending/descending/alternating direction, and an accent curve. Schedule each tone once and fan it out to audio and generated MIDI at its actual start time. Releasing or replacing a chord cancels starts that have not fired. Queue overflow reports a counter and drops the latest pending starts deterministically; it must not convert a timed chord into an unexpected burst.

Eight project-level chord memory slots store descriptors and voicing. Recall uses CHANGE timing. In degree mode, slots follow the selected root/scale; absolute mode preserves pitch. Hardware key mapping is finalized during UI prototyping.

## 9. Note ownership and event scheduling

Replace pitch-only held-note bookkeeping with bounded source instances. Distinguish local key, MIDI port/channel/note, sequencer, follower, and generated-event identities. A local chord trigger owns its complete expanded pitch set.

Maintain aggregate pitch membership while retaining per-owner contributions. Releasing one owner cannot remove a pitch held by another. Modifier revoicing replaces that owner's set atomically and preserves common tones. Duplicate note-ons from the same MIDI port/channel/pitch retrigger or replace that owner's contribution rather than incrementing an unbounded count. USB and TRS identities remain distinct.

Use explicit active flags; MIDI pitch 0 cannot mean inactive. Apply this audit to arp, mono, memory, preset-pattern encoding, and recording paths rather than fixing only `arp_note`. Skip pitches outside 0–127 during octave expansion instead of clamping them to duplicate edge pitches.

Timed events carry owner, target track, pitch, velocity, start/deadline, and cancellation generation. Cancellation invalidates queued starts and releases events already started. Internal voice allocation can still steal voices under the existing budget; MIDI note lifetime must not depend on whether an audio voice survived allocation.

All queues and owner tables have compile-time capacities. Overflow never evicts an unrelated held owner. Reject a new owner/event, increment diagnostics, and retain the existing valid state. Critical release handling needs reserved capacity or direct processing so saturation cannot cause stuck notes.

## 10. MIDI interoperability

Output choices: SOURCE, GENERATED, BOTH, OFF. Configure source and generated channels separately; BOTH on the same channel warns in the UI about overlapping pitches. Legacy projects restore their existing key/SEQ output behavior through a compatibility setting.

- SOURCE emits expanded source chord pitches before performance rhythm/strum processing.
- GENERATED emits the exact scheduled performance heard internally, including strum, gate, ties, accents, and ratchets.
- BOTH emits both streams; aggregate ownership prevents premature note-off if they share a port/channel/pitch. A fresh attack on an already active MIDI pitch uses a deliberate off/on retrigger policy.
- MIDI input is not echoed through SOURCE by default. Its transformed generated performance may be sent when GENERATED is enabled. Raw thru, if later added, is explicit.
- CHORD ROOT MIDI input bypasses the local white/black keyboard gesture mapping. Each received root generates a chord; quality/modifiers come from track settings or locks.
- Add sustain CC64 and all-notes-off/all-sound-off handling with scoped cleanup. Sustain affects live input ownership; generated gates follow the arp phrase. Panic clears pending strums, active generated notes, and latch state.

Retain existing USB/TRS clock selection. Test start, continue, stop, and clock-source changes; release generated notes if the selected external clock times out while running. Define the timeout using the existing clock implementation before coding it. Do not add automatic raw MIDI echo loops.

## 11. Recording and baking

Two record modes:

1. SOURCE records chord descriptors, changes, and modifiers. These can be reharmonized later. Until metadata support lands, record literal chord snapshots and label them accordingly.
2. RESULT records scheduled generated notes with effective velocities and durations, including follower and strum output.

SOURCE metadata is a versioned per-step sidecar; retain existing 10-byte literal steps as fallback. Per-step metadata is valid only when explicitly marked. Editing a step's literal pitches clears its descriptor to prevent conflicting sources of truth. Undo captures both step and sidecar.

BAKE TO PATTERN renders a chosen duration into a scratch pattern using an isolated copy of generator state. It does not emit audio/MIDI, alter the source, advance live PRNG state, or recursively record its own result. Commit the destination atomically as one undo operation.

The existing sequencer cannot represent arbitrary high-rate or millisecond timing exactly. Bake chooses a supported destination division and quantizes starts/durations into existing steps and microtiming. Show a preview of lost timing, more than four simultaneous pitches, or events beyond 64 steps. Commit only a representable result; offer a shorter span or coarser phrase when limits are exceeded. SOURCE recording and baking must not promise lossless capture outside this format.

## 12. Hardware controls

Preserve the current SCL and ARP pages. Add four-control pages rather than replacing familiar gestures:

| Page | Four controls |
| --- | --- |
| HARMONY | Source, direct sound, change timing, follow behavior |
| ARP FLOW | Restart, hold, skip policy, velocity mode |
| VOICING | Inversion, spread, low limit, high limit |
| MIDI PERF | Input interpretation, output stream, source channel, generated channel |

A phrase layer uses the step keys to select cells and knobs to edit selector, octave, gate, and accent. Secondary controls expose tie/ratchet. Display source track, resolved chord/root when known, active phrase cell, latch, and pending change. No chord name is invented for literal note sets.

Project-level links stay out of sound-preset recall by default. Loading a sound must not unexpectedly rewire a progression. Parameter locks may change musical settings such as quality, voicing, and rhythm; source routing and MIDI channels are initially not lockable.

## 13. Persistence and compatibility

- Add common `P_*` parameters only immediately before `P_E0`, following current preset count mapping. Keep existing enum values stable.
- `UP_PMAX` is currently 72. Budget new scalar settings before adding them; complex phrases, links, and chord slots use versioned project extension records instead of exhausting parameter slots.
- Freeze the current project layout as an explicit old-version reader before changing `P_COUNT` or structure sizes. Introduce a new writer version; do not reinterpret old blobs using the new `project_t` size.
- Validate extension lengths, counts, ranges, descriptor roles, and routing links before exposing them to the audio path. Missing extensions yield legacy defaults.
- Audit stored parameter-lock IDs, preset extras, editor/protocol parameter indexing, autosave, section copies, undo, and flash object capacity. Firmware compatibility work is in scope; application feature work is not.
- Persist configuration, phrase data, descriptors, and seed. Never persist active owners, event queues, sounding notes, pending changes, or voice-leading history.
- Old firmware is not expected to understand new project versions. Document this limitation and retain old fixtures for migration tests.

## 14. Implementation plan

### Phase 1: reliable shared performance foundation

Add source ownership and atomic chord batches; explicit active flags; velocity preservation; bounded shared scheduling; generated MIDI timing; output stream routing; lifecycle cleanup. Keep legacy musical behavior selectable.

Exit: overlapping sources release correctly, audio/MIDI strum timing matches, saturation has no stuck notes, old fixtures load unchanged.

### Phase 2: sequencer-fed arp and timing controls

Publish literal sequencer snapshots, introduce source selection, change/restart/skip controls, and deterministic boundary processing. No descriptor sidecar is required yet.

Exit: a four-chord recorded progression drives an arp without duplicate direct playback; rests/ties and live overrides behave as specified.

### Phase 3: semantic chords and phrase engine

Add descriptors, role selectors, expression cells, per-track PRNG, SOURCE metadata recording, and bounded result baking.

Exit: root–fifth–third–octave survives inversion, saved source recordings reharmonize, baking is repeatable and reports representation limits.

### Phase 4: harmony followers

Add one-level track links, bass/chord/arp consumers, independent registers, silent publication, persistence, and hardware status.

Exit: pads, bass, and arp follow one progression consistently regardless of track processing order.

### Phase 5: expanded performance controls

Add explicit qualities, constrained voice leading, bass overrides, tempo-synced strum, Euclidean phrase fill, and chord memory.

Exit: each feature has bounded resource use and works with SOURCE/RESULT recording and GENERATED MIDI.

## 15. Validation and resource gates

Extend the existing host test harness, including sequencer, project, preset, and target-budget coverage where applicable. Test observable note/event traces rather than duplicating implementation logic.

Required cases:

- Two chords share a pitch; either owner can release first. Add simultaneous local/USB/TRS input and same-pitch repeats.
- Modifier revoicing under HOLD preserves common tones and never publishes a partial chord.
- Pitch 0, pitch 127, empty input, maximum octave span, unavailable roles, and queue saturation.
- Chord changes immediately before/on/after grid and bar boundaries; rate/swing changes; probability ADVANCE/RETRY.
- ST_NOTE/ST_TIE/ST_REST, fill rejection, recording replay suppression, live override, and source clearing.
- Stop, panic, project/section load, engine change, MIDI route change, sustain release, external-clock loss, and reconnect.
- SOURCE/GENERATED/BOTH streams, shared output channel, balanced note-offs, and matching strum event times within existing scheduling resolution.
- Fixed/input/pattern velocities; ties, ratchets, missing chord tones, and per-track random independence.
- Old project/preset fixtures, corrupted extension records, parameter-lock migration, undo, and round-trip new storage.
- Bake twice with the same seed; identical result, unchanged live state, and explicit rejection of unrepresentable output.

Before each phase ships, measure flash growth, static RAM, maximum queue occupancy, stack usage, and worst-case block processing on the target. Exercise all three synth parts with dense chords, fast ratchets, and heavy engines. The 256-frame audio block remains the deadline; use the repository's target-budget thresholds and establish measured headroom before approving added runtime work. No allocation or unbounded search is allowed in the real-time path.

## 16. Decisions to resolve during implementation

- Exact queue/owner capacities and snapshot layout, based on measured RAM and event throughput.
- External-clock timeout behavior compatible with the current clock receiver.
- Flash extension capacity and whether a separate preset format is needed for reusable phrases.
- Final hardware gestures for chord memory and secondary phrase controls.
- Whether later versions should support REST=KEEP, follower chains, or larger bake destinations. These are deferred and do not block the initial design.
# Chord latch control — 2026-10-08

SCL page 2 knob 4 now shows LATCH and controls the existing per-track ARP HOLD parameter.
Outside CHROM, a latched chord remembers its root and responds immediately to modifier
press/release. Only changed tones are released/started, with matching MIDI messages;
common tones continue ringing. Minor and combined seventh/sus4 restoration pass host tests.
With CHORD enabled and ARP off, releasing a keyboard chord retains its owned notes;
the next root replaces the retained chord. LATCH off, CHORD off, enabling ARP, STOP,
or panic releases the retained notes and corresponding MIDI output. With ARP enabled,
the existing arp HOLD behavior applies. The setting survives sound preset changes.
No parameter IDs or project format changed. Keyboard sustain/replacement/disable/STOP
and the real panel knob mapping pass host tests; the UI passes 20,000-frame fuzz.


## Phase 1 bounded foundation — 2026-10-08

Implemented in `src/harmony_owners.h` / `src/harmony_owners.c`, with dedicated
`tests/harmony_owners_test.c` event traces. This is an **inactive module**, not full
Phase 1 or production arp ownership. `seq.c` remains unchanged: its pitch-only
`input_on` / `input_off` interfaces cannot preserve local/USB/TRS identity.

The caller-owned table accepts 16 owners per destination track, each contributing
at most four unique literal pitches. Identity is source kind, channel, and trigger;
USB and TRS are distinct. Repeated same-identity input replaces its set. Explicit
active flags allow pitches 0 and 127. Aggregate reference membership prevents an
owner release from removing another owner's shared pitch. Replacement commits the
complete set and returns sorted aggregate off/on transitions; common tones never
retrigger. Calls require serialization; atomic means one complete caller-visible
commit, not interrupt-safe lock-free publication. Unknown release is harmless.
Saturation rejects new owners without eviction, increments a saturating diagnostic,
and still permits existing-owner replacement and release. Invalid batches preserve
state. Panic clears owners/membership while preserving diagnostics; the existing
output panic must release sounding notes (the four-tone delta is not a panic list).

Host traces cover overlapping chords released in either order, modifier-style
revoicing, simultaneous local/USB/TRS ownership, repeated USB note-ons, pitch edges,
duplicate tones, invalid input, full-table rejection, release/revoice at saturation,
and panic. Strict `-std=c99 -Wall -Wextra -Werror -O2` compilation and assertions pass.
No real MIDI decoder, keyboard gesture, audio scheduler or output route is activated
by these tests; those integration traces remain required before activation.

Exact future hooks: compile `src/harmony_owners.c`, include `harmony_owners.h`, and
allocate/init one `harmony_owners` per synth destination. Carry source/channel/trigger
identity from local keys and the physical MIDI ingress port to the arp input boundary.
Submit each expanded local chord as a single `harmony_owner_replace`; release the
same identity with `harmony_owner_release`. Consume deltas only after commit, release
before attack, and preserve legacy held-list press ordering and HOLD physical-count
semantics explicitly. Replace modifier owners as complete batches. Clear owners on
STOP/panic and every existing project/engine/route lifecycle cleanup. Do not derive
identity from pitch-only `input_on` or guess a MIDI port downstream.

Resource measurements on the host: owner 9 bytes; state 276 bytes (16 owners +
128 pitch memberships + 32-bit diagnostic); transition buffer 10 bytes. Three
instances would cost 828 static bytes. Replacement scans 128 pitches and 16 owners,
with at most four-tone membership searches; no allocation or staging scene RAM.
Current target RAM is 92,980 / 98,304 bytes, atomic scenes are disabled, and existing
CPU budget failures remain unresolved. This module currently adds zero allocated
production state; target flash, stack and block cost must be measured before wiring.
No budget thresholds may be relaxed. Scheduling, velocities, generated MIDI timing,
routing, semantic snapshots, sidecars and followers remain proposed.

Compatibility gates: preserve CHROM literal black keys, the 700 ms ARP/SEL-SCL
capture, octave/latch behavior, fixed qualities and persisted IDs/layout. Since
2.4.11, CHORD+ARP latch supports toggled black-key modifiers after root release;
do not regress that gesture or change legacy STOP/panic latch clearing. The module
has no HOLD policy and does not reinterpret those gestures.

## Production literal sequencer-to-arp slice — 2026-10-08

The existing **ARP 2 / ORD** control now has two appended opt-in choices:
`SNOTE` (sequence + live pitches, sorted) and `SPLAY` (sequence + live pitches,
press/stored order). Existing `NOTE=0` and `PLAY=1` retain direct recorded-step
playback and live-only arpeggiation. Enable an ARP mode and select SNOTE or SPLAY
on the desired synth track. Its recorded four-note steps then drive the existing
arp, with no additional direct sequenced chord. ARP OFF restores direct playback.
No new parameter IDs, project versions, preset counts or step layouts are added.
This intentionally combines routing with the existing order control for a small,
usable hardware-accessible release; separate source/direct controls remain future work.

`harmony_seq_source` in `harmony_owners.h` is the production boundary: one exclusive
sequencer contribution per synth track, separate from the existing live held list.
The general 16-owner table in `harmony_owners.c` remains inactive. Allocating that
table or claiming full physical/USB/TRS owner identity is outside this slice.
Sequence replacement never calls live arp_add/arp_remove, changes arp_phys, replaces
HOLD's live chord, or erases an overlapping live pitch. The generator deduplicates
sequence/live union pitches. Existing live-to-live same-pitch ownership limitations
remain; this release does not solve repeated or overlapping MIDI owners among live
ports. It does not infer semantic roots, add followers, or reinterpret chord gestures.

Timing/lifecycle contract for the opt-in route:

- NOTE publishes one complete deduplicated literal snapshot at the existing effective
  microtimed step boundary after locks/conditions. Recording replay skip does not
  suppress publication. Step ratchets, gate, slide and strum do not create generator
  hits or expire the snapshot. The arp retains its own rate/gate/swing/probability.
- TIE retains the previous snapshot. REST, empty NOTE, and a rejected fill condition
  clear it even under HOLD. Each explicit NOTE/rest change releases the current
  generated gate before publishing; unchanged pitches may therefore attack again
  at the next arp boundary. Live input remains available after a sequence rest.
- First source activation starts the existing arp first-note policy; subsequent
  changes continue phrase position and invalidate the shuffle pool. Changes do not
  insert an extra between-grid hit. Probability retains legacy RETRY behavior and
  generated velocity remains 100. No new restart/change/velocity controls are implied.
- PLAY/section reset clears sequence publication and resets transport phase; the
  next effective step republishes. STOP clears sequence state and the opt-in live
  latch/held count, releases generated output and pending direct sequence output.
  Panic/project adoption clears the snapshot and generated output. Pattern clear
  invalidates publication; arp output cleans up on the next events block.
- Switching between legacy and sequence routes releases old generated/direct gates,
  clears the snapshot and adopts the effective current step in the next sequence
  block. ARP mode changes and HOLD-off retain their existing live behavior.
- USB/TRS clocks retain the existing 500 ms timeout fallback to internal tempo.
  No new external-clock-loss stop policy is introduced.

Generated notes use existing seq_out_on/off and arp_release bookkeeping and the
existing GLO SYSTEM MIDI=SEQ option. Tests assert balanced generated note-on/off
traces across replacement, route changes and STOP. Physical keys retain their
legacy immediate source MIDI output. Source/generated same-channel collisions
remain an existing limitation: these are not independently routed or aggregate-owned
MIDI streams. MIDI input is not raw echoed. PULSE still uses the shared eight-voice
allocator and a maximum 64 generated tones, independent of surviving audio voices.

The pool is bounded to the first 16 unique base pitches, with live press order first
and sequence stored order second, then the existing maximum 64 octave-expanded tones.
At saturation a sequence contribution remains published but excess pool pitches are
not selected; no live owner is evicted. This deterministic compromise avoids growing
the audio-path arrays. Existing octave edge clamping remains a legacy limitation.

Persistence: old valid ORDER values and defaults remain live-only. New values 2/3
round-trip in the unchanged project/preset layout and are range-validated by the
existing descriptors. Historical project readers retain their existing count mapping.
Older firmware clamps new values to its maximum PLAY=1, restoring legacy direct
playback. Factory sound recall currently resets ORD with the other arp sound controls;
user sound recall can restore its saved ORD. Routing is deliberately not promised as
a separate project-only setting in this slice. Making routing survive all sound recalls
would require the shared UI preset application scope and a separate policy decision.

Validation and integration:

- `tests/run_seq_arp.sh` builds the real host engine from `tests/seq_arp_test.c`;
  no new production C translation unit or build registration is required.
- Assertions cover normal events_block execution, no duplicate direct chord,
  recording skip, microtimed NOTE/TIE/REST, rejected fill, step replacement,
  overlapping live/sequence pitches, actual USB/TRS packet ingress, legacy direct
  routing, ARP OFF, latched physical chord modifier revoicing, STOP/panic/project
  adoption, project round-trip, pitch 0/127 and saturated 64-tone capacity.
- Dedicated optimized and UBSan tests pass. Existing seq2, scale/chord/latch and
  historical project-format tests pass. Host hardware-address cast warnings are
  unchanged. The shared test runner includes this suite after parent integration.
- `tests/seq_arp_size_audit.c` compiled with the real JieLi pi32v2 clang: source
  owner 6 bytes, three owners 18 bytes; target track_t 2072 bytes, step_t 10 bytes,
  P_COUNT 61 and P_E0 53, all existing layouts unchanged. The object records these
  values in seq_arp_audit_sizes; the 18-byte common state symbol is inspected with
  pi32v2 objdump. No full firmware image was built or linked by this component task.
- This slice allocates 18 static bytes rather than 828 bytes for three general tables;
  it adds no voice reservation or scene/preset storage. Per-step replacement scans
  at most four pitches; generator union adds at most 4x16 comparisons to the existing
  bounded pool construction. No allocation, new queue or unbounded traversal occurs.
- The previous combined target log (2.4.14) records image 579656 bytes and static RAM
  93428 / 98304 bytes. These are baseline observations, **not a new combined audit**.
  Final linked flash growth, RAM alignment, stack/CPU budget and on-device timing
  remain parent integration gates; no thresholds were relaxed and no target safety
  claim is made from the host tests or isolated sizeof probe.

Deferred tradeoffs: full live source/channel/trigger ownership, independent source
routing/order controls, explicit direct+arp mode, source/generated MIDI aggregation,
full live-owner velocity arbitration, scheduled strum output, semantic descriptors and cross-track followers.

Implemented expression slice: live MIDI and sequenced per-note dynamics now reach
all arp modes, octave expansion, PULSE, generated MIDI and recording. Shared
live/sequence pitches use strongest velocity; route changes transfer sounding
snapshots across ties. This adds 72 static bytes without changing persistent
layouts. Detailed policy, overlap/clamp behavior, tests and remaining limitations
are documented in [ARP-EXPRESSION.md](ARP-EXPRESSION.md). Full source identity and
semantic chord architecture remain future work.

Combined integration checkpoint: firmware 2.4.15 links with image 580296 bytes and RAM
93444/98304, unchanged pool and 925-instruction call-free RAM text. HAL checks pass;
the existing ISR cost check still fails and no thresholds were relaxed. These target
checks do not establish physical deadline headroom. Current evidence lives in
[verification](<../../android/design docs/VERIFICATION.md>).
