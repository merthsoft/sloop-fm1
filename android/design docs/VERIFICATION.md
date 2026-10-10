# Integrated Merthsoft.7 — October 9, 2026

## Modulation displays and save-loss investigation — Merthsoft.18

- `tests/edit_tools_test.c`: square-wave bounds, zero-depth baselines and drawing that leaves audio modulation state unchanged; fresh waveform/locked-panel captures.
- `tests/project_test.c`: production project capture, NOR A/B replacement of slot B, loss of RAM, import/apply and exact instrument/voice/key/note restoration pass. This models serialization/storage, not hardware writes or the complete boot/UI save path.
- Modulation behavior and broad UI regressions pass. Target image 578,912 bytes; static RAM 96,244 bytes; pool 333,948 bytes. No DSP changes or new display allocation.
- Reported hardware save loss is unresolved; the firmware updater's app-write window excludes project storage. Device save acknowledgement and exact missing preferences are still needed.

## Scale guides and whole-pattern EDIT octave — Merthsoft.17

Held LFO/ENV and synth EDIT show key-signature guides with physical-key hints;
keyboard/erase/MIDI mapping remains unchanged. Focused real-UI tests cover E-minor
F#, C-minor Eb, physical F erasing F while preserving F#, EDIT knob 4 octaves,
semitone/shift totals, undo/redo and undo-then-edit reset, chord intervals at MIDI
bounds, ties/metadata, drum rejection and neighboring-track isolation.
Modulation regressions and 20,000-frame broad UI fuzz pass.

Target image 578,208 bytes; static RAM 96,244/98,304 (2,060 free); pool unchanged
333,948/344,064. HAL and call-free 925-instruction RAM text pass. The 610,066-byte
package is served on port 8803, SHA-256
`2f8e1d51046a669618194feacf950f76b73f072cdc4a7fcf45d54ef6a0faaf66`.
Physical hardware validation is not claimed.

## Retained sequence-browser choices — Merthsoft.16

Native sequence choices initialize once and remain shared across synth tracks.
The real UI regression checks retained root/scale/octave/mode/VLEAD/rhythm choices,
reopening, applying D-minor bass to a track configured for A major, unchanged track
SEL parameters, and neighboring-track isolation. Starter UI and phone protocol
regressions pass. Broad 20,000-frame UI fuzz passes.

Target image 577,072 bytes; static RAM 96,212/98,304; pool 333,948/344,064.
HAL and call-free 925-instruction RAM code checks pass. The installer at
`http://127.0.0.1:8802/webapp/installer/` serves the hash-verified 610,066-byte package:
`05d8075ed7dc8cecb0dcd1c9e75bcc69af570e5600f2c8b4711fbdffca61d658`.
Physical validation is not claimed.

## Branch optimization — October 10, 2026

The optimized build is packaged as **2.5 Merthsoft.15**. It links at
577,056 image bytes (816 fewer) and 96,212/98,304 static RAM bytes (1,536 fewer,
2,092 free); pool remains 333,948/344,064. HAL and call-free 925-instruction RAM
code checks pass. Rebuilt baseline `d45c21c` confirms the previous sizes, and every
tracked DSP/ISR cost record is identical. See the [optimization record](../../docs/firmware/OPTIMIZATION.md).

The local installer at `http://127.0.0.1:8801/webapp/installer/` serves the verified
610,066-byte package, identity FM-1_900, SHA-256
`17874cb08879526337158768707f8815a12a7cf5c750e9b4a3728069912a8f60`.

Host coverage includes 20,000-frame UI fuzz, all fourteen byte-identical visualizer
captures, shared-history transition/snapshot assertions, an independent all-scale
pitch oracle, complete groove/sequence undo, audition ownership and phone protocol
tests. Fresh divide-by-zero trap tests cover UI, sequencer, projects and one minute
of live-use simulation. The packed-font stress reference decoder was corrected;
its 153,600-pixel oracle and 40,000-frame stress rerun pass. The sanitizer runner
now refuses to execute old binaries when compilation fails.
An additional 40,000-frame undefined-behavior trap run passes, as does the Windows
Node web protocol/sample/updater suite.

