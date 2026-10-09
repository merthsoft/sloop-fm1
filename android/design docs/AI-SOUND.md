# Prompt-driven FM6 sound design

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Status: platform-neutral foundation and offline procedural recipes implemented;
Android controls, persistence and hardware audition integration implemented; model inference
pending. Reversible RAM A/B and Restore/Keep implemented; hardware testing accepted for now. 2026-10-08.

## Implementation status

[Sloop.SoundDesign](../src/Sloop.SoundDesign/README.md) is a package-free .NET 10
library with immutable six-operator patches, firmware-compatible 155/128-byte conversion,
strict parameter/name validation, single-voice/bank SysEx checksums and original-asset
preservation. The model uses OP1-first order and algorithms 1..32; codecs use OP6-first
order and algorithms 0..31. Eight embedded factory patches and all 32 firmware carrier
maps support deterministic, seeded four-draft generation and bounded refinements.

Implemented contracts include `SoundIntent`, `PhraseParser`, `PatchDraft`, `PatchLocks`,
`MacroContext`, `ProceduralDesigner` and `SoundDocument`. Drafts retain complete before/after
patch and macro states, parent revision, provenance, changes and audition-note metadata.
Local apply rejects stale targets/revisions and lock violations; undo/redo replay stored
states. Refining a preview retains its original application/undo snapshot. Generation
declares neutral macros; refinement preserves captured macros. Phrase recognition is
limited and reports unsupported wording/conflicts; it is not model inference.

[Sloop.SoundDesign.Tests](../src/Sloop.SoundDesign.Tests/Program.cs) passes 1,327 assertions,
including all eight firmware-generator/web factory vectors, every parameter bound,
all carrier maps, valid/invalid SysEx, reproducibility, locks, cancellation, no-ops,
source immutability, stale apply and complete local undo/redo. These are data tests;
they do not execute the firmware codec on hardware or establish audible quality.

Android controls/project references, persistent patch storage and GET/PUT/readback adapters
are integrated. Hardware RAM Keep/Restore and process-scoped audition recovery are implemented.
Hardware bank saving, local rendering and optional models remain pending. Audition uses MIDI
notes for the current hardware sound. Mono/poly preference is metadata; no track
allocation changes are made. Effect uses a disclosed glass-bell approximation without
effects processing. All-carrier brightness edits currently produce a disclosed no-op;
movement changes LFO amplitude depth. Engine calibration, register/velocity listening
checks, release/output verification and phone benchmarks remain acceptance work.

## Product scope

The cross-editor intent/proposal/undo workflow lives in
[PROMPT-EDITING.md](PROMPT-EDITING.md); this document supplies its FM6-specific behavior.

Describe an instrument sound and receive an editable FM6 patch, with several variations,
an audition phrase, and a short explanation of the changes. Example prompts:

- A warm, round bass with a sharp attack and little sustain.
- A glassy bell that becomes brighter when I play harder.
- An airy pad with a slow attack, subtle motion, and no obvious vibrato.
- A metallic percussion hit, short and slightly inharmonic.
- Make this patch darker but keep its attack and tuning.

The output is synth settings, not AI-generated audio. The FM1 or a local implementation
of its synthesis engine produces the sound during audition. Generated patches remain
editable in the regular operator editor. Sound generation and loop composition are
separate operations; a loop can optionally request a sound for each synth part.

The manual editor in [FM6-EDITOR.md](FM6-EDITOR.md) is a required companion feature, not
an advanced mode hidden behind prompting. Users can hand-edit every supported patch field
and combine manual changes with generated drafts.

## Existing firmware support

The checked-out FM6 engine already exposes six operators with four-rate/four-level
envelopes, keyboard level/rate scaling, velocity sensitivity, ratio/fixed-frequency mode,
coarse/fine frequency and detune. Voice controls include 32 algorithms, feedback, pitch
envelope, oscillator sync, LFO and transpose. Each synth part supports at most six FM6
voices. The engine's own operator envelopes control amplitude: the generic track ADSR
and ENV DEST do not shape this engine. Recipes must use operator envelopes.

Protocol v9 supplies FM6_GET (68), FM6_PUT (69), FM6_LIST (70), FM6_ERASE (71).
The internal editable voice has 155 bytes; the wire patch has 128 packed bytes, already
7-bit, with no pack7 encoding. Operators are stored OP6 first. Targets are track 0
(indices 0–2), bank 1 (0–26), factory 2 (0–7; read only). Track PUT changes RAM immediately;
bank PUT writes flash and returns rc 3 while the song plays.

Projects preserve the PTCH selector rather than an arbitrary edited track patch. Project,
preset and PTCH changes can replace a RAM patch. Keeping a patch on the device requires
bank storage and the corresponding PTCH selector. The app also keeps the full patch
locally, independent of the device's bank. No new firmware command is required for a
first working prompt-to-patch feature.

