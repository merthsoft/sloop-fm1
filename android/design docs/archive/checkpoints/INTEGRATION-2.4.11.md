# Current verification record — October 8, 2026

Firmware 2.4.8 adds latched modifier toggles. Host scale/keyboard tests cover release
persistence, second-press removal (including the other octave), independent combined qualities,
new-root persistence, per-synth isolation and latch/CHROM/STOP cleanup. Sequencer regressions
pass, including unchanged momentary modifiers. Target image is 571,500 bytes with unchanged
92,948-byte static RAM and 925-instruction call-free RAM text. Package details are in
[release notes](../../RELEASE-NOTES.md). This verification did not flash hardware.

Hardware atomic scene staging remains a tested scaffold, disabled in production until complete
engine revision/scheduling/application hooks are available. Existing stopped-only transfer stays usable.

## Integration audit — October 8, 2026

The final combined solution/APK builds with zero warnings/errors and all 20 runners pass.
Added 55 focused checks over the previous wave; the executable production contracts are used
rather than source-text checks or UI stubs. Assertions/scenarios are counted, not 8,898 distinct
test cases. Existing broad engine/encoding vectors remain separate from these failure regressions.

- Draft playback: transport identity is reserved before worker startup. A real delayed-worker
  loop test stops preview and verifies balanced MIDI note-on/off. Failed starts, stale external
  epochs, reentrant stops, late old completion, normal completion and failures exercise the
  same CompositionAudition owner used by Android. Transport tests also verify cancelled
  startup, reused-loop identities and invalid empty epochs.
- Draft storage: null nested payloads reject predictably. Malformed JSON, catalog capacity,
  duplicate keep choices, truncated files and failed publication retain existing material;
  failed publication cleans temporary payloads.
- Piano roll: source/channel changes cancel captured gestures before release. Tests cover
  group resize/quantize atomic undo, imported sub-grid notes, mid-gesture protections,
  cancelled/reverse box selection, invalid coordinates and shared phrase/MIDI bounds.
- Tap chopping: partial-preview start/end positions reject even after latency compensation.
  Tests cover invalid time/ranges, backward scrubbing, last-gesture undo, capacity reuse,
  foreign/stale documents and original WAV overwrite rejection.
- Sample library: corrupt sidecar numeric overflow and unreadable-file errors are isolated
  per asset. This Android browse filter was reviewed and compiled; host tests do not exercise
  Android file browsing, physical touch dispatch or lifecycle delivery.

The previous APK/test receipt is preserved in [initial 2.4.8 verification](../../archive/checkpoints/INTEGRATION-2.4.8-FIRST.md).
No phone was connected for installing or physically testing this audited APK.

## Follow-up integration — October 8, 2026

Composition draft MIDI audition, local named draft persistence and tempo/progression/rhythm
controls; retained sample browsing/reuse and tap chopping; and multi-note piano-roll edits
are integrated. Preview cleanup is wired to workspace/track changes, session adoption,
OnStop and OnDestroy, and only stops the owned draft transport epoch. A mixed source-text
encoding issue in composition labels was normalized to UTF-8.

Combined Sloop.slnx build passes with zero warnings/errors. All 20 compiled domain runners
pass: 8,898 executed checks/scenarios plus nine Python fixtures. Logs are under
`build/verification-audit/`. Extended focused runners: composition 504, sampling tools
70 and piano roll 87. Existing session, transport, sound, scenes and kit tests also pass.
The APK is built; ADB reports no connected devices, so this follow-up was not installed or
touch-tested. [Earlier Pixel and firmware checks](../../archive/checkpoints/INTEGRATION-2.4.7.md)
retain their original scope; hardware acceptance remains recorded in STATUS.md.

Final APK: `android/src/Sloop.Android/bin/Debug/net10.0-android/org.sloopfm.mobile-Signed.apk`,
44,988,805 bytes; SHA-256
`f7eacc8ebf8d5a4434d90dfbadd7463420cffa779c64efb924a4d3a0ce7a2100`.
The final label/encoding rebuild also passes with zero warnings/errors. The 2.4.8 installer
returns HTTP 200 and the served firmware hash matches the package recorded in release notes.