The complete host run retains two baseline failures: LOFI/8BIT_ARP differs from the
stored golden, and the ISR cost exceeds its historical budget. The same host
compiler and untouched baseline reproduce both; all 182 audio render hashes match
between trees, with no health/routing failures or crashes. No golden/budget update
was made. ASan is unavailable in this compiler; trap instrumentation is explicitly
reported instead. Physical device and deadline/stack validation are not claimed.

## Illustrated workflow documentation — October 10, 2026

Fresh production-code host renders show STEP tie/rest painting, moving an onset
inside its own tie chain, SEL latch controls, EDIT's load-undo affordance with real
project-history availability, and SAVE NEW/confirmation. The host uses panel and
storage ports; these captures illustrate controls rather than claiming a physical
device test. All native workflow images were visually reviewed.
Four fresh captures from the connected Pixel show musical/drum library entry
points, the existing-note prompt dialog and its exact proposal review. The phone
was connected to the PC, so library browsing correctly appears disabled without
FM1; no connected hardware-library apply is claimed. The proposed C-to-C# edit
was discarded, leaving the note unchanged. No old Android capture was reused for
these additions, and no scale-light hardware photograph is included.
The illustrated guide's 25 images and README images validate; local documentation
targets resolve apart from existing GitHub-relative issues/releases links.
README now describes current capabilities without feature release-version/date
annotations; historical release and test evidence stays in the detailed records.
Firmware and Android source are unchanged in this documentation checkpoint.

## Merthsoft.14 modulation locks — October 10, 2026

Target image 577,872 bytes; static RAM 97,748/98,304 (556 bytes remaining);
pool 333,948/344,064; RAM text 925 instructions/no calls; HAL access check clean.
Package 610,066 bytes, FM-1_900; SHA-256
`486fcb221954e7f990837883eea8804d9664aa2a41fb177715235e3c1b5b21f0`.
Focused modulation regression passes for both effects: HOME lock survives release,
keyboard gates and knobs remain functional, original page and scale LEDs are
preserved, HOME/function unlock and STOP/panic/track change cleanup work, and drums
cannot lock. Broad real UI/audio suite and 20,000-frame fuzz pass. Fresh native
library, recording, arp, scale/MIDI and LOCK framebuffer captures were visually
reviewed. Documentation links and image assets checked. Android code is unchanged;
no physical or new target ISR/stack measurements are claimed. Existing ISR
cost-budget limitations remain recorded below.

## Merthsoft.13 scale/MIDI grids and beat sync — October 9, 2026

Target image 577,744 bytes; static RAM 97,748/98,304 (556 bytes remaining);
pool 333,948/344,064; RAM text 925 instructions/no calls; HAL access check clean.
Package SHA-256: `72425357615a5f8fb245716f37fe71db4d50a0f4b196f4c295fcfaf5dcbd2d19`.
Focused scale/MIDI tests pass across all roots/scales/key modes and MIDI pitches,
including accidental spelling, original-pitch note-offs across changes, SNAP
collisions, independent USB/TRS owners, USB reset, selected-track routing, panic,
capacity rejection and persisted preference. Modulation tests verify all nine sync
divisions against transport phase, independence from free Hz, stopped BPM changes,
and knob 1 returning to OFF. Scale-light and musical-starter integration pass.
Broad UI/audio regression and 20,000-frame fuzz pass; SYSTEM K2 MIDI SCALE and K3
ABOUT navigation are exercised. Fresh SEL/vibrato/tremolo framebuffer renders were
visually reviewed. No physical testing or new ISR deadline/stack measurements are
claimed. Android/wire/project formats remain unchanged.

## Merthsoft.12 modulation-panel lights — October 9, 2026

