# Sound editing and presets

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Status: FM6 data/recipes and manual Android editor implemented, with local persistence,
prompt proposals and physical GET/PUT/readback adapters. Reversible hardware RAM A/B with Restore/Keep is implemented; FM6 base-voice bank storage
is implemented through the existing FM6 bank protocol; process-scoped audition recovery is integrated. Physical bank-save validation remains required. 2026-10-08.

## Editor model

Combine curated engine layouts with device-derived descriptors. INFO defines counts; DESC
defines label, format, range, default, units and enum values. Cache metadata by firmware build,
engine and relevant context, not just parameter ID. Changing engine invalidates dependent
descriptors and displayed values. Track selection remains independent of editing destination
where TRACK_PARAM supports it.

Pages cover oscillator/engine, envelopes, pitch/glide/voice, LFO, arp, slicer/drive/filter,
effects sends and mixer. Drum track uses a kit/lane editor and avoids irrelevant synth controls.
Advanced pages expose all supported controls without hiding them behind panel button emulation.
Unknown parameters remain editable through a generic metadata view when their format is known.

## Gestures and synchronization

A drag updates local display immediately, sends coalesced settings, then displays acknowledged
clamping. Numeric entry respects units; enum pickers show names. Double-tap reset is explicit
and undoable. Hardware edits update controls without fighting an active gesture: record the
incoming revision, show a conflict indicator, and reconcile on release instead of oscillating.

Preset load updates sound and applicable sends/arp while preserving whichever mix/pattern/key
fields the firmware guarantees. Re-read rather than assuming a preset overwrote every field.
User preset save/erase writes flash and follows stopped-state restrictions. App presets store
portable metadata plus firmware identity; incompatibility is reported rather than silently
mapping numeric IDs onto a different engine.

## FM6

[Sloop.SoundDesign](../src/Sloop.SoundDesign/README.md) now implements the immutable
six-operator model, strict firmware bounds, OP6-first 155/128-byte conversion, names,
single-voice/bank SysEx validation and checksums. Imported assets preserve original
bytes separately from canonical export, including unused packed bits. Unsupported
formats and out-of-engine-range voices are rejected; there is no salvage import or
DX7 emulation claim. Bank export requires exactly 32 voices.

The library also supplies factory recipes, deterministic variations, constrained edits,
patch locks, explicit macro state and complete local draft undo/redo. Its test runner
passes 1,327 assertions, including firmware/web factory golden vectors and invalid-input
fixtures. Android operator/global/macro controls, factory selection, voice/bank import,
voice export, prompt proposals, local storage and FM6 GET/PUT/readback are integrated.
Sound now separates Edit sound from a visible Describe a sound page for offline recipes.
Manual editing includes firmware-derived algorithm diagrams (including macro overrides),
draggable native envelope levels, whole-operator copy/swap and bounded direct numeric entry.
The additional FM6 editing runner passes 2,891 checks; graph stage spacing is schematic.
FM6 base-voice bank persistence is implemented; generic user-preset storage and full all-engine UI remain pending. Audio and
physical listening validation are not supplied by these data tests.

The manual Android editor is implemented and its remaining UI targets are scoped in [FM6-EDITOR.md](FM6-EDITOR.md).
All supported patch parameters are editable without AI; prompt-generated drafts use the
same controls, document and undo history.

Prompt-driven patch generation and refinement are scoped in [AI-SOUND.md](AI-SOUND.md).
The same validated patch model, codec, operator editor and preset workflow serve manual
editing and generated drafts. AI generates settings; synthesis remains the firmware's job.

Provide algorithm diagram, six operator editors, output levels, ratios/fixed frequency,
envelopes, feedback and supported modulation parameters. Keep packed DX7 patch parsing and
checksums in a platform-neutral codec. Import single voice or bank with names, validation,
preview selection and a clear distinction between temporary application and persistent bank
save. Preserve original SysEx asset and unsupported bytes when exporting the original format.
Do not claim exact DX7 emulation for parameters the current engine implements differently.

## Parameter locks and macros

A lock captures a typed parameter/value with engine compatibility and a hardware capacity
check. Sound changes can invalidate lock meaning; warn and offer conversion/removal before
applying incompatible engine changes. Macro destinations carry explicit range/curve mappings.
Engine-specific IDs must be resolved against the current descriptor set.

## Acceptance

Verify engine switching, device preset reload, drag clamping, unknown enums, drum selection,
lock compatibility and save-busy behavior. FM6 fixtures cover valid/invalid checksums, single
voice and bank length, packed conversion, and names. Physical listening checks supplement
data equality; the app does not reproduce all firmware synth engines for offline preview.

## Reversible RAM A/B checkpoint — October 8, 2026

SoundEditor now captures a double-read, acknowledged FM6 original through the existing public
single-track scene batch API (patch, all seven macros, preset and PTCH). A/B writes validate
the complete current baseline and verify readback; writes are sequential and not atomic.
Restore verifies the captured original; Keep verifies B and retains device RAM only. Neither
operation changes the local document or its shared manual/prompt undo history. Local edits
invalidate B/Keep targets by document identity and revision; Restore remains available.

Unknown outcomes require read/reconciliation. Partial state or external preset/PTCH/macro/
patch edits block Restore/Keep. A changed connection token also blocks recovery writes because
public APIs cannot prove stable physical device identity. Abandon preserves current device RAM.
Activity recreation retains the original baseline in a process-scoped service. Session adoption marks active auditions for recovery and disables B/Keep until they are finished or abandoned. Sound pages show retained auditions on every synth track, with links to the recovery controls. Disconnect immediately marks retained records as conflicts, including during a pending write, and never discards the original or restores automatically. Records last for the app process; recovery after process termination is not implemented.
Dedicated SoundAudition runner passes 40 focused checks, including A/B, stale targets, external
edits, lost acknowledgment with verified readback, partial writes, reconnect conflicts, Activity-style service reacquisition, session adoption and disconnect during an in-flight write.
No APK build, installation or new hardware measurements were performed in this slice.

## Persistent FM6 bank saving — October 8, 2026

Sound offers stopped-only bank inventory, local base-voice reads and explicit destination/name
review for B1–B27. Occupied slots require a labelled overwrite confirmation. The destination
is re-read before writing; any changed record cancels the operation. Saving verifies all 128
packed bytes after acknowledgement. Flash refusal is reported; disconnect, lost acknowledgement
or mismatched readback never claims persistence and never retries automatically. Reconnect and
inspect the destination before choosing another save after an uncertain outcome.

Track/local revision changes and active retained A/B ownership block saving. Bank writes target
the explicit slot and never select a hardware track. They store the base voice only; track macros
and project association remain separate. Select the saved bank PTCH on FM1 and save the project
when project persistence is intended. Existing A/B Restore/Keep and recovery remain unchanged.
See [wire contract](../src/Sloop.Protocol/SOUND-BANKS.md).

Validation: dedicated Sloop.SoundBank.Tests passes 25 checks. Android compile succeeds with
the installed SDK/JDK (one unrelated PerformHardwareEditor member-hiding warning). No phone
installation, flash power-loss test or physical listening was performed.
