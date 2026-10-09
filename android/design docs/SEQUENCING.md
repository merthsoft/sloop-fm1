# Sequencing, clock and scenes

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Latest implementation checkpoint (2026-10-08): App patterns now have a touch piano roll
with creation, single/multi selection, group movement, duration resizing, deletion,
quantization, zoom/pan, protected notes and existing undo/redo. The gesture/domain runner passes 71 checks.
See [Touch app piano roll](#touch-app-piano-roll--october-8-2026) for actual behavior and
[verification record](VERIFICATION.md) for changed files, hooks and limitations.
Combined Android compilation and APK installation pass.

UI refinement checkpoint (2026-10-07): The Android app grid now shows one selected bar as two rows of eight steps, with highlighted bar/step selection. Velocity/duration/tempo, transforms/history, note lists and native hardware editing are expandable sections; the stored pattern remains 64 positions.

## Implementation checkpoint (2026-10-07)

Combined integration: continuous app MIDI looping, Perform recording, staged local scene
restores, beat/bar/phrase switching, ordered arrangements and MIDI CC dispatch are wired.
Sequence exposes visible recipe entry and Scenes & arrangements. Hardware scene transaction
and measured timing remain future work; see [verification record](VERIFICATION.md).

Follow-up: prompt entry now uses the exact `PromptEditor` vocabulary with channel/bar/step
selection, optional selected-step note locks, complete diffs and one undoable local apply.
Native selected-step commands are also reviewed before apply. Scenes can be managed and
arrangement steps reordered/repeated. Explicit stopped-only FM1 scene sound transfer is
available separately; live atomic hardware scene switching is still pending.

The standalone .NET 10 [Sloop.Sequencing](../src/Sloop.Sequencing/README.md) library
implements local models, validation, selection, editing proposals and transaction history.
It references neither Android, Core nor Protocol. Android editors and lossless native
wire codecs/sending are integrated through Workstation. Continuous app-pattern MIDI looping and
Perform gesture capture, app scenes and stepped MIDI CC automation are implemented;
measured scheduling, external sync and atomic full hardware scene application remain planned.
Playback and execution-mode sections below describe the complete target architecture.

`AppPattern`/`AppNote` have stable IDs, explicit PPQ, tick onset/duration, part identity,
velocity and zero-based MIDI channel. `HardwarePattern` retains three synth tracks and one
drum track, all 64 stored steps per track, four physical synth slots, sixteen drum lanes,
inactive slot/lane data, ties/slides, shared synth-step velocity, native levels/ratchets,
micro timing, fill conditions, parameters and up to 24 locks per track. Hardware steps do
not provide independent note durations or per-note MIDI velocity.

| Target | Implemented local edits | Constraints |
| --- | --- | --- |
| App notes | Move, resize, transpose, duplicate, nearest-grid quantize, velocity delta, seeded thinning | Phrase bounds and MIDI ranges validate; half-grid quantize rounds forward |
| Synth steps | Whole-step move/duplicate, active-slot transpose/level, shared velocity delta, micro quantize | Move/copy carries locks; independent resize and synth thinning reject |
| Drum steps | Whole-step move/duplicate, selected-lane level/thinning, whole-step micro quantize | Drum pitch/velocity and lane-only move/copy/micro edits reject |

Move/copy rejects overlap, occupied destinations, active-length overflow and crossing or
bordering tie/slide chains, including loop wrap. Variation is deterministic thinning, not
fill generation, syncopation or reharmonization. Unselected material stays unchanged;
changing a locked part/event rejects the complete edit. Ranges are half-open; app ranges
select note onsets. Tick, Frame, StepIndex, StepOffset and MicroOffset are separate units;
micro timing remains native 1/64-step units (-32..31).

`EditProposal` retains immutable before/after states, note or complete-track diffs and
provenance. `EditHistory` applies against the exact captured parent snapshot and supports
full undo/redo with fresh restoration revisions. `EditProposal.Compose` groups a contiguous
preview chain into one atomic local gesture. Move preserves IDs; duplicate creates new IDs.
Undo/redo history is session-local. Workstation persists app/native content and identities;
Android provides continuous app-pattern MIDI playback, gesture recording, and native
read/compare/write/readback reconciliation.
The UI exposes insertion/update/removal, pitch/velocity/duration, transpose, quantize and
thinning for app notes; native UI exposes transpose, levels and micro quantize. Other domain
operations in the table are not yet all exposed as Android controls.

Supply per-track capability identity, lockable IDs and engine-specific ranges. Unknown lock
ranges and invalid values reject rather than clamp. This firmware checkout uses TFLT=50,
STRUM=51, VLEAD=52 and engine IDs 53..60, newer than the protocol prose's older 50..57 range.
Stable IDs are app-owned; readback reconciles them by physical slot/lane and lock key.
Native wire adapters preserve inactive slots/lanes and supported lock data. Export of a
complete firmware project, including unused lock padding, remains separate work. The
current native adapter requires protocol 9, 61 parameters and engine start 53; old layouts reject.

The package-free [test project](../src/Sloop.Sequencing.Tests/README.md) passes 20 scenarios
covering preservation, identities, ranges, capacities, locks, stale proposals, compound
transactions, undo/redo and tie/slide boundaries. Domain tests do not certify playback
jitter, polymeter, swing, scenes or physical-device interoperability.

## Two execution modes

### Touch app piano roll — October 8, 2026

Sequence now includes a six-row touch piano roll for the selected app MIDI channel.
Rows are 48dp high. Select, Draw, Move, Resize, Pan and Multi select are explicit modes in two rows
of large buttons. Draw inserts only on release of a stationary tap; dragging in Draw
does not insert. Tapping existing material selects it. Select uses expanded 48dp horizontal
hit targets, preferring a literal note rectangle when expanded targets overlap. Move and
Resize drag the selection from anywhere in the canvas, avoiding tiny edge handles.
Select replaces the selection with one note. Multi select taps toggle individual notes;
dragging a box adds intersecting note rectangles on the edit channel to the selection.
The box previews until release and cancellation discards it. Select channel and Clear
selection provide explicit bulk-selection controls. All selected notes are highlighted.
Preview changes leave the pattern untouched until release. Android cancel, an additional
pointer and view detachment discard editing gestures. Pan never creates proposals.

Creation and movement/duration deltas snap to the selected quarter/eighth/sixteenth/
thirty-second grid; half-grid rounds forward. Moving existing off-grid notes retains
their onset offset until the explicit Quantize selected note action. Phrase/MIDI bounds
are clamped, and clipping at the phrase edge can produce a duration shorter than the grid.
Creation defaults to the existing velocity/duration controls and preserves the edit channel.
Moves preserve ID, part, channel and velocity. Group movement clamps one shared tick/pitch
delta against every selected note, preserving relative spacing at phrase/MIDI boundaries.
Group resizing applies one duration delta bounded by the shortest note's minimum and
every note's phrase end; existing sub-grid durations can remain sub-grid. Quantize rounds
each selected onset independently. App-note overlaps remain legal under existing domain
validation; native destination/tie/slide collision rules are unchanged. Deletion and
quantization target the selected stable IDs, and channel changes discard ineligible IDs.
Zoom and pan have bounds; Earlier/Later and pitch-page buttons provide alternatives to drag.
Tick range and visible MIDI pitch range are displayed. Reconciliation to a different source
or edit channel cancels any captured gesture before a late release can propose an old edit. Native FM1 and numeric/step editing
remain accessible, as do recipes, scenes, looping and note lists.

Each release produces one existing AppEditor proposal, applied through EditingWorkspace's
save-before-adopt path and shared undo/redo. Group replacements/deletion compose a contiguous
AppEditor proposal chain without applying intermediate states; any changed protected member
rejects the complete edit. Protect selection uses existing EditLocks for all selected IDs;
roll edits and this editor's legacy add/update/delete/track transforms honor those locks.
Locks/viewport/selection are activity-local, not session-persisted. Undo explicitly restores
history regardless of current protection. Prompt edits and composition apply share the same
activity-local locks. Protected identities survive temporary absence during Undo/Redo.
Workspace/session/scene adoption clears these local protections and selection through the
integrated `ResetPianoRollForAdoptedWorkspace()` hook.

The dedicated Sloop.PianoRoll.Tests runner passed 87 focused checks on October 8, 2026.
It links the production gesture model and validates cancel/drag safety, hits, edits, bounds,
unselected/channel preservation, part/event locks, history/staleness and viewport navigation.
The combined APK packages multi-note editing with zero warnings/errors; it has not been
installed or touch-tested in this follow-up. `PianoRollEditor` routes group proposals through
the existing `editing.EditPattern` callback. User hardware acceptance is recorded separately
in STATUS.md. Pinch zoom,
velocity drawing, note audition and native hardware piano-roll rendering are future work.

Integration follow-up: the numeric step grid now follows the app pattern's actual PPQ and
loop length, including all 1–16 composition bars. Bar selectors wrap in rows of four.
Out-of-range step selection after shorter-loop adoption is clamped; inserted notes clip
to the loop end. Native FM1 editing remains limited to its own 64 stored steps and shows
an explanation for later app steps instead of indexing past the native array. Touches
outside the roll, nonfinite coordinates and zero-sized surfaces cannot insert notes.

Hardware mode edits and plays FM1 patterns. App mode schedules MIDI from an app document.
The selected mode is prominent. Additional app lanes target existing parts or external devices;
they do not expand the FM1's number of synths. Mixed operation is an advanced explicit setup
with one owner per destination to avoid duplicate playback.

## Hardware pattern editing

Model the actual format: four tracks, 64 steps, four synth notes per step, 16 drum lanes,
ties/slides, native hit level/ratchet encoding, micro offsets, fill conditions, and 24 locks per
track in current firmware. Read capacities and parameter ranges when possible. Preserve
unmodified fields in edits; do not round-trip through a simplified model that loses data.
Use a drum grid, synth piano roll constrained to representable steps, and step inspector.

Copy/paste and transpose validate note/lock capacity. Multi-step edits have local atomic undo;
current wire writes may partially apply. Show partial application and offer reconciliation.
A future firmware transaction makes the device operation atomic. Recording hardware notes
refreshes affected steps, including other-track data absent from the current push coverage.

## App scheduling target

Current app looping and the performance arp/strum player run on workers using monotonic
musical deadlines. Tempo changes preserve phase; cancellation releases owned MIDI notes.
Transport exposes exact queued ticks and epochs for scene integration. These immediate MIDI
senders do not establish measured timestamped lookahead, external sync or arrangement playback.

Documents use explicit PPQ (initially 960), note start/duration, velocity, channel and automation.
A scheduler converts musical time to monotonic timestamps with a bounded lookahead, initially
tuned on hardware rather than claimed as a fixed latency. Do not schedule through render loops,
Task.Delay chains, or UI timers. Sending events early works only if the platform/device honors
timestamps; otherwise use a dedicated timed sender and measured dispatch behavior.

Tempo changes re-anchor future events without moving already-sent notes. Stop cancels unsent
events and releases owned voices. Seek clears voices and starts from a defined position;
chase sustained notes only under an explicit setting. Parameter automation is lower rate than
note events and does not compete with bulk sample transfers.

## Clock authority

Hardware patterns default to FM1 internal clock; app arrangements default to phone master with
FM1 USB sync when required. External clock is a later tested option. MIDI clock is 24 pulses
per quarter and requires phase/transport handling, not just averaged BPM. START and CONTINUE
have different position semantics. Clock loss is an explicit state with a user-selected stop
or hold policy; do not silently switch masters. Current firmware MIDI transport depends on
the configured sync source; explicit remote transport is proposed in FIRMWARE.md.

Visual playheads interpolate recent position telemetry and mark stale data. They never become
clock authority. A firmware timestamp/position extension anchors hardware progress; legacy
mode cannot present guessed position as sample-accurate truth.

## Scene lifecycle

Scene states: Edited -> Validating -> Prepared -> Queued -> Active, plus Failed/Canceled.
Hardware scenes reference sections A-D. App scenes reference sounds, patterns, mixer/macros,
and sample slot fingerprints. Prepare verifies dependencies and current revisions. Launch
chooses next step/beat/bar or immediate according to capability; next-bar is the default.

Only one queued launch initially. Replacing/canceling it is an acknowledged operation.
Panel launches and app launches share the firmware queue; notify the app when panel action
changes its request. Prepared multi-parameter changes commit inside the firmware boundary,
not as a last-minute burst of SysEx messages. Legacy devices expose only supported operations;
they must not claim atomic quantized app scene changes.

Chains store scene references, repeat counts, and loop/stop behavior. Resolve all sample
dependencies before starting. No automatic flash save/upload during chain playback. Scene
switch note policy is explicit (release, preserve compatible tails, or retrigger); initially
release destination-owned notes to prevent accidental cross-scene sustain.

## Acceptance

Test polymeter, swing, triplet divisions, ties at loop wrap, tempo changes, start/continue,
clock loss, stop during queued events, scene cancellation at the boundary, panel/app conflicts,
and stale slot dependencies. Measure jitter under simultaneous ordinary control traffic.
Audio stem rendering remains separate multi-pass recording, not a sequencer export promise.

## Continuous transport and Perform capture — 2026-10-07

App mode now exposes Loop app pattern and Stop controls. PatternLoop runs on a worker with
absolute Stopwatch musical deadlines; phrases repeat until cancellation. Duration-end events
precede new onsets at loop wrap. Overlapping channel/pitch notes retain ownership until the
last duration ends. Stop synchronously releases app-owned MIDI notes before ports close;
worker completion performs a second cleanup. Disconnect epochs cancel pending work. Tempo
changes re-anchor current phase and future deadlines. Edits affect the next explicitly started
playback snapshot; live pattern replacement is reserved for scene integration.

Perform recording stores emitted notes as one EditProposal, with existing persistence and
undo/redo. Default overdub preserves existing material; replace removes notes only on channels
that actually received captured notes. A silent one-bar count-in is optional. The count-in
has no click track; notes held through its end start at tick zero, earlier completed notes
are ignored. Recording uses Perform's BPM. Sequence's BPM controls loop playback. Arbitrary
recorded MIDI channels can be selected for editing. Canonical generic-MIDI channel remapping
(1–3 and 10) remains in the playback adapter; other explicit channels remain unchanged.

Held notes crossing the phrase boundary split into editable segments because AppPattern's
existing validator requires every duration inside the phrase. Holds longer than a phrase
are bounded to one phrase of material. Split segments retrigger on subsequent loop playback;
there is no cross-boundary tie identity in AppNote. No quantization is imposed during capture;
existing quantize controls can edit the result afterward.

TransportTick carries absolute tick, phrase tick/index, PPQ, phrase length and a fresh
TransportEpoch per start. Its beat/bar flags use quarter-note beats and 4/4 bars. Tick fires
on sixteenth subdivisions, phrase start, and an explicitly queued scene boundary, before
notes at that musical tick. QueueTransportBoundary inserts the exact absolute due tick;
CancelTransportBoundary removes it. Events run on the worker and callbacks must remain bounded.
The scene host maps this to MusicalPosition(epoch, Tick(absolute), ppq, 4, Tick(length)).
App scene apply/restore, arrangement switching and stepped MIDI CC dispatch are integrated
through the shared transport; atomic hardware application remains future work.

This is an immediate MIDI sender with monotonic deadlines, not measured timestamped lookahead,
external MIDI sync, swing or sample-accurate scheduling. Standalone Sloop.Transport.Tests
covers capture durations/wrap/count-in/replacement, undo, MIDI ownership, actual arp gates,
phase-preserving tempo changes, continuous looping, exact scene ticks and cancellation.
Android compilation and APK packaging succeed with the configured SDK/JDK paths. Earlier
versions were installed on Pixel 7a; the latest follow-up has no connected-device checks.
See VERIFICATION.md for the actual check scope.

### Discoverable sequence recipes — 2026-10-07

A visible Describe a sequence · offline recipes button near the top opens phrase entry plus
four explicit recipe choices: transpose up/down by one semitone, quantize to sixteenths,
and keep every second note. The dialog identifies fixed offline rules rather than conversational
AI. Only those supported phrases are accepted. Operations target the selected edit channel and
use existing AppEditor proposals. Review shows changed notes before Apply locally or Discard;
apply uses the exact captured snapshot, existing stale-proposal rejection, persistence and undo.
The recipe entry is integrated into the combined Android build. Current verification
and the scope of earlier Pixel checks are recorded in VERIFICATION.md.

## FM1 groove bank — October 8, 2026

Select Drums in Sequence and expand FM1 groove bank. On physical protocol-12 SLOOP,
Browse reads the firmware bank, then replacement confirmation applies a starter to the
native drum pattern. The phone app pattern is separate. Stop hardware playback/recording;
busy devices reject replacement. Kit and global tempo stay selected. Hardware Undo restores
the replaced pattern; project adoption invalidates stale undo. Native read/write baselines
are cleared after apply, so read hardware again before editing it from the phone.
Generic MIDI and the limited simulator do not advertise this feature. An uncertain result
is not automatically retried. See [hardware design](../../docs/firmware/DRUM-GROOVES-DESIGN.md).

Firmware 2.4.14 expands the shared bank to sixteen entries, including a four-bar
Amen-inspired pattern. Phone discovery needs no new app code; full native pattern
reads include all 64 steps. The FM1 GROOVE page also offers non-destructive looping
preview with OCT−; this playback control is hardware-local.

## Recorded chords feeding firmware arpeggios — 2.4.15

On a synth track, enable an ARP mode and set ARP 2 ORD to SNOTE or SPLAY. Its native
recorded chord steps feed the generator without an additional direct chord. Literal notes
are used; no root inference or follower tracks are implemented. The live held list remains
separate, with overlapping sequence/live pitches deduplicated in the bounded arp pool.
TIE retains the snapshot; REST and rejected fill steps clear it even with HOLD enabled.
Legacy NOTE/PLAY and ARP OFF keep direct native step playback. Routing shares ORD and
factory sound recall may reset it. Existing generated velocity/timing and MIDI output rules
remain. See the [full contract](../../docs/firmware/CHORD-ARPEGGIO-DESIGN.md).
