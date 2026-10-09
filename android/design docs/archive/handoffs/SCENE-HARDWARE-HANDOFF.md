# Scene hardware handoff — 2026-10-07

Parent integration complete: SceneHardwareEditor exposes baseline Read and explicit stopped-only
three-track RAM Send from Scenes & arrangements. Outcomes and observed sound summaries are
displayed, baselines consumed after attempts, and Unknown blocks transfer until a fresh Read.
Combined APK builds with zero warnings/errors; physical transfer acceptance remains pending.

Implemented in `Sloop.Android/Services/DeviceEditing.cs` and new
`Services/SceneHardwareBatch.cs`; no shared project registration required for the
Android SDK's default compile glob. Parent owns UI, combined builds and deployment.

## Caller contract

Use connection control APIs on the main thread, as with all `Fm1Connection` methods.
Await `StopPlayingAsync()`, finish recording and release performance before reading
or applying. Connect physical FM1 (simulated and generic connections reject).
Explicitly map each scene sound to a distinct synth hardware track 0–2. This API
requires protocol >=9, parameter start 53/count 61 and FM6 selected on every mapped
track; it does not select engines, presets or PTCH for the caller. Native pattern
references, sample uploads, automation and app workspace swaps are outside this API.

1. `ReadSceneSoundsAsync(IEnumerable<int>, CancellationToken)` returns opaque
   `SceneHardwareBaseline`. `Tracks` contains immutable complete patches, seven
   macros and preset/PTCH identity, verified through a second full pass. Baseline
   belongs to this exact connection epoch. Store it with the prepared UI operation.
2. Build `SceneHardwareTarget(track, desiredSoundState)` for every baseline track.
   `ApplySceneSoundsAsync(baseline, targets, CancellationToken)` validates exact
   mapping, capabilities and every desired patch/macro before entering hardware I/O.
   It rechecks every baseline before any write, then each track immediately before
   its write. Writes use existing patch acknowledgment/readback plus seven matching
   parameter acknowledgments and complete sound readback. First failure stops later
   writes. Final reconciliation rereads every mapped track, including earlier writes.
3. Consume every `SceneHardwareTrackResult`. `Applied` means observed complete sound
   equals desired and preset/PTCH still match baseline. `Partial` means a written
   track or externally changed track has a complete observed differing state.
   `Unknown` means post-attempt readback could not establish complete state.
   `NotApplied` means no batch write occurred, or a remaining track is verified at
   baseline. Before any attempted write, stale/cancel/read failure returns NotApplied
   with the last available preflight observation (possibly null) and Error.
   Global `Applied` requires every track Applied. Error may accompany Applied if a
   lost acknowledgment was subsequently resolved by complete readback.
4. Publish observed state for Partial; never publish the intended scene as live on
   incomplete success. Block further restore after Unknown until a fresh complete
   Read succeeds. Obtain a fresh baseline for retries; do not automatically retry
   writes or attempt rollback. Caller owns reconciliation of app-only workspace state.

Admission/mapping/state validation throws before I/O. Once orchestration starts,
operational failures produce the per-track report, preserved across disconnects.
Caller cancellation stops writes; read-only reconciliation intentionally uses the
connection token rather than the canceled caller token. This may take protocol
request timeouts. A disconnected/replaced connection cannot be reconciled and yields
Unknown after any write attempt. Busy is restored only on the original connection.
The existing generic device-edit gate additionally rejects recording now; existing
single-sound fingerprint/readback safeguards remain intact.

This is sequential non-atomic hardware editing. Busy blocks app playback, recording
and performance while executing; each request checks stopped app state. The editor
protocol exposes no reliable hardware playback status/lock or multi-track transaction.
The host/user must keep the physical sequencer stopped and avoid panel/external MIDI
edits throughout the operation. Repeated reads detect observable races but cannot
prevent changes between the final read and the next request, nor identify an ABA
change returning to identical state. Do not advertise hardware atomic support or use
this from a live transport boundary callback.

## Validation

New standalone project `Sloop.SceneHardware.Tests` links the exact pure orchestration
source. `dotnet run --project android/src/Sloop.SceneHardware.Tests/Sloop.SceneHardware.Tests.csproj`
passes 8 meaningful checks: complete application; stale last-track baseline prevents
all writes; lost acknowledgment readback; disconnect with no cached observations;
cancellation reconciliation; preset mismatch stopping later writes; full sound
mismatch stopping later writes; Unknown has no observed state.

No Android APK/build, install, flash, commit or server restart was performed. Parent
must compile the combined Android implementation and test actual protocol/device
behavior. Tests use injected device state/failures, not physical or simulator transport,
and do not establish physical timing, hardware playback locking or USB reliability.