## Shared integration completed

- SessionPresetCodec stores bounded, versioned mapping/macro JSON, validates destinations,
  controller ownership, arrays/ranges/enums, and accepts older presets without those fields.
  PerformOptions was separated into a platform-free contract for direct codec tests.
- Named session export includes `.chopaudio`; archive and staged/startup validation enforce
  file-size/source-frame bounds. Default settings apply to sessions without the sidecar.
- Piano-roll protections constrain ordinary edits, prompts and composition. Workspace adoption
  resets local selections/protections; undo/redo preserves protected IDs. Sequence controls
  use actual loop length and PPQ; native editing rejects positions outside its 64 stored steps.
- Sound audition initializes a process-retained recovery service. Session adoption marks
  recovery and disables B/Keep against replaced local material. Disconnect remains conflicted
  after reconnect or late completion; no automatic writes occur. Ordinary Activity recreation
  retains acknowledged originals without retaining the Activity in callbacks.
- All 20 domain test projects are registered in Sloop.slnx and pass against the combined build.

## Where to use the new features

- Sample → Chop → Selected chop · pitch, gain & tuning; Send uses matching processed conversion.
- Sequence → App piano roll → Multi select for group edits.
- Sequence → Describe a sequence → Compose a new offline loop → Audition / Save named draft.
- Sample → Browse retained samples; Chop → Tap along & review.
- Perform → Settings → chord/joystick mappings and XY macro settings.
- Sound → FM1 A/B · reversible RAM audition (physical FM6 connection required).

## Honest limits

Pitch estimates require review; tuning changes playback speed/duration. Composition uses an
explicit vocabulary and procedural engine rather than a language model. Draft MIDI audition,
phone-local saving and multi-note piano-roll editing are integrated. Draft saving is outside
portable session archives. XY emits one CC pair and does not
record automation. A/B Keep means device RAM only, not flash bank storage; reconnect conflicts
require explicit abandonment because device identity cannot yet be proven. Atomic hardware
sound/pattern/tempo scenes are not available; sample flash dependencies cannot be live-atomic uploads.


## Firmware 2.4.9 held-chord latch shortcut — October 8, 2026

Real UI/audio harness passes both ARP and physical SEL/SLOOP SCL capture, MIDI-event preservation, voice gates, one-shot timing, latch-off key release, empty-chord and knob cancellation, plus the 20,000-frame fuzz run. Scale/keyboard regressions and target build pass. Release package and installer details are in [RELEASE-NOTES.md](../../RELEASE-NOTES.md).

Pixel 7a was connected and the audited Android APK installed successfully; cold launch took 876 ms and the app remained running. Perform, Sequence and offline composition controls were inspected through device UI dumps. No FM1 audio/MIDI or touch-performance behavior was measured in this live phone pass.


## Firmware 2.4.10 arp-plus-chord latch regression — October 8, 2026

The shortcut now accepts physical chords with ARP enabled. Twelve added checks exercise both ARP and physical SEL/SLOOP SCL buttons: three-note arp pool, preservation while toggling, once-per-hold, sustained arp pool after physical root release, unlatching while held, and cleanup on key release. Real UI/audio harness including 20,000 random frames, scale/keyboard regressions, and target build pass. Release metadata is in [RELEASE-NOTES.md](../../RELEASE-NOTES.md).


## Firmware 2.4.11 latched arp modifier regression — October 8, 2026

Fourteen new real UI/audio checks cover minor toggles after physical root release, persistent modifier quality, toggle-off from another octave, seventh changes while holding a root, expanded-chord release and extra-note cleanup. Both ARP and SEL/SCL capture paths pass, along with prior chord capture checks and 20,000 random UI frames. Scale/keyboard and full sequencer suites pass. Target build and package metadata are in [RELEASE-NOTES.md](../../RELEASE-NOTES.md).
