# Native starters, navigation and font packing — October 9, 2026

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