Source of truth: [eng_fm6.c](../../firmware/src/eng_fm6.c),
[fm6_core.c](../../firmware/src/fm6_core.c),
[editor_fm6.c](../../firmware/src/editor_fm6.c), and
[v9 protocol](../../web/EDITOR_PROTOCOL.md#v9-fm6-patches-sloop-24).

## Generation architecture

Prompt -> SoundIntent -> recipe/template selection -> bounded mutations -> PatchDraft.

SoundIntent is platform-neutral C# data: family (bass, keys, bell, pad, brass, organ,
pluck, percussion, effect), brightness, harmonicity, attack character, decay/sustain/release,
velocity response, movement, register, mono/poly preference, variation amount and seed.
Qualitative controls use explicit bounded domains. Unsupported requests produce a clear
approximation description or clarification, not invented controls.

PatchDraft stores the full validated voice, optional per-track settings, resolved intent,
recipe/model/version identity, seed, base-patch reference, explanation, locks and its parent
revision. Effects are an explicit optional layer. Do not silently change shared delay/reverb
parameters or overwrite another track's mix while designing one sound.

Use calibrated templates from the existing eight factory families (TINE EP, GLASS BELL,
ROUND BASS, BRASS SECT, SOFT PAD, WOOD BARS, DRAWBARS, NYLON PICK), plus later curated patches.
The first implementation can operate fully offline with C# phrase parsing and recipes.
Call it a procedural sound designer while no model is running. A small optional local
language model later translates richer prompts into SoundIntent and constrained edits.
It does not emit arbitrary protocol commands or serve as the only patch validator.

For an initial draft, generate four reproducible variations around a suitable template.
For refinement, mutate the current patch within the requested dimensions and preserve locks.
Retain the previous revision so Undo and A/B work even after hardware application.
Generation must not require a network connection or run on the audio/MIDI scheduling thread.

## Mapping musical language to synthesis

| Description | Recipe controls | Constraint |
| --- | --- | --- |
| Warmer/darker | Reduce modulator output or accelerate modulator decay | Preserve carrier pitch and requested attack |
| Brighter/sharper | Increase selected modulator output/feedback; alter spectral envelope | Bound changes; keep velocity extremes usable |
| Bell/metallic | Select suitable topology, inharmonic ratios, transient modulation | Preserve a playable fundamental when requested |
| Round bass | Harmonic ratios, focused carrier register, controlled modulation | Check low notes and sustained levels |
| Plucked/wooden | Fast attack, decaying operator envelopes, brief bright transient | Ensure release reaches silence |
| Airy/evolving pad | Slow envelopes, layered carriers, subtle detune/LFO | Respect six-voice part limit; avoid unintended pitch drift |
| More expressive | Operator velocity sensitivity and spectral response | Test soft, medium and hard velocities |
| More movement | LFO and evolving modulator envelopes | Keep requested pitch stability |

These are starting hypotheses, not guarantees. FM output levels, envelopes and frequency
controls are nonlinear. Calibrate envelope recipes against the actual firmware instead of
pretending DX-style rate values are milliseconds. Preserve supported fixed-frequency mode
and learn useful ratio sets; do not assume every integer produces a desirable timbre.
Choose carrier/modulator roles from the firmware algorithm graph, not operator-number guesses.

## Sound workspace experience

A Create sound panel contains the prompt, current-patch versus new-sound mode, Generate,
four draft cards, and audition controls. Cards show family, variation, meaningful changes,
and the ten-character device name alongside the full local name. Keep the ordinary operator
editor reachable. Suggested refinements include darker, brighter, shorter, softer attack,
more expressive, more metallic, and less motion. Users can lock algorithm, tuning,
envelopes or individual operators before regenerating.

Audition uses a consistent phrase appropriate to the family: bass riff, chord, arpeggio or
percussion hits. Velocity and register can be changed. A/B switches the whole draft and its
explicit macro context; it does not compound mutations. Favorites remain local until the
user chooses Apply to track or Store in FM1 bank.

For hardware preview, read and retain the original track patch and relevant per-track state,
send one validated draft at a time, await acknowledgment and read back the canonical patch.
Release audition notes before switching patches. Restore returns to the captured sound;
Keep makes the auditioned RAM patch current. Detect engine/preset/PTCH changes and refresh
instead of silently restoring over an external edit. A lost acknowledgment means state is
uncertain: reconnect/read back before claiming a restore or successful application.

The seven FM6 sound macros can alter a patch substantially. For generated sounds, use a
declared baseline (ALG=PAT; FB/MLVL/MRAT/MEG/VMOD/DTUN neutral), or preserve and display an
explicit macro context. Changing PTCH can reload a different patch; do not treat it as an
ordinary brightness control. Bank save is a separate stopped-song operation, with destination
selection and preservation of any existing bank entry through the normal preset workflow.

## Offline audition and optional AI