Focused scale-light regression passes: all roots/scales and background modes/levels
in PLAY/VIB/TREM, plus real button holds/releases with scale guide enabled/disabled.
Dim, bright, backlight and landmark masks retain their normal playing values.
Target image 576,008 bytes; static RAM 97,508/98,304; pool 333,948/344,064;
RAM text 925 instructions/no calls; HAL check clean. Package SHA-256: `45bc0f09c4cbd6fce96e1f32dfebf5e62c6a0c488277f662f17a86a816bb0dc5`.
No physical LED measurements or new ISR deadline measurements are claimed.

## Merthsoft.11 performance modulation — October 9, 2026

Build passes: image 576,056 bytes; static RAM 97,508/98,304 (796 bytes remaining);
pool 333,948/344,064; RAM text 925 instructions/no calls; HAL check clean.
Package SHA-256: `74ca04df2d3fb52d827547f753b6ab794807eb474d3de18ccd79193193f97969`.
Focused host tests cover selected-track vibrato/tremolo isolation, popup release,
normal taps, four waveform bounds, saved patch preservation, note gates, MIDI
source ownership and STOP/panic cleanup. Broad UI/audio suite and 20,000-frame fuzz pass. Android code is unchanged. Hardware
validation and new ISR deadline measurements are not claimed.

## Merthsoft.10 modulation checkpoint — October 9, 2026

Target build passes: image 575,360 bytes; static RAM 97,444/98,304 (860 bytes
remaining); pool 333,948/344,064; RAM text 925 instructions/no calls; HAL check clean.
Package SHA-256: `38609157694869baad071fc4050a0185fbb15da8a2972089bb731fc933ccfd28`.
Focused native modulation and scale-guide regressions pass; modulation covers CC1
routing, physical priority and release, rate/depth preservation, bounds, note gates,
USB/TRS packet ownership, USB reset and STOP/panic/reset isolation. Broad UI/audio
suite with 20,000-frame fuzz passes. Domain performance-controls runner passes
3,035 checks, including touch ownership, duplicate coalescing, zero-reset, invalid
values/destinations and send-failure cleanup. Android APK build passes with zero
warnings/errors. No firmware flash, phone installation or live gesture validation
is claimed. Existing target ISR cost-budget failures remain unresolved.

## Merthsoft.9 scale-guide corrections — October 9, 2026

Target image 574,832 bytes; static RAM 97,412/98,304; pool 333,948/344,064;
RAM text 925 instructions/no calls; HAL check passes. Package SHA-256: `3abc96bbf9ed7ba523b9b373a5de5a2ba8a0fc9302c0d6615b1fbaeda3bb7206`.
Focused regression now checks deliberately different selected/first-track keys/scales,
held display and knob alignment, and every background mode/level against all root/scale
masks. Broad UI/audio suite and 20,000-frame fuzz pass. No physical flash/LED measurement
is claimed; existing ISR budget limitations remain. Android/wire protocol are unchanged.

## Merthsoft.8 scale-light checkpoint — October 9, 2026

Target build passes: image 574,720 bytes; static RAM 97,412/98,304; pool
333,948/344,064; RAM text 925 instructions/no calls; HAL access check clean.
Package SHA-256: `c4900695f509d2102c9099b8bb569826dd12a4ed862a44dd948f25a925e00cbe`.
The focused scale-light production UI/audio regression and broad UI/audio suite with
20,000-frame fuzz pass. Physical LED brightness has not been measured. Android and
protocol code are unchanged; their most recent validation is the Merthsoft.7 checkpoint
below. Existing ISR cost-budget failures remain unresolved.

All 14 retained visualizer captures were exported losslessly from this same fresh
production UI/audio run and visually reviewed. The README shows the six additional
styles; the [full gallery](../../docs/firmware/VISUALIZERS.md) includes upstream styles
and documents the idle Song Journey capture accurately.

