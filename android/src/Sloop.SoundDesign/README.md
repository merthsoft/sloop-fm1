# FM6 procedural sound-design foundation

Platform-neutral, package-free .NET 10 library. No Android, model runtime, audio, MIDI,
device transport, simulator, or solution-file changes are included. Recipes are a
procedural sound designer. They produce editable synth settings, not generated audio.

## Contracts

- `Patch`, `Operator`, and `Envelope` are immutable typed records. Operator order is
  **OP1..OP6** in the model, **OP6..OP1** in both wire layouts. `Algorithm` is **1..32**;
  the codec writes **0..31**. `Transpose` is the firmware's **0..48**, neutral **24**.
  Envelope values are firmware rate/level units, not milliseconds. Frequency mode,
  coarse/fine and detune remain distinct; fixed mode is fully round-trippable.
- `PatchCodec.EncodeVoice` / `DecodeVoice` handle exactly 155 bytes. `Pack` / `Unpack`
  handle exactly 128 bytes. FM6 wire records are already seven-bit: never apply pack7.
  Validation matches `eng_fm6.c` OPMAX/VMAX. Invalid values are rejected rather than
  silently sanitized. Unpack ignores unused packed bits as firmware does, but rejects
  invalid semantic values and non-seven-bit bytes. Canonical pack removes unused bits.
- Device names contain at most ten printable ASCII characters and are space-padded
  on the wire. Decoding trims trailing spaces. `DeviceName` is an explicitly lossy
  conversion helper; keep the original full local name separately in `PatchDraft`.
- `SysExCodec` validates complete single-voice and 32-voice-bank envelopes, sizes,
  seven-bit data and checksum. `ImportedSysEx.Original` / `ExportOriginal()` retain
  the exact original message, including unused packed bits. Canonical export is
  deliberately separate. Invalid/out-of-engine-range DX assets are rejected; there
  is no salvage parser, concatenated-message scanner or emulation claim. Bank export
  requires exactly 32 voices, without implicit truncation or padding.
- `FactoryLibrary` embeds the eight current firmware-generated voices. Percussion
  uses WOOD BARS; Effect explicitly approximates GLASS BELL without effects processing.
  Factory resources derive from `tools/gen_fm6_patches.py` (GPL-3.0-only, copyright
  2026 Leo Kuroshita / Hügelton Instruments); they are FM1's own patches.

## Generation, refinement and locks

`Generate` returns four independent drafts from a selected factory recipe. A seed and
versioned xorshift32 stream reproduce the same content across runtime versions.
Variation amount is 0..100; zero disables random variation and can yield identical cards.
Variations are bounded changes to active operator levels. Recipe identity is
`fm6-procedural/1`. Do not promise reproduction across recipe/firmware updates: persist
the accepted content, prompt, resolved intent, refinements, seed and provenance.

`SoundIntent` exposes bounded qualitative controls. `Template` leaves a dimension
unchanged. Higher attack speed increases envelope rate; longer decay/release decreases
rate. Brightness edits modulator levels using carrier roles from the firmware graph.
Organ/all-carrier brightness currently yields a disclosed no-op. Harmonicity edits
modulator fine ratios only in ratio mode. Movement edits LFO amplitude depth and does
not introduce pitch modulation. Higher velocity response changes operator sensitivity;
the nonlinear musical result still requires engine audition.

`Refine` edits a copy of the supplied current patch; it does not reselect a template,
randomize unrelated controls, change algorithm or tuning by default, or rename it.
Signed refinement amounts are -100..100 and saturate within firmware bounds. Zero
amounts remain exact no-ops. Opposing requests for the same dimension are disclosed
and leave that dimension alone. Refinements are deterministic; the seed is retained
as provenance, with no random refinement jitter.

`PatchLocks` enforce algorithm, tuning (operator frequency/detune, transpose, pitch
envelope and pitch-modulation depth/sensitivity), envelopes, attack (first rate/level),
all global voice controls, and/or entire individual operators. They capture values
from the supplied snapshot. Operator locks freeze operator data; freezing global LFO
controls requires `VoiceControls`. Locks govern patch values, not track macros.
To freeze the entire patch, lock all six operators and `VoiceControls`.

`PhraseParser` is a limited offline vocabulary, not free-form understanding. It
recognizes families, darker/brighter, envelope length/attack, expressive/metallic,
movement, register, mono/poly and preservation phrases. `Notices` must be displayed
and included in the proposal's explanation. `HasSupportedRequest == false` should
keep the source untouched and show the unsupported wording. Conflicting families
use the caller's supplied default and disclose it. Unsupported terms are never
invented controls. Typed controls remain available when wording is unsupported.

## Integration example

Add a project reference from the future adapter; this slice intentionally does not
edit the shared solution or existing projects. Capture the target's stable identity,
revision, full patch and seven sound macros before planning, and run planning away
from audio/MIDI scheduling threads. Access `SoundDocument` serially on its owning thread.

