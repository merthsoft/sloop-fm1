# Next implementation wave — October 8, 2026

Integration checkpoint: five Android slices are integrated, including persistence, shared
protections and audition lifecycle recovery. Hardware scene transaction scaffolding is
validated but compile-disabled pending complete engine hooks. See WAVE-INTEGRATION.md.

Hardware testing is accepted by the user for now. Read STATUS.md and the relevant component
design before editing. Implement useful complete slices; update your component design with
actual behavior, tests and limitations. Existing uncommitted work must be preserved.

## Coordination

All chats share C:\code\sloop-fm1. Each owns the files below and may add dedicated new files.
Do not edit MainActivity.cs, shared session integration, solution files, central test runners,
STATUS.md or this ownership document. Provide exact integration hooks in your handoff.
Do not run Android APK builds/installations, firmware target builds, flash, start servers or
change firmware version: the parent chat owns final integration and those serialized actions.
Run focused domain checks; separate new tests avoid concurrent central-runner changes.
Avoid editing another chat's files; identify dependencies in the handoff rather than stubbing
another owner's implementation. Existing public contracts remain compatible where possible.

## Chats and first deliverables

| Chat | First deliverable | Owned existing files / design |
| --- | --- | --- |
| Sampling | Pitch/root estimate with confidence and manual override; per-chop gain/tuning carried through preview and kit preparation | SampleEditor.cs, KitEditor.cs, Services/SampleWorkspace*.cs, SampleTools.cs, SampleKit.cs, Workstation/SamplePreparation.cs, SampleKitPlan.cs, SAMPLING.md |
| Piano roll | Touch app-pattern note creation/select/move/resize/delete, zoom/pan, quantization, shared undo/protected-note handling | SequenceEditor.cs, dedicated PianoRoll*.cs; SEQUENCING.md. Do not alter PatternPromptEditor.cs or existing sequencing domain files; add dedicated helpers if needed |
| Offline composition | Deterministic prompt-to-editable loop drafts with seed/key/scale/bars/parts, review/apply/regenerate and existing history integration | New composition files/project as needed, PatternPromptEditor.cs, COMPOSITION.md. Avoid SequenceEditor.cs and existing shared sequencing domain files |
| Perform controls | Persistable configurable chord/joystick mappings and bounded XY MIDI CC macros with reliable touch ownership/cleanup | PerformEditor.cs, dedicated performance files, Workstation/Performance.cs, PERFORMANCE.md. SessionPresetCodec.cs remains parent-owned; hand off serialization additions |
| Sound audition | Reversible hardware FM6 A/B audition, acknowledged original baseline, safe Restore/Keep, disconnect/failure reconciliation | SoundEditor.cs, dedicated SoundAudition*.cs/services; SOUND.md and AI-SOUND.md. Avoid shared Fm1Connection.cs; supply parent hooks if needed |
| Hardware scene transaction | Design and implement a bounded negotiated RAM transaction for synth sounds/patterns/tempo, prepare/commit/cancel/status and boundary application; sample dependencies reported explicitly | SceneHardwareEditor.cs, Services/SceneHardwareBatch.cs, dedicated scene transaction files; firmware/src/usb.c and dedicated firmware transaction files, protocol domain extensions; SCENES.md and FIRMWARE.md. Do not edit firmware ui/version or other app editors |

The hardware-scene chat must audit RAM/flash budget before selecting payload/queue sizes,
retain old stopped-only fallback, and avoid claiming sample flash uploads are atomic live swaps.
Free-form language-model inference and full local audio rendering remain later independent work.

## Launched chats

- Sampling pitch and chop controls: `01a11c34-9d39-7893-b900-8dd5f1ec05a8`.
- Touch piano roll: `01a11c34-cd44-7bd3-b14c-e4101424cfdf`.
- Offline prompt loop composition: `01a11c35-05f2-7cd2-a90a-8455bd5f25a6`.
- Perform mappings and XY macros: `01a11c35-b773-7b22-a8de-110c60b4c1a2`.
- FM6 hardware A B audition: `01a11c35-c679-7002-bac5-8f5af320a954`.
- Atomic hardware scene transactions: `01a11c35-dccb-7660-9237-e8efecb87d77`.

Created against the same local project checkout. Parent owns shared integration and final builds.
