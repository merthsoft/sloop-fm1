# Sessions integration — 2026-10-07

Implemented writable `WorkspaceGenerationStore` independently of immutable Library snapshots.
Legacy workspaces remain active until first adoption. Every preparation copies assets into a
new owned directory; validated prepared generations activate through same-volume pointer
replacement with flushed current/previous pointer files. No prior generation is collected.
Startup resolves the active pointer and recovers malformed/missing generations through the
previous pointer. Startup validates the complete active generation before either editor singleton
is constructed; corrupt editor/kit/settings/scene content recovers the complete previous generation
or the retained legacy workspace. Stale scene/session preparations cannot supersede a newer active generation.

`EditingWorkspace.Prepare(root)` and `SampleWorkspace.PrepareAsync(root)` load independent
candidate models without mutating live state. Their `Adopt(prepared)` switches persistence paths
and in-memory state without deserialization/copying. Sample preparation validates original WAV,
edit boundaries, kit settings and calculates peaks before activation. Converted caches and
pending exports are invalidated on adoption. Default empty sessions clear old sample/native data.

`MainActivity.InitializeSessionIntegration()` enables Library load callbacks and restores settings.
Call after assigning editing/samples and before initial UI rendering. Session load stops owned
performance and awaits transport shutdown; preparation failure does not change editor state.
`PrepareWorkspaceGenerationAsync(WorkspaceGeneration)` lets scenes prepare outside transport;
`CommitWorkspaceGeneration(PreparedWorkspace)` performs the fast boundary activation without
stopping transport or rebuilding views. Root posts UI notifications afterward.

Scene hooks: `PrepareSessionExtrasAsync(root)` validates/stages catalog objects and returns opaque
prepared state; `AdoptSessionExtras(value)` must be nonthrowing and perform only in-memory swaps.
`CaptureSessionExtraAssets()` and `CaptureSessionSceneIds()` include portable catalog assets in
named saves. Call `SaveSessionSettings()` before staging a current-workspace scene, preserving
all current default documents, sequence tempo, performance options and output/edit channels.

Library uses the effective generation for workspaces/imports/settings/current sample. FM1 slot
backups and immutable named snapshots remain outside generations. Imported performance presets
are usable directly from the active generation and included in subsequent snapshots.

New standalone `Sloop.SessionIntegration.Tests` requires root solution registration. Its 19 checks
pass: staging isolation, durable restart activation, rejection of unprepared/stale/external stages,
previous live edits retained, pointer recovery/repair, empty-session activation and prior generation
retention and whole-generation malformed-content recovery. Android managed compilation passes
with zero warnings/errors; packaging/installation remains root-owned. Physical low-storage/power-loss
tests and Android multi-editor interaction are acceptance work; filesystem unit tests do not claim
those guarantees. Pointer replacement durability depends on platform filesystem guarantees.