- Combined target passes image/RAM/pool bounds, HAL register access and RAM-text checks: image 574,480; static RAM 97,412/98,304; pool 333,948/344,064; RAM text 925 instructions/no calls. Static headroom is 892 bytes; this is separate from the 10,116-byte remaining pool budget.
- `build/merthsoft7-android-build.log`: complete solution and signed APK, zero warnings/errors. `build/android-domain-integration7/results.json`: all 27 runners pass, including external MIDI ownership/timed cancellation and 306 musical starter protocol checks. Storage runners execute outside the sandbox because atomic temporary-file moves are restricted there; this does not change their assertions.
- `build/host/seq_navigation-integration7.log`: real physical held-OCT gestures for octave editing and knob-3 moves, own-tie shortening at pattern end, lock collision precedence, exact metadata undo/redo, interval-preserving MIDI bounds, retained tie/rest paint and independent recording snap.
- `build/host/seq_arp_expression-integration7.log` and `seq2-integration7.log`: live/sequence velocity, selectors, PULSE collisions, release/route/tie/recording lifecycles and retained sequencer behavior.
- `build/host/sequence_starters_ui-integration7.log`: preview state is byte-identical across internal SELECT-knob page navigation; musical changes still release audition. Entry, hold, replacement, metadata undo and transport/physical takeover checks retained.
- `build/host/editor_musical_starters-integration7.log`: real production generator, request validation, leased audition, disconnect/reset/expiry, MIDI and physical takeover, held-input refusal, per-track panic bookkeeping, stale selection and hardware undo. Command 74 is independently retained in the chat's focused check.
- `build/host/ui_pages-integration7.log`: broad UI/audio regression and 20,000-frame fuzz pass. `build/merthsoft7-web.log`: browser protocol/updater tests pass; both protocol 13 and 14 relocate SYN commands to 80–84.
- `build/merthsoft7-budget.log`: inherited target ISR static budget still fails (34,943 versus 174); previous Merthsoft.6 baseline was 34,612. No budget thresholds were changed. These static counts are not measured real-time deadline evidence. New expression storage is bounded; memory/pool checks are not a complete stack/high-water measurement.
- Signed APK installed on Pixel serial 36121JEHN07361. Cold launch returns PID 11731; no fatal crash in filtered AndroidRuntime/DOTNET/monodroid log. System UI hierarchy confirms the phone is locked, so new screen interaction, controller hardware input and phone/FM1 round-trip audition are not claimed. Existing hardware acceptance remains accepted for earlier behavior.
- Native README frames regenerated from the current host UI. Existing October 9 fresh Pixel screenshots remain the earlier README capture checkpoint. Local documentation links/images checked; only existing GitHub-relative issues/releases links are outside the filesystem.
- Installer served at `http://127.0.0.1:8793/webapp/installer/`, HTTP 200 and version 2.5 Merthsoft.7 verified. Package SHA-256 `5383f87299acb314bda595a634879ae75aa1de2fd21ad70d5829db5bdd9c0c30`.

## Merthsoft.6 checkpoint

# Tie/rest painting and native starters — October 9, 2026

**2.5 Merthsoft.6** target build passes: image 571,472 bytes, RAM 97,348/98,304,
pool 333,948/344,064, RAM text 925 instructions/no calls and HAL access checks clean.
`tests/seq_navigation_test.c` exercises production controls with audio running:
tie/rest painting, cross-bank sweeps, unchanged source chord and flags/levels,
backtracking, end stops, separate hold sessions, full step undo/redo, normal cursor
wrapping, octave controls off STEP, page navigation and SNOTE ARP mode changes.
It passes undefined-behavior trap instrumentation with only `shift-base` excluded;
existing DSP negative signed left shifts prevent a full-UBSan whole-audio claim.
The 24-starter pure generator passes full UBSan, including fixed POP FOUR nearest
voicings, disabled-mode identity and unchanged bass. The broad sequencer suite passes
coarse eighth/quarter capture, future/past held notes, chord grouping, loop wrapping
and incompatible-grid fallback, plus its retained recording/live-voicing regressions.
The browser suite passes Major initialization, independent octave, navigation,
apply/hold/undo and audition ownership. These runners are in the normal test script.
The broad UI/audio regression and 20,000-frame fuzz pass. The all-scale native browser regression also guards against duplicate voices in sparse-scale inversions.
No new hardware flash is claimed. The README's four Android images are fresh captures
from the connected Pixel on October 9; no older captures are used. Native library
images are current actual host framebuffer renders.

