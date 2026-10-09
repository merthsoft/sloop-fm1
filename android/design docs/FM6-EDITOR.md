# Manual FM6 editor on Android

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

UI refinement checkpoint (2026-10-07): Android groups patch files/templates, prompt proposals, device exchange, operators, globals and macros into expandable sections. Operator editing splits tone, envelope and keyboard/velocity response and highlights the selected operator; all existing fields remain reachable.

Status: manual operator/global controls, macros, factory templates, SysEx import/export,
local history and prompt proposals are implemented. Algorithm diagrams, draggable operator
envelope levels, operator copy/swap and direct numeric entry are integrated (2026-10-07).
Manual editing works independently of optional AI.

## One patch, two ways to edit

The manual editor and prompt designer use one validated patch document and shared undo history.
Users can start from an init/factory/user patch, edit entirely by hand, import a DX7 voice,
or generate a draft and then adjust it. AI is an optional editing tool; loading a model or
entering a prompt is never required to reach any supported patch parameter.

An accepted prompt change appears in the same operator controls and algorithm view.
Manual changes create a new revision and invalidate/rebase older prompt proposals.
Locks constrain prompting without disabling ordinary manual edits; distinguish locks from
any user interface edit-protection feature.

## Android layout

Current UI uses operator selection with 21 native fields per operator, 19 global fields,
seven macro sliders, carrier/modulator role labels, factory picker and ten-character patch
name. SysEx bank import selects a voice; export currently emits a single voice. Sound has
visible Edit sound / Describe a sound navigation; prompts are deterministic offline recipes,
not a conversational model. Operator reset/solo, calibrated envelope timing, full-name
metadata UI and increment/decrement buttons remain targets.

The algorithm diagram decodes the firmware's render-order bus flags for all 32 algorithms,
shows carrier paths to AUDIO, and supports operator selection by tapping nodes or numbered
buttons. A track algorithm override takes precedence in the diagram and role labels; changing
the base patch does not silently clear that override. Feedback markings describe the current
renderer: it applies self-feedback where both flags are present with no input bus. Algorithms
4/6 have no invented multi-operator feedback loop; FBOUT alone is unused by this renderer.

Envelope handles edit L1–L4 in native 0–99 units, with L4 also drawn as the starting level.
Each completed drag creates one persisted, undoable edit; cancellation restores the preview.
Stage spacing is schematic and does not represent duration. Numeric labels open a bounded
whole-number entry dialog. Copy/swap replaces entire operators, preserves other operators,
globals and macros, and uses the existing document history/proposal invalidation.

Validation: Sloop.Fm6Editing.Tests passes 2,891 checks against the firmware routing table,
carrier masks, bus edges, renderer feedback, all operator copy/swap pairs, byte-preserving
envelope edits and rejected ranges. Existing sound-design tests pass 1,327 assertions.
Android APK builds with zero warnings/errors. Installed and cold-launched on Pixel 7a:
Describe a sound opens the prompt field directly; algorithm/carrier and envelope drawings
were visually reviewed, copy destination choices are reachable, numeric algorithm 0 is
rejected while preserving algorithm 5, and an L1 drag updates the displayed native value
99 → 53 → 99. Physical FM1 listening and copy/swap readback remain required.

Sound -> FM6 opens an overview with patch name, local/device state, algorithm graph,
six operator level controls, carrier/modulator labels, and access to operator/global pages.
Use the firmware topology to draw actual connections and feedback, with OP1–OP6 display
labels despite OP6-first patch storage. Changing algorithms updates the roles immediately.
Algorithm browsing should support comparison/undo, not repeatedly destroy the original patch.

Selecting an operator opens a page with:

- Output level, ratio/fixed-frequency mode, coarse/fine frequency and detune.
- Its four envelope rates and four levels, with a visual envelope and precise numeric entry.
- Keyboard breakpoint, left/right depth and curves, and rate scaling.
- Velocity and amplitude-modulation sensitivity.
- Copy/paste operator settings, reset from a known baseline, and prompt locks.

The envelope view edits the actual DX-style fields. Rates are nonlinear and are not ordinary
ADSR durations; show native values first and calibrated approximate timing only when validated
against this engine. Frequency displays must use firmware-compatible ratio/fixed-frequency
conversion rather than a guessed mapping. Invalid numeric input does not reach the device.

