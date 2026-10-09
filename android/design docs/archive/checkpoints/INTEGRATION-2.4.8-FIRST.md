# Archived verification — initial 2.4.8 follow-up, October 8, 2026

Firmware 2.4.8 adds latched modifier toggles. Host scale/keyboard tests cover release
persistence, second-press removal (including the other octave), independent combined qualities,
new-root persistence, per-synth isolation and latch/CHROM/STOP cleanup. Sequencer regressions
pass, including unchanged momentary modifiers. Target image is 571,500 bytes with unchanged
92,948-byte static RAM and 925-instruction call-free RAM text. Package details are in
[release notes](../../RELEASE-NOTES.md). This verification did not flash hardware.

Hardware atomic scene staging remains a tested scaffold, disabled in production until complete
engine revision/scheduling/application hooks are available. Existing stopped-only transfer stays usable.

## Follow-up integration — October 8, 2026

Composition draft MIDI audition, local named draft persistence and tempo/progression/rhythm
controls; retained sample browsing/reuse and tap chopping; and multi-note piano-roll edits
are integrated. Preview cleanup is wired to workspace/track changes, session adoption,
OnStop and OnDestroy, and only stops the owned draft transport epoch. A mixed source-text
encoding issue in composition labels was normalized to UTF-8.

Combined Sloop.slnx build passes with zero warnings/errors. All 20 compiled domain runners
pass: 8,843 executed checks/scenarios plus nine Python fixtures. Logs are under
`build/verification-next-wave/`. Extended focused runners: composition 481, sampling tools
57 and piano roll 71. Existing session, transport, sound, scenes and kit tests also pass.
The APK is built; ADB reports no connected devices, so this follow-up was not installed or
touch-tested. [Earlier Pixel and firmware checks](INTEGRATION-2.4.7.md)
retain their original scope; hardware acceptance remains recorded in STATUS.md.

Final APK: `android/src/Sloop.Android/bin/Debug/net10.0-android/org.sloopfm.mobile-Signed.apk`,
44,984,709 bytes; SHA-256
`e7ac99b01d87e0b6a64f278a6d57b17b2d15993fcd38effc75565b02a994393c`.
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
