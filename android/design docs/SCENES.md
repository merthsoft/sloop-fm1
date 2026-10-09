# Scenes and arrangement foundations — 2026-10-07

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

## Hardware transaction staging checkpoint — October 8, 2026

Experimental command 72 implements bounded negotiated RAM staging, prepare/commit/cancel/status,
CRC, epoch/token identity, and exact-boundary lifecycle checks for three FM6 synth
sounds, complete native synth steps/micro/fill/locks and tempo. Schema is 2,956 bytes;
single staging allocation is 3,072 bytes, chunks 96 bytes, maximum frame 125 bytes.
See `../src/Sloop.Protocol/SCENE-TRANSACTION.md` for the exact wire contract.
Host preparation explicitly rejects missing/unsupported sample dependencies; sample
flash uploads and drum-track replacement are not part of this operation.

This is implemented staging, not an integrated live hardware scene operation.
Production compiles this extension out (`FELUCCA_SCENE_TX=0`) and remains protocol 10.
Experimental capabilities stay zero until complete engine hooks are installed. Existing
stopped-only transfer and recovery remain available. [Firmware requirements](FIRMWARE.md#atomic-hardware-scenes-remaining-requirements) define the remaining work.

## Scene management follow-up (existing integration)

Sequence → Scenes & arrangements → Manage scenes supports rename, duplicate, recapture
current sounds/pattern/tempo, and confirmed deletion. Duplication gives the scene and its
automation fresh identities; arrangement references remain on the original. Recapture keeps
the scene identity and rejects incompatible automation when the replacement phrase is shorter.
Deletion removes associated automation and rejects scenes still used by arrangements.

Arrangement editing now lists individual steps with explicit phrase repeats (1–1024),
move earlier/later and remove actions. Arrangements can be deleted without deleting scenes.
Catalog edits invalidate prepared transitions. Preparation requests use a cancellation version
so superseded asynchronous staging cannot apply after cancellation or a catalog edit.
The scene suite now passes 26 scenarios, including reference preservation, stale edits,
independent automation duplication and safe deletion.

## Explicit FM1 sound transfer

Scenes → Send scene sounds to FM1 is integrated. Read verifies complete baselines for fixed
Synth 1→1, 2→2, 3→3 mappings. Choose scene sounds to send confirms three RAM replacements;
the adapter rechecks baselines, writes patches/macros sequentially and rereads all tracks.
The UI reports Applied/NotApplied/Partial/Unknown with observed patch names/algorithms.
Every attempt consumes its baseline; another Read is required before retry. Unknown blocks
transfer until that succeeds. Pattern, samples and tempo are not sent by this operation.
Generic MIDI and simulator modes do not expose it. The physical FM1 must already use FM6
on all destinations and its sequencer must be stopped. There is no hardware playback lock
or atomic live-boundary sound transaction. Eight injected fault checks pass; the user
considers hardware acceptance complete for now. See [VERIFICATION.md](VERIFICATION.md)
for adapter details and verification limits.

Implemented independently in `Sloop.Scenes` (.NET 10) and `Sloop.Scenes.Android` (native
Android view library). Shared shell and transport integration are complete: Sequence →
Scenes & arrangements captures and restores app scenes, queues beat/bar/phrase changes,
builds/plays ordered arrangements and creates MIDI CC sweeps. See
[verification record](VERIFICATION.md) for verification and limits.

The host stages a complete writable generation and validates all editor/sample/settings/catalog
state before queuing. At a due tick the worker waits for a short UI-owned pointer/state swap;
the loop then releases outgoing notes, discards old events and starts the new pattern at that
absolute tick. Tempo re-anchors without resetting the transport epoch. Phrase boundaries use
the current phrase origin, including after a mid-phrase switch. Stopping cancels pending work.
Busy sample/device work or recording that starts after preparation rejects the switch.

Arrangements stage their unique scenes before starting, honor ordered repeats, loop or finish,
and switch at completed phrases. Patterns must share PPQ. App sound snapshots do not change
hardware sounds automatically; native-pattern/hardware macro/atomic restores remain future
adapters. MIDI CC automation uses explicit channels, evaluates at sixteenth-note ticks and
deduplicates values. Smooth audio-rate automation and measured scheduling jitter are pending.

## Snapshot and editing

`SceneSnapshot` stores scene identity/revision/name/tempo, named target `SoundState` values,
an optional complete immutable `AppPattern`, and native pattern references retaining existing
pattern identity/revision plus device/bank/slot. It is neither a session nor a performance
preset. Native references contain no device pattern bytes; integration must resolve and verify
them before restore. Sound macros remain the existing `MacroContext`, including its ranges.
`SceneEditing.Capture` copies enumerable inputs into immutable collections. Replace preserves
scene identity, checks the expected revision, and generates a fresh revision. Arrange steps
by immutable records or `SceneEditing.Move`; repeats and scene references validate.

`ArrangementCursor` advances once per completed scene phrase, honoring repeats, ordered steps,
finite completion and optional looping. It does not send MIDI or operate the hardware clock.
Hosts must queue the next scene early enough to enter its desired boundary; do not call Advance
after the boundary and expect a same-boundary queue to take effect retroactively.

## Scheduling and restore

`SceneScheduler` is owned by one transport thread. Prepare validates content and captures the
expected live revision; a new prepare supersedes the prior pending request. Queue chooses the
strictly next beat/bar/phrase boundary. Commit accepts only its exact absolute tick, rejects
late callbacks, and invalidates queues on changed epoch, PPQ, meter or phrase length. Transport
must insert a due-boundary event into its event schedule; polling that can skip ticks is unsafe.
Stop/seek/restart should cancel and establish a fresh transport epoch.

Commit is permission to execute a restore, not evidence that it succeeded. Before changing
patterns, stop outgoing owned notes and invalidate outgoing automation. Swap app pattern,
tempo and app-owned sound document state as one operation, re-anchor transport timing, and
report Applied only after success. AppOnly does no hardware writes. Native references reject
AppOnly. HardwareReconciled allows non-atomic device work, requiring baseline verification,
readback and explicit Applied/Partial/Unknown reconciliation. HardwareAtomic always rejects.
Unknown keeps the restore blocked until readback resolves it; partial results invalidate the
old live revision and require the host to publish observed state, not the intended scene.
Cancellation cannot undo a committed restore. External edits call ObserveLiveRevision before
another prepare and invalidate pending work. Reconcile app-only failures too.

## Automation and persistence

Automation lanes use explicit ticks, ordered unique points and Hold/Linear interpolation.
Before the first point no value is emitted; after the last point its value holds until phrase
end. Lane evaluation does not loop implicitly. Targets are MIDI CC or the seven existing sound
macros (index order matches MacroContext constructor). Hosts resolve target routing, deduplicate
unchanged values, enforce capabilities, and decide which timestamped events to dispatch.
No timestamp sender, smoothing, automation recorder, tempo lane or parameter-lock converter is
implemented here. Automation is catalog metadata keyed by scene identity, not a competing pattern.

SceneStore uses a versioned source-generated JSON catalog for Android trimming compatibility,
validates references/content, bounds files to 16 MiB, and saves via flushed sibling staging
and replacement. Paths are host-selected; no asset imports, session archives or shared storage
mutations are included. Scenes/arrangements are limited to 1024 each; automation entries to 8192.

## UI and validation

SceneEditor is a standalone compact LinearLayout with boundary selection, scene queue buttons,
capture/cancel callbacks and arrangement summaries. The host supplies status and handles
callbacks; arrangement editing and automation drawing are currently domain operations only.
All 20 standalone executable scenarios pass and cover snapshots, editing, ordering/repeats, automation, boundary scheduling,
stale/cancel/epoch/late events, restore reconciliation and persistence. Native Android view
compilation is verified with zero warnings/errors, as are the domain and test project builds;
the combined APK is installed, and user hardware acceptance is complete for now. See
[verification record](VERIFICATION.md) for project registration, host callbacks,
transport boundary requirements and separately proposed firmware work.