```csharp
var document = new SoundDocument("song:123/track:0", capturedState, capturedRevision);
var parsed = PhraseParser.Parse(prompt, SoundFamily.Keys, seed: 42);
if (!parsed.HasSupportedRequest) {
    // Show parsed.Notices; do not apply an empty interpretation.
    return;
}
// Combine parsed.Locks with any explicit editor locks before calling the designer.
var proposal = ProceduralDesigner.Refine(document.TargetId, document.Revision,
    document.State, parsed.Intent, parsed.Refinements, parsed.Locks, prompt, cancellationToken);
proposal = proposal with { Explanation = proposal.Explanation.AddRange(parsed.Notices) };
// Present proposal.Changes, Explanation, Before/After, locks and Audition.
// Only when the musician chooses Apply:
document.Apply(proposal);
// Undo/redo replay stored complete states, without generating again.
document.Undo();
document.Redo();
```

Use `Generate` for Create, and `RefineDraft` for follow-ups to a preview card. The latter
retains the original before state/revision for applying the complete refinement chain;
it records its parent draft identity and accumulated refinements. Switch A/B by reading
whole `Before` / `After` states, never by applying incremental mutations. Discarding a
draft has no side effects. Keep drafts tied to their target selection.

`SoundDocument.Apply` checks target, revision, complete before patch/macro equality,
validation and hard locks. It increments revision and records accepted history.
Undo/redo restore the complete patch and macro context and also advance revision, so
an older proposal stays stale after undo. Manual editing should use the same validated
transaction boundary. Persist state/revision and accepted drafts in the app's own
storage adapter; this library supplies no persistence IO or cross-editor history engine.

Create drafts declare `MacroContext.Neutral` (ALG=PAT and six neutral sound macros).
Refinements preserve the captured macro context. Macro changes are represented in
whole before/after states and generation explanations, separately from patch-field
diffs. Display that context during preview. Mono/poly preference and register are
intent/audition metadata; track allocation is not changed. Audition plans are note
metadata at 100 BPM, in sixteenth-note steps, with velocities 40/80/120 and at most
three simultaneous notes. No playback backend or audition success is implied.

## Future transport/audio boundaries

FM6_GET/PUT carry the 128-byte codec result directly. Track targets 0/indices 0..2 are
RAM; bank targets 1/indices 0..26 are separate explicit stopped-song flash operations;
factory target 2/indices 0..7 is read-only. Do not write a bank during ordinary preview.
Do not change PTCH as a sound macro: it reloads a different patch. Store an edited
patch locally and, if requested, in a bank with the correct PTCH selector for project
persistence. A project/PTCH/preset load can replace a RAM edit.

A future hardware-preview adapter must capture state, release its owned notes, send
one draft, await acknowledgment, read back the canonical result and offer Keep/Restore.
Compare fresh state before restore; engine/preset/PTCH/panel changes invalidate the
snapshot. Missing acknowledgments mean uncertain state: reconnect/read back before
claiming success. This library's local undo is not proof of hardware restoration,
and read/compare/write is not atomic arbitration with panel edits.

Local audio preview must wrap the actual firmware FM6 core and measure release/output
across register and velocity. The present tests do not render audio, calibrate spectral
qualities, verify hardware, or add FM6 behavior to the protocol simulator. Effects,
mix/shared delay/reverb, generic track ADSR and sampling/routing are outside this layer.
An optional later model should emit validated `SoundIntent`/`Refinement` data only;
never accept model-provided wire bytes or executable commands.

## Validation

From the repository root:

```powershell
dotnet restore android/src/Sloop.SoundDesign.Tests/Sloop.SoundDesign.Tests.csproj --configfile android/src/Sloop.SoundDesign.Tests/NuGet.Config
dotnet run --project android/src/Sloop.SoundDesign.Tests/Sloop.SoundDesign.Tests.csproj --no-restore
```

The package-free console runner matches the checkout's existing test-project style;
`dotnet test` is not its execution command. Failures print to stderr and return exit
code 1, avoiding Windows unhandled-exception dialogs. Golden tests verify 155/128-byte
factory vectors, web parity, source hash, every firmware bound and all 32 carrier maps,
plus randomized valid round trips, invalid inputs, SysEx/checksum/original preservation,
deterministic recipe output, locks, no-op/conflict handling, phrase disclosure,
cancellation, source immutability, stale-parent checks and full undo/redo.

Factory fixtures are checked in separately from production resources. Explicitly
regenerate them only after reviewing firmware changes:

```powershell
python android/src/Sloop.SoundDesign.Tests/regenerate_golden.py
```

That script reads the firmware's Python factory generator and writes only under these
two new directories. The C# tests do not run it automatically. Tests require the repo
checkout to validate firmware/web source parity. They compare the firmware's generator
and checked-in C tables; they do not execute the firmware C codec on hardware.