Merthsoft.5 pure-generator and native UI tests passed for distinct BASS/ARP rendering,
all starter chord tones, retained syncopation and the explicit POP FOUR note sequence.

## Merthsoft.4 checkpoint

**2.5 Merthsoft.4** integrates twelve musical starters, 24 drum grooves, native rhythm
controls and audition, 700 ms apply holds, and shared metadata undo.

- `build/seq_navigation-next.log`: physical SEQ/SELECT reaches all synth pages and the
  browser rhythm subpage; ARP knobs edit the selected track while a four-note source plays.
- `tests/seq_arp_modes_test.c`: every mode emits its expected pitches through production
  MIDI, with fifteen ties, grid-aligned runtime switching and balanced releases. OFF/on
  transfers sustained notes; duplicate-pitch dynamics, rests and skipped notes are covered.
- `build/drum_grooves-next.log` and `build/editor_drum_grooves-next.log`: 24 templates,
  shaped hit preservation, canonical phone apply, physical hold/cancel, metadata undo,
  full Amen preview, project preservation and lifecycle guards pass.
- Musical generator and real UI tests pass: root/scale/modes, rhythm, Feel, 700 ms apply,
  complete undo/redo, track isolation and preview ownership. Audition refuses an existing
  live latch rather than altering its notes.
- `build/native-starters-ui-final.log` and `build/native-starters-ui-final-divzero.log`:
  real UI/audio and 20,000-frame fuzz pass in optimized and divide-by-zero-trap variants.
  Full undefined-behavior sanitizer is clean for the pure generators; fixed-point audio
  arithmetic prevents an equivalent whole-audio claim. Font tests pass 6,120 pixel/metric
  comparisons for each of packed and legacy formats.
- `build/native-starters-regress.log`: 182 unchanged audio renders; zero health,
  voice/routing, CPU-budget or crash failures. Host timings do not prove target deadlines.
- `build/native-starters-web-final.log`: web/editor/OTA compatibility passes.
- `build/native-starters-android-build.log`: solution/APK build passes with zero warnings
  and errors. The updated 24-groove protocol runner passes. No new phone install claimed.
- `build/merthsoft-4-build.log`: final image 568,800 bytes, RAM 97,332/98,304,
  pool 333,948/344,064; RAM text 925 instructions/no calls and HAL checks pass.
  Font bitmap data saves 16,128 bytes; the splash remains. Target font speed has not
  been benchmarked. No physical firmware flash or new hardware listening is claimed.

Historical verification follows. Existing target ISR cost-budget and ASan limitations
were not removed or relaxed.

---

# Black-key punch FX verification — October 9, 2026

**2.5 Merthsoft.3** uses the existing ring/DSP with no extra sample buffers.
- `build/punch-mods-final.log`: all 16 effects x 10 modifier combinations, audible changes,
  opposing controls, output bounds and exact dry cleanup; real keyboard capture/release,
  duplicate-key ownership, retrigger, STOP and no recording/synth playback pass.
- `build/punch-ui.log`: undefined-behavior traps, real UI/audio flow and 20,000-frame fuzz pass.
- `build/punch-ui-final.log`: final compact-label UI input/render/fuzz passes; rendered
  `build/host/layer-punch-modifiers.png` was visually inspected for readable labels.
- `build/punch-remote.log`: actual physical/remote priority, release, expiry and STOP pass.
- `build/punch-regress.log`: 182 unchanged golden renders, zero health/voice/routing failures,
  zero host CPU budget failures and zero crashes.
