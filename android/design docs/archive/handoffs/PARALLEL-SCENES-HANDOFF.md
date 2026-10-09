# Scenes handoff — 2026-10-07

Independent implementation complete. No reserved shell, shared storage, existing editor,
solution, firmware or shared test files were changed. No commit/reset, flashing or APK install.

## Owned files

- `android/src/Sloop.Scenes/`: new project, Models.cs, SceneEditing.cs, Automation.cs,
  Scheduling.cs, SceneStore.cs.
- `android/src/Sloop.Scenes.Tests/`: new standalone console test project and Program.cs.
- `android/src/Sloop.Scenes.Android/`: new Android library project and SceneEditor.cs.
- `android/design docs/SCENES.md` and this unique handoff.

## Exact root integration hooks

1. Register the three new projects in Sloop.slnx. Add a project reference from Sloop.Android
   to Sloop.Scenes.Android (domain reference is transitive). Instantiate
   `Sloop.Scenes.Android.SceneEditor(context, catalog, status, queue, cancel, capture)` in a
   root-owned workspace section. Marshal callbacks from UI to the transport owner thread.
2. Capture existing sound documents as SceneSound target IDs/SoundState values and existing
   AppPattern; use SceneEditing.Capture. Store native refs only for verified device identities
   and revisions. Save SceneCatalog with SceneStore in an app-private host-selected path.
   Session storage should include this catalog or scene IDs using these types.
3. Construct SceneScheduler with a live aggregate revision; external edits invalidate it via
   ObserveLiveRevision. Prepare(scene, liveRevision, mode), then Queue(token, MusicalPosition,
   boundary). MusicalPosition is absolute elapsed ticks within a TransportEpoch, explicit PPQ,
   beats per bar and phrase length. Beat/bar arithmetic assumes quarter-note beats; normalize
   other meter units in the transport adapter. Pattern timing must be converted explicitly.
4. Insert Pending.Due into transport's exact event list. Call Commit at that tick before new
   scene note dispatch. Cleanup outgoing owned notes/unsent events, restore app documents,
   change tempo and re-anchor phase, then Reconcile(token, Applied, detail). A callback after
   Due rejects, so reprepare/requeue rather than applying late. Stop cancels pending token;
   restart/seek uses a new epoch. Queue always means strictly next boundary.
5. HardwareReconciled requires device baseline/readback and native ref resolution. Report
   Partial for verified mismatch, Unknown for disconnected/unverified state. Unknown blocks
   more restores until resolved. Partial requires publishing actual observed app/device state.
   AppOnly changes local documents; sound snapshots must not implicitly send hardware writes.
6. Advance ArrangementCursor once per completed phrase. Determine and prepare the next target
   ahead of the transition boundary, accounting for repeats. This domain cursor is independent
   of MIDI playback. AutomationLane.Evaluate accepts scene-relative ticks; root resolves
   routes, schedules CC/macro writes, deduplicates values and cancels outgoing automation.

## Validation

Build using `--disable-build-servers -m:1 -nr:false` to avoid shared MSBuild server failures.
Domain/test project builds with zero warnings/errors; run the built test DLL directly.
20 executable scenarios cover capture/stale edits, snapshot isolation, reorder/repeats/loop/end,
automation values/ranges, boundaries/cancellation/stale/epoch/late events, hardware rejection,
reconciliation, schema/reference validation and save/replace/load round-trips.
Android adapter library builds with zero warnings/errors using explicit existing paths:
`-p:AndroidSdkDirectory=C:/Users/shaun/AppData/Local/Android/Sdk`
`-p:JavaSdkDirectory=C:/Users/shaun/AppData/Local/Android/Jdk`.
No phone or physical FM1 testing performed. Full shared-shell integration is root-owned.

## Proposed firmware work (not implemented)

Provide a capability-advertised scene transaction: bounded staging with target/device revision,
validated complete sound/macro/pattern data, explicit commit at device musical boundary,
transaction ID acknowledgement, cancellation before commit, and queryable final transaction
status after disconnect. Define lock/pattern/sound conflict handling and all-or-nothing rollback.
Current multi-command FM1 updates cannot make this guarantee; HardwareAtomic rejects even
if a caller assumes support. A future capability-aware adapter/API revision must prove support
before enabling atomic hardware scene behavior.
