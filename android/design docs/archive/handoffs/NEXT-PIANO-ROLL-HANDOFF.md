# Piano-roll handoff — October 8, 2026

Implemented app-pattern touch insertion, stable-ID selection, pitch/time movement, duration
resizing, selected deletion, selected quantization, bounded zoom/pan and event protection.
Large rows/mode buttons and release-only Draw prevent scroll drags inserting notes.
Move/Resize operate the selected note from anywhere in the canvas. Native FM1 editing remains
in the existing hardware section. No other owner's files or existing changes were replaced.

## Changed files

- `android/src/Sloop.Android/SequenceEditor.cs`: calls AddPianoRoll; existing step edits and
  track transforms pass pianoRoll.Locks to validated AppEditor proposals.
- New `android/src/Sloop.Android/PianoRollEditor.cs`: activity-owned controls/model and
  existing EditingWorkspace.Run/EditPattern/PatternHistory integration.
- New `android/src/Sloop.Android/PianoRollView.cs`: Android canvas and touch routing.
- New `android/src/Sloop.Android/PianoRollModel.cs`: platform-free gesture/viewport/hit state.
- New `android/src/Sloop.PianoRoll.Tests/{Sloop.PianoRoll.Tests.csproj,Program.cs,
  RunPianoRollChecks.ps1,README.md}`: dedicated package-free runner, outside central runners.
- `android/design docs/SEQUENCING.md` and this handoff.

## Integration hooks

No MainActivity.cs or project/solution changes are required: SDK wildcard compilation includes
all three new partial/view files and the model. AddSequenceEditor calls AddPianoRoll directly.
Final combined Android source compilation/APK packaging remains with the parent chat.

`PianoRollModel.SetLocks(EditLocks)` accepts existing shared part/event locks. `Locks` returns
the current protection set. Parent integration should pass the same set to prompt/composition
proposals and reconcile shared protection on scene/session adoption. The current roll and
legacy SequenceEditor operations already pass it to AppEditor.PutNote/Propose. Do not change
the proposal/history contracts: gestures capture a source snapshot, preview without applying,
then submit exactly one proposal; stale snapshots reject in EditHistory.Apply. Persistence and
Changed refresh use EditingWorkspace.Run(() => editing.EditPattern(proposal), status).

Quantization picker derives tick grids from the current pattern PPQ. The model's default
240-tick grid matches the existing 960 PPQ workspace; hosts can set Grid for other PPQ or
custom divisions. `Reconcile(pattern, channel)` clears missing/wrong-channel selection and
missing event locks and clamps the viewport. Model/view cancellation drops preview; additional
touch pointers suppress commit until final release. View detachment also cancels.

## Validation

Ran `dotnet run --project android/src/Sloop.PianoRoll.Tests/Sloop.PianoRoll.Tests.csproj`:
**38 focused checks passed**, including creation/selection/move/resize/delete/quantize,
cancel and Draw drag safety, hit resolution, preserved IDs/other notes/channels, event and
part locks, phrase/pitch/duration bounds, atomic undo/redo, stale proposals, pan and zoom.
The runner links the exact production model rather than duplicating the implementation.
No APK build/install, firmware build/version/flash, server or hardware test was performed.

## Limitations / remaining work

Android rendering and native MotionEvent routing need the parent combined source compile;
the console runner verifies the touch model and domain behavior, not Android event dispatch.
No further hardware acceptance is requested. Current protection, selection and viewport are
activity-local. Shared protection persistence and propagation to recipes/composition/scenes
require parent integration; a selected note lock is not a global session lock.
Undo/redo are explicit history restoration and can restore protected material.
The roll edits one note at a time. Pinch zoom, multi-selection, velocity gestures, audible
preview and native hardware piano-roll rendering are not implemented. Zoom uses buttons,
with explicit Pan for drag navigation. Off-grid moves keep their original onset offset;
selected quantization explicitly snaps onset. Phrase-edge clipping takes priority over grid.