- `build/merthsoft-3-build.log`: image 579,492 bytes, RAM 97,204/98,304, pool 333,948/344,064;
  RAM-text 925 instructions/no calls and HAL checks pass. No splash removal required.

UI-visible latch/key state uses volatile reads across ISR/main contexts. No new phone test
or physical firmware flash is claimed. Earlier target ISR cost and ASan limitations below
remain unresolved; host CPU checks do not establish target audio deadline margin.

---

# Manual ARP latch verification — October 9, 2026

**2.5 Merthsoft.2**: `build/arp-latch-before.log` reproduces failure to capture three piano
notes with CHORD OFF/ARP ON. `build/arp-latch-after.log` passes capture, one toggle per hold,
toggling off while keys stay held, re-enabling, release retention and cleanup, plus existing
generated-chord/modifier/roll regressions and 20,000-frame UI fuzz. Real audio runs between
input frames; assertions and divide-by-zero trap instrumentation remain active.
Target build log `build/merthsoft-2-build.log`: image 578,248, RAM 97,108/98,304,
pool 333,948/344,064, RAM-text and HAL checks pass. No physical flash or new phone test.
Earlier broader merge checks and their limitations follow.

---

# Upstream 2.5 merge verification — October 8, 2026

Release **2.5 Merthsoft.1** builds successfully: image 578,152 bytes, RAM 97,108/98,304,
pool 333,948/344,064; RAM-text and HAL checks pass. The leading version identifies upstream;
`Merthsoft.N` is our release counter.

- `build/merge-main-android-build.log`: solution builds with zero warnings/errors.
- `build/merge-main-domain-tests/results.json`: all 25 domain runners exit 0.
- `build/merge-main-host-tests.log`: real firmware suites, 182 unchanged golden renders,
  voice/routing checks, UI fuzz and 10-minute soak pass. Aggregate exit remains 1: its
  sanitizer compiler default was unavailable and the existing ISR cost baseline is exceeded.
- `build/merge-main-sanitizers.log`: explicit Zig trap-instrumented divide-by-zero tests
  for UI/seq/project/one-minute soak and 15,000-frame UBSan stress pass. ASan is unavailable
  with the installed compiler. This does not claim equivalent memory instrumentation.
- `build/merge-main-usb.log`: CDC 0/1/2, descriptors, loader and USB return tests pass;
  new assertions verify 48 kHz capture negotiation cannot reset/change 44.1 kHz playback.
- `build/merge-main-ui-final.log`: final UI input/render/fuzz and persisted visualizer IDs.
- `build/merge-main-web-tests.log`: full editor/sample/package/updater simulations pass;
  protocol 13 routes SYN commands 80–84 without consuming companion IDs 72–75.

The historical target ISR static estimate is compiler/inlining-sensitive and remains above
its old budget. It has not been reset to conceal the failure; actual target timing needs
hardware measurement before claiming a performance margin. No phone test or firmware flash
was performed for this merge. Earlier device observations below are historical evidence.

---

# Current verification — October 8, 2026

Firmware 2.4.16 adds whole-project load undo/redo. Assertion-enabled, undefined-behavior
checked project tests exercise full capture/exchange/apply, repeated direction rejection,
subsequent loads, sequence editing and recording-history invalidation. Real UI rendering/input
tests (including 20,000-frame fuzz), sequencer lifecycle tests, and song/audio restoration tests
pass. Logs: `build/project-load-undo-test.log`, `build/load-undo-ui-test.log`,
`build/load-undo-seq-test.log`, `build/load-undo-song-test.log`.
Target build passes image/RAM/pool/RAM-text and HAL checks. Snapshot uses 3,840 bytes of
ordinary RAM to retain the required pool reserve; RAM is 97,300/98,304, pool 334,560/344,064.
No physical EDIT + OCT recovery gesture or firmware flash was performed by this chat.

