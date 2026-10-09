# Parallel implementation assignments — 2026-10-07

Completed and integrated. This file retains the original ownership boundaries; current
behavior and verification are in [PARALLEL-INTEGRATION.md](PARALLEL-INTEGRATION.md).

Four local chats work in the shared checkout. Preserve all existing/uncommitted work; do not
commit, reset, delete or reformat unrelated files. Each chat owns the files listed below.
The originating chat owns final cross-component integration and shared shell changes.

| Chat | Owns | Outcome |
| --- | --- | --- |
| Loop transport and performance recording | New transport/recording domain and service files; PerformEditor.cs, SequenceEditor.cs; playback portions of Services/DeviceEditing.cs; SEQUENCING.md, PERFORMANCE.md | Continuous app-pattern looping, transport controls, capture performed notes into editable patterns, cancellation and note cleanup |
| Sampling zoom and smart chopping | Core/Sampling/SampleDocument.cs; SampleEditor.cs; sample service/editor files; new sampling tools; SAMPLING.md | Waveform zoom/navigation, move/remove boundaries, transient proposals, precise slice editing with undo and immutable originals |
| Sessions and performance presets | New session/preset domain/storage/service files; LibraryEditor.cs; STORAGE.md | Named sessions and portable archives with assets, safe load/save, performance presets, useful Library browser |
| Scenes and arrangement foundations | New scene/arrangement/automation domain and persistence files; new independent SceneEditor.cs; scene-specific design document | Scene snapshots, chains, musical-boundary scheduling contracts, explicit restore/reconciliation semantics; reusable adapter for transport integration |

Reserved for the originating chat: MainActivity.cs, WorkspaceStyle.cs, Fm1Connection.cs,
SoundEditor.cs, Fm6Visuals.cs, new FM6 editing helpers/tests and FM6-EDITOR.md/SOUND.md,
PerformanceState.cs, LivePerformance.cs, EditingWorkspace.cs, WorkspaceFiles.cs, existing
shared test Program.cs files, Sloop.slnx, global docs/index and firmware. Request specific
hooks through a handoff document rather than changing these concurrently. New standalone
test projects are allowed; root integration will register them in the solution.

Transport and scenes expose compatible boundary/tick contracts; sessions store scene/preset
references without inventing competing scene or performance models. Sampling preserves current
conversion/upload protocols and routing. Session archives must reject path traversal, bound
resource usage and restore through staging before replacing live work. No chat should flash
firmware or install competing phone APKs; root integration owns device deployment.

Each chat reads relevant design docs, implements its owned slice, runs meaningful tests,
updates its owned docs and writes a separate PARALLEL-<SLICE>-HANDOFF.md documenting APIs,
changed files, validation and exact integration hooks. Clearly distinguish completed behavior
from planned features and untested physical hardware. Finish independent work before handing
off integration requirements. Do not add package/model downloads or new external services.