Global pages expose algorithm, feedback, oscillator key sync, pitch envelope, LFO speed/delay,
pitch/amplitude depth, LFO sync/waveform/sensitivity and transpose. Keep the full local name
separate from the ten-character device patch name. Numeric fields have accessible slider,
increment/decrement and direct-entry alternatives; gestures are not the only editing path.

## Patch macros and other sound settings

Display FM6's eight EDIT parameters alongside the base patch: ALG, FB, MLVL, MRAT, MEG,
VMOD, DTUN and PTCH. They can change what a base patch sounds like. Offer an explicit neutral
macro baseline for editing, and preserve/restore the previous macro context when requested.
PTCH loads another patch and is not a harmless continuous tone control.

The engine uses operator envelopes for amplitude. Generic track ADSR/ENV DEST must not be
presented as working FM6 amplitude controls. Other applicable track controls and effect sends
remain accessible but distinct from the base voice. Shared effect changes are clearly scoped.

## Editing and audition

Local edits are immediate and undoable. Coalesce hardware updates at a bounded rate and keep
one protocol request outstanding; a full patch travels in FM6_PUT rather than invented
operator commands. Validate locally, await acknowledgment and read back canonical state.
Show pending/acknowledged/disconnected state, not merely slider motion. Preserve the local
patch after disconnect. An uncertain send requires readback before declaring hardware state.

Provide keys/chords and a selectable audition phrase with velocity and register controls.
On FM1, audition sends owned MIDI notes to the selected synth part, so the instrument generates
the sound directly. Release owned notes when switching patch/engine or leaving the instrument
surface. A/B compares whole patches and macro context. Without hardware, patch editing still
works; audible FM6 preview needs the separate native firmware renderer.

Operator mute/solo are audition tools only if accurately implemented: output-level changes
alter modulation as well as carrier volume. Do not claim an isolated audible modulator when
the algorithm has no direct carrier path. Snapshot and restore temporary audition changes;
never bake them silently into a saved patch.

## Library and persistence

Load init, firmware factory patches, local patches and device bank entries. Import single
DX7 voice or bank with checksum/length/range validation and selection before applying.
Export local voices/banks with correct format/checksum and original-file preservation.
Local saves do not require FM1 or write flash.

Apply to track updates RAM. Store in FM1 bank is a separate destination operation and obeys
the firmware's stopped-song rule. Projects keep PTCH, not arbitrary RAM patches; after bank
save, associate the correct selector when the user intends project persistence. Warn about
unsaved RAM edits before a preset/project/PTCH load that would replace them.

## Implementation ownership and dependencies

The parallel sound-design task can own Sloop.SoundDesign and its tests: typed voice/operator
model, 155/128-byte codec, validation, golden fixtures, recipes and constrained refinements.
Android integration owns views, gesture coalescing, connection state and audition.
Sloop.Workstation.Fm1Operations owns the integrated FM6 GET/PUT/readback adapter over
Sloop.Protocol.EditorClient. EditingWorkspace owns persistence/proposal application;
Android SoundEditor owns the controls. Bank LIST/read/save UI is implemented through SoundBankEditor
and the existing commands 68–70. Erase UI and generic user-preset storage remain future integration.

Bank saving reviews B1–B27, a 1–10-character ASCII name and occupied-slot overwrite explicitly.
It compares the complete destination before writing and verifies packed readback afterward.
Stopped playback, unchanged local track/revision/connection and no active A/B owner are required.
An uncertain acknowledgement or readback reports uncertain persistence without retry. Bank reads
replace only the local base voice with undo; macros are preserved. Bank saving does not set PTCH
or save a project; those remain explicit device actions.

Deliver the codec/document first, then manual local editing, device apply/readback and
audition, then prompt refinement. The same foundation supports both experiences. See
[SOUND.md](SOUND.md), [AI-SOUND.md](AI-SOUND.md) and
[PROMPT-EDITING.md](PROMPT-EDITING.md).

## Acceptance

Every supported voice/operator field is reachable without AI. Test OP1/OP6 mapping,
algorithms/roles, range enforcement, operator copy, undo/redo, imported names/checksums,
macro context, pending updates, external PTCH changes and bank persistence. Golden codec
fixtures compare to firmware/web factory patches. Physical listening validates envelope,
ratio/fixed-frequency, velocity, feedback and audition behavior. Rendering a diagram or
passing byte tests alone does not establish that the manual editor controls the real engine.