Perform follow-up: the Transport runner passes 26 checks, including repeated latched block
attacks with bass and updated velocity, common-tone preservation during modifier revoicing,
shared-owner protection, and final note release without stuck notes. Voice lead is now beside
Latch and affects the next voicing without releasing the currently sounding chord.
The follow-up Android build passes with zero warnings/errors, installs on the Pixel 7a,
and cold-launches in 734 ms. The live UI hierarchy confirms Latch and Voice lead share
the same row. MIDI attack behavior is verified by captured messages in the Transport tests;
speaker behavior has not been retested on the physical FM1 for this follow-up.

Combined Android build succeeds with zero warnings/errors. All 25 domain runners pass,
including 546 composition checks and 26 cross-session library checks. The repeatable
[domain runner](../scripts/Test-Domain.ps1) writes per-project logs and `results.json`.
Logs: `build/integration-2.4.15-android-build.log`, `build/integration-2.4.15-domain-tests/`.

Composition tests exercise real ZIP/JSON roundtrips preserving edited notes/identities,
prompt/seed/resolved intent/kept parts/generator; CRC, versions, missing/unknown/duplicate
fields, paths, extra entries, truncation, invalid note bounds and streamed byte caps reject.
Failed catalog publication preserves existing drafts; stale apply rejects. Android adds
workspace/pattern guards before and after asynchronous import and at dialog confirmation.

Sample-library tests use actual filesystem snapshots and staged copies: exact WAV and
sidecars, chop-frame identity, corrupt sibling isolation, stale/missing sources, duplicate
reuse, semantic metadata validation, cancellation before and during copy, and stage cleanup.
Prepared conversion is invalidated on reuse. Library UI is reachable from Sample and Library.

Firmware 2.4.15 builds: image 580,296 bytes, static RAM 93,444/98,304, pool 334,560/344,064;
RAM-text 925 instructions/no calls and HAL checks pass. Protocol remains 12; ORD gains
SNOTE/SPLAY with unchanged parameter counts, persisted layouts and legacy enum values.
The sequence source adds 18 bytes; full general live ownership remains inactive.

The dedicated real-engine arp suite passes with C assertions enabled, covering actual
events/USB/TRS ingress, overlapping sequence/live pitches and audio gates, direct chord
suppression, microtimed NOTE/TIE/REST, fill rejection, recording skip, saturation, latch
modifiers, route changes, STOP/panic/project cleanup and balanced generated MIDI output.
The component's UBSan, sequencer, scale/latch and historical project/preset regressions pass.
Parent reran the dedicated arp and both groove suites on the combined source.
Logs: `build/integration-2.4.15-arp-tests.log`, `build/integration-2.4.15-groove-tests.log`,
`build/firmware-2.4.15-build.log`. The arp suite is now in the shared firmware runner.

Existing target ISR cost check still fails: cost 31,596 versus budget 174. Thresholds
were not raised (`build/integration-2.4.15-target-budget.log`). Successful builds and host
functional tests do not prove physical audio deadline headroom. Full live owner identity,
MIDI source/generated aggregation, semantic followers and atomic scenes are not delivered.

The new APK is installed on Pixel 7a 36121JEHN07361; cold launch succeeded in 1,016 ms.
Live sample-library indexing found a saved-session asset and showed source/search/reuse
controls. A 92-note procedural draft was exported through SAF, then selected and validated
through import review and saved/opened after explicit confirmation; no sequence apply was
performed. The provider appended `.zip` to the `.sloopdraft` filename; import handles it.
The pulled archive passes ZIP CRC validation, and its full draft/notes/metadata/keep payload
matches the imported on-phone catalog exactly; the catalog entry has a fresh identity.
The reproducible comparison is `build/check-live-draft-2.4.15.py`.
Evidence: `build/integration-2.4.15-live/`, including exported archive and UI dumps.
Phone source-copy cancellation/reuse and physical FM1 arp/audio performance were not tested
live in this checkpoint. Firmware is served, not flashed by this chat.

Earlier evidence: [2.4.14 checkpoint](archive/checkpoints/INTEGRATION-2.4.14.md).
Package identity and installer: [release notes](RELEASE-NOTES.md).
