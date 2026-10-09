# Shared core and application state

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

## Responsibilities

Own musical intent, editable documents, validation, undo, and application coordination.
Keep platform handles, Android activities, audio buffers, and raw firmware structs outside
the domain model. The existing WorkstationState is navigation scaffolding, not a device model.

## Implemented sequencing foundation

[Sloop.Sequencing](../src/Sloop.Sequencing/README.md) owns independent platform-neutral
`AppPattern`/`AppNote` and `HardwarePattern`/`HardwareTrack` models, typed units, selections,
hard edit locks, validators, editors and transaction history. These concrete domain APIs
sit alongside Core; they do not implement the complete generic document families below.

Immutable proposals capture full before/after content, typed diffs and provenance. Apply
checks the exact parent snapshot; composed proposals group a gesture into one undo entry.
Undo/redo restore all content and stable identities with fresh revisions; redo replays the
stored result. Domain history has no device effects; Workstation supplies persistence. See
[SEQUENCING.md](SEQUENCING.md) for supported edits and representation limits.

The library references neither Core nor Protocol and is included in the shared solution.
Android consumes it through Sloop.Workstation. WorkspaceFiles persists app/native patterns
and sound documents; EditingWorkspace saves candidate history before replacing live state.
HardwarePatterns and Fm1Operations handle read/compare/write/readback separately from local
history. LiveNotes and PerformancePlayer own overlapping notes and cancel timed generations.
Frame wrappers are units only, not sample editing. Local intent, hardware baselines and
pending operations remain separate; local atomic undo does not make wire writes atomic.

## Implemented scene foundation

`Sloop.Scenes` references the existing Sequencing and SoundDesign domains. `SceneSnapshot`
retains typed `SoundState`, optional `AppPattern`, and native pattern identity/revision references.
`Arrangement` and `ArrangementCursor` implement ordered repeats and loop/completion behavior;
automation lanes evaluate typed tick points. `SceneCatalog` persists these models with validated
references and bounded source-generated JSON. Use these concrete types for scene integration
rather than introducing another session/preset or generic SceneDocument representation.

`SceneScheduler` exposes Prepare, Queue, Commit, Cancel and Reconcile on one transport owner
thread. Exact boundary events and live revisions guard transitions; device results remain
explicitly Applied, Partial or Unknown. HardwareAtomic rejects because current FM1 writes are
not atomic. See [SCENES.md](SCENES.md) and [verification record](VERIFICATION.md).

## Proposed model families

| Model | Contents |
| --- | --- |
| SessionDocument | Schema version, stable ID, name, tracks, scenes, arrangements, assets |
| HardwareSnapshot | Device identity/capabilities, globals, tracks, samples, transport revision |
| TrackDocument | Stable ID, destination, sound, pattern, mixer state |
| PatternDocument | Timing basis, length, note/drum events, locks, conditions |
| SampleDocument | Source asset, frame ranges, edits, slice mapping and conversion settings |
| SceneDocument | Broader target concept; implemented SceneSnapshot/SceneCatalog own current snapshots and references; mixer/sample dependency expansion remains pending |
| ConnectionState | Discovery/open/handshake/sync/ready/recovery/failure state |
| OperationState | ID, phase, progress, cancellability, error, affected resources |

Use MIDI note numbers 0..127; channel representations are zero-based internally with named
conversion at UI/wire boundaries. Never mix frame counts, milliseconds, beats, and firmware
step indices as unlabelled integers. Frame positions use 64-bit integers and half-open ranges.
Musical positions use integer ticks with an explicit ticks-per-quarter (initially 960).
Hardware micro timing retains its native discrete units; conversion must be explicit.

## State ownership and editing

Separate three states: local document intent, last acknowledged device snapshot, and pending
operations. A slider can display a pending value without claiming it is applied. Device replies
replace it with the clamped value. Offline edits remain local until an explicit apply decision.
Hardware updates invalidate only affected regions; project reload invalidates the full snapshot.
Keep selection local unless an operation explicitly requires changing hardware selection.

Use typed commands such as EditStep, SetParameter, MoveSliceBoundary, and QueueScene.
Validation happens before effects. A command returns document changes plus requested effects;
coordinators execute effects through services and fold replies back into state.
Undo groups one continuous gesture into one edit. Hardware undo is an acknowledged inverse
operation, not a rollback of firmware flash. Do not expose undo for destructive uploads as
instantaneous; restoring a backed-up slot is a separate transfer.

## Service boundaries (proposed)

- IMidiTransport: enumerate/open, timestamped receive/send, disconnect, dispose.
- IDeviceSession: handshake, typed requests, snapshot updates, operation status.
- IAudioService: routes, capture session, preview voices, counters and actual format.
- IClock: monotonic time and conversion metadata; never wall-clock scheduling.
- ISessionStore: load/save/recover versioned documents and assets.
- IFileExchange: platform-selected import/export streams.
- IPerformanceSink: timestamped owned-note and control output.

These are design names; stabilize signatures during the first implementation slice rather
than adding empty interfaces for all future features now. Operations support cancellation
only where cancellation can be honored safely. Disposal must stop producers before consumers.

## Concurrency

One application event loop owns editable state. Worker jobs return immutable results with
document/revision IDs; discard stale results. UI receives snapshots on the UI thread.
Protocol serialization, MIDI scheduling, audio capture, DSP analysis, and file writes have
independent execution paths. No locks shared with UI on real-time callbacks.
Bound queues; report overflow rather than letting memory grow indefinitely.

## Acceptance

Core builds without Android. Validation catches device limits before transfer. A failed reply
does not mark intent applied. Reconnection does not replay stale commands. Long recordings
and edits use 64-bit frame positions. Undo of a gesture restores the previous document state.
