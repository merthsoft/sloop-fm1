# Sloop.Sequencing

Standalone .NET 10 domain library. No Android, Core, Protocol, inference, audio,
MIDI scheduling, or device-send dependencies. Companion executable tests use no
NuGet packages. From the repository root:

```powershell
dotnet build android/src/Sloop.Sequencing/Sloop.Sequencing.csproj
dotnet run --project android/src/Sloop.Sequencing.Tests/Sloop.Sequencing.Tests.csproj
```

If the sandbox cannot read the user's NuGet configuration, run the isolated
`android/src/Sloop.Sequencing.Tests/Verify.ps1` instead. It restores both projects
with an empty package source configuration and runs the tests.

## Model and representation

`AppPattern` uses an explicit PPQ (960 recommended), tick length, immutable
notes, part IDs, and zero-based MIDI channels. Notes have independent onset,
duration and velocity. MIDI pitch 0..127, velocity 1..127 and channel 0..15 are
validated. Notes must end inside the phrase. Tick/frame ranges are half-open;
app selection ranges select **onsets**, not every note whose tail overlaps.
`TickRange.Overlaps` is available for an explicit overlap-selection UI. Empty
selections are safe no-ops; unknown selected note IDs reject.

`HardwarePattern` stores three synth tracks and the fourth drum track. All 64
physical steps remain present even when a track has a shorter active length.
Each synth step retains all four slots, active count, NOTE/TIE/REST, accent/slide,
shared velocity (including native zero fallback), per-slot native level and
ratchet. Each drum step retains all 16 lanes, including inactive level/ratchet
bits. Level enum values are the firmware's two-bit codes; ratchets use musical
counts 1..4 (wire stores count minus one). MicroOffset is signed native 1/64-step
units, -32..31. Fill code 3 is retained as ReservedNormal. Track parameters and
all parameter locks remain in the snapshot. No simplified piano-roll conversion
is used. Unused firmware lock slots are storage padding, not domain events; a
future codec may need its own raw backing image for exact padding-byte export.

IDs are application-owned stable identities, not firmware addresses. Generate
IDs once on initial ingestion; reconcile readbacks by track/step/lane or slot
and keep identities on pitch, timing and level edits. A panel replacement cannot
always be identified from legacy protocol alone; the adapter must resolve that
ambiguity. Move keeps step/note/hit/parameter-lock IDs. Duplicate generates new
IDs for every copied event; original identities remain unchanged. Removed event
identity and all fields are retained in the proposal's Before snapshot and undo.

Limits were checked against `firmware/src/core.h`, `seq.c` (`p_lockable`), and
`web/EDITOR_PROTOCOL.md`. The checked-out firmware adds TFLT=50, STRUM=51 and
VLEAD=52, placing engine parameters at **53..60**. The prose protocol's older
50..57 engine range must not be hard-coded into adapters. Supply a
`HardwareCapabilities` snapshot per track containing the device/firmware/engine
identity, exact lockable IDs and engine-specific parameter ranges. The library
provides `CurrentFirmwareLockable` for this checkout; missing lock ranges reject,
values reject instead of clamping, and capacities are 24 locks per track with
one per (step, parameter). Do not reuse the current profile for old firmware.

## Editing and transactions

Use `AppEditor.Propose` with typed `AppEdit` factories for move, resize,
transpose, duplicate, nearest-grid quantize (half-grid rounds forward), velocity
**delta**, and seeded thinning variation. Tick-bearing operations require `Tick`.
Variation retains every Nth selected event in document order, with the seed as
phase; it is a deterministic rule recipe, not generative inference. Reuse a stable
source ordering for refinement. Pitch, duration, part, channel and unaffected
notes remain unchanged under rhythmic edits. App duplicate's ticks are an offset,
not an absolute insertion position.

Use `HardwareEditor.Propose` and `HardwareSelection`. Move and duplicate require
`StepOffset` and whole steps, carry locks and all native fields, reject overlap,
occupied destinations, active-length overflow, and crossing/bordering tie or slide
chains (including loop wrap). Synth transpose edits active slots; velocity edits
the shared step velocity. SetLevel edits active synth slots or selected active drum
lanes. QuantizeMicro sets the whole step's micro offset to zero. Drum variation
only removes selected active hits and preserves inactive native bits; other lanes,
locks, fill and micro timing stay identical. A per-hit EditLock also protects that
hit from shared timing changes. Locked parts/events reject a transaction that
would change them; there is no partial apply or silent skipping.

Independent hardware note resize, drum MIDI velocity/pitch changes, lane-only
move/copy/micro edits, and synth thinning reject with representability explanations.
Use app mode for independent durations, or build an explicit validated step/tie
editor later. Current variation is thinning, not fills or reharmonization.

`EditHistory` owns one immutable Current snapshot on one application event loop.
A proposal captures immutable Before/After, typed NoteChange or complete TrackChange
records, recipe version, optional prompt and seed. Audition/sending are separate.
Apply requires the exact captured parent snapshot and validates the result.
Undo restores **all** content and identities; redo replays accepted content without
rerunning a recipe. Each restore gets a fresh revision so pre-undo drafts stay stale.
A new accepted edit clears redo. Compose a contiguous proposal chain with
`EditProposal.Compose` to group a gesture or compound edit into one atomic undo entry. No-change app proposals do not enter history.
`AcceptedHistory` exposes ordered accepted entries for a future persistence adapter.
Persist the complete result and provenance, not merely a seed. History is in-memory;
there is no storage format or automatic hardware undo in this slice.

## Integration coordination

Add references from future consumers to this csproj; adding these projects to the
shared solution is intentionally left to the integration owner. Do not copy these
models into Sloop.Core: agree on a shared identity/tick abstraction first, then use
an adapter or a reviewed extraction if sampling/routing needs common frame types.
The frame wrappers here are units only, not a sample editing implementation.

Protocol integration must retain native fields and use complete drum reads (legacy
four-note drum views lose lanes), fetch micro/fill/locks, and refresh other-track
recording data explicitly. Wire adapters must account for each firmware parameter
layout and parameter range. Document intent, acknowledged hardware snapshots and
pending writes need separate owners. Local atomic proposals do not make multiple
wire writes atomic; report partial application and reconcile readback in the device
session. No app/services, shared solution, firmware, simulator or design docs were
changed by this project.