Generating patch data works without an FM1. Audition without hardware requires a native
backend executing the existing FM6 core, adapted from the host simulator, with the same
tables, topology, envelope behavior and patch conversion. Start with short offline phrase
renders and a bounded cache; add interactive playback after measuring it. Synth-rendered
audio used for preview is not a text-to-audio model. Do not use an unrelated DX synth and
claim its preview matches this firmware or include effects the preview does not implement.

Benchmark a small quantized text model on the current Pixel 7a before selecting a runtime
or model pack. The model is optional, installed once by download or local import, and then
used offline. The C# recipe/validator/library layer is reusable on future Windows/iOS;
only inference and audio adapters are platform-specific. See
[COMPOSITION.md](COMPOSITION.md) for the proposed inference boundary and source links.

A later search mode can render candidate patches with the actual engine, measure attack,
spectral balance, harmonicity and release, then rank candidates for the resolved intent.
This may improve consistency, but audio descriptors do not establish subjective musical
quality. User selections/refinements can personalize template ranking locally without
requiring an on-device training pipeline.

## Delivery slices and effort

| Slice | Work | Status / remaining effort |
| --- | --- | --- |
| Foundation | FM6 patch model/codec, GET/PUT/readback, operator UI, local patch library | Model/codec, Android controls, templates, SysEx, current-patch persistence and device adapters implemented; richer library/graphs remain |
| Offline recipes | SoundIntent, factory-template library, constrained mutations, seeds, four variations | Library, Android prompt proposals/diffs/locks and local transactions implemented; engine calibration remains |
| Hardware audition | Note ownership, A/B/restore, macro context, bank persistence | Explicit RAM apply/readback, macros and owned MIDI audition integrated; reversible A/B/Restore/Keep implemented; bank saving remains; process-scoped lifecycle recovery integrated |
| Local audition | Native firmware FM6 renderer, phrase playback, cancellation/cache | Note-plan metadata implemented; renderer/playback/cache pending, medium to large effort |
| Free-form language | Local model adapter, model pack lifecycle, constrained intent output, phone benchmarks | Pending; medium to large effort, runtime/device uncertainty |
| Candidate ranking | Descriptor analysis, calibrated search and preference ranking | Pending optional enhancement; larger effort |

The recipe-based version is substantially easier than finished audio generation. Free-form
prompt quality and offline preview are still separate engineering problems. Do not commit
to a timing/memory estimate until the native renderer and inference spike run on the phone.

## Acceptance gates

The codec, deterministic recipe, lock and local transaction gates below have automated
data coverage in the foundation; Pixel exercised local prompt proposal/apply without a device.
Dedicated airplane-mode, playback/listening, hardware
recovery/persistence and model-runtime gates remain pending. Run instructions and precise
boundaries are in the [implementation README](../src/Sloop.SoundDesign/README.md#validation).

- Golden pack/unpack vectors against firmware/web factory patches, all parameter bounds,
  OP6-first ordering, algorithm numbering and name conversion; invalid patches rejected.
- Fixed seed reproduces drafts; locked fields survive refinement; generating never mutates
  the live patch; undo restores the complete patch and macro context.
- No-op refinements remain no-op; conflicting instructions and unsupported capabilities are
  disclosed; no arbitrary model-provided wire bytes are accepted.
- Every family auditions across register and velocity; release ends, output is usable, and
  heavy feedback/modulation variations are judged on the real engine. Avoid audible switching
  artifacts by stopping owned notes and using consistent audition gain.
- Track PUT/GET equality and restore recovery pass; no flash writes during ordinary preview;
  bank save handles stopped-state requirements and project/PTCH persistence.
- Offline recipe mode works in airplane mode. Model mode also works offline once its pack
  is installed, handles cancellation/memory pressure, and leaves manual editing available.
- The protocol simulator must gain explicit FM6 behavior before tests claim it supports
  these operations. Current simulator coverage does not include FM6 patch commands or audio.

## Hardware A/B checkpoint — October 8, 2026

Both manual and prompt pages expose acknowledged original capture, local B, original A,
phrase playback/stop, Restore, RAM-only Keep and explicit reconciliation/abandonment.
Apply a reviewed proposal locally first to use it as B: this keeps the existing validated
proposal application and shared history authoritative. Hardware actions never save a proposal
locally or add a second undo stack. Local identity/revision changes reject B/Keep; Restore
only addresses captured hardware. Process-scoped originals survive rotation and session adoption,
with visible recovery controls. Replaced sessions disable B/Keep; guarded Restore remains available
on the same connection. Disconnect immediately marks Conflict and preserves the original until
explicit abandonment, including during an in-flight write. Reconnect never grants blind write
permission. Bank storage, automatic disconnect rollback and recovery after process termination
remain unimplemented. Sequential patch/macro writes can leave partial state;
unknown or conflicting state is displayed and never reported as successful restoration.
See VERIFICATION.md for integration and the 40-check focused runner.
