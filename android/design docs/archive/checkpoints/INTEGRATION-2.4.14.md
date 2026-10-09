# Current verification — October 8, 2026

The integrated Android solution builds with zero warnings/errors. All 24 domain runners
pass, including 47 groove client checks, 46 remote performance checks and 25 sound-bank checks.
Tests cover actual wire validation, truncated replies, unexpected acknowledgments, legacy
capability gates, cancellation/ownership and save readback rather than source-text matching.
Logs: `build/next-wave-domain-tests/` and `build/next-wave-android-build.log`.

Firmware 2.4.12 target checks pass: image 577,608 bytes, static RAM 93,332/98,304,
pool 334,560/344,064, RAM text 925 instructions with no calls; HAL register checks pass.
Focused host suites cover groove application/confirmation/undo/project roundtrip and actual
button routing; command 74 malformed/busy/apply behavior; remote owner expiry, panel priority,
sequencer boundaries and DSP; USB gain/mute/ramp/counters and capture ordering; harmony owner
aggregation/saturation; scale/latch and randomized UI behavior. C assertions are explicitly
enabled with `-UNDEBUG`: optimized Zig otherwise disabled some previous assertions.
The scale tests now inspect real plain-chord voice gates instead of an arp-only held-note field.

Existing target audio ISR cost checks still fail (`build/next-wave-target-budget.log`).
Budgets were not raised; a successful build does not establish audio deadline headroom.
Atomic scene transactions remain disabled; shared harmony ownership remains inactive.

The new APK was installed on Pixel 7a serial 36121JEHN07361. Cold launch succeeded in
991 ms. Offline Perform and Sequence UI checks use dumps in `build/next-wave-live/`.
The phone was connected to the PC, not an FM1: physical remote controls, bank saving,
groove transfer and USB return audio are not claimed as tested live in this checkpoint.
Firmware has been built and served, not flashed by this chat.

Earlier evidence: [2.4.11 checkpoint](../../archive/checkpoints/INTEGRATION-2.4.11.md).
Current package: [release notes](../../RELEASE-NOTES.md).

## Drum navigation correction — 2.4.13

EDIT/SEQ now enter the drum workspace from every starting page when Drums is selected.
New physical-event tests cover entry from SCL and ordinary sequence pages, all four SEQ
taps and individual SELECT detents through GROOVE. Both groove suites pass with assertions
enabled. Target image is 577,528 bytes; RAM/pool/RAM-text checks are unchanged and pass.
Logs: `build/drum-navigation-2.4.13-tests.log` and `build/firmware-2.4.13-build.log`.
This correction has not been flashed or physically verified by this chat.

## Expanded groove bank and preview — 2.4.14

The two groove harnesses pass with assertions enabled. Tests cover all sixteen ROM
entries, every long-pattern preview window, full bank wire capacity, Amen fourth-bar
hits/ghost levels, replacement undo, and a complete 64-step preview plus loop rollover.
Preview triggers exact lane masks without modifying captured project bytes or transport;
exit/STOP/recording/project/track/groove changes stop or refuse preview. Full UI regressions,
including randomized frames, pass. The phone client runner passes 48 checks, including
sixteen-entry decoding with a 64-step Amen pattern. No app update is needed for bank discovery.

Target build passes: image 579,656 bytes, RAM 93,428/98,304, pool 334,560/344,064;
RAM-text 925 instructions/no calls and HAL checks pass. Existing audio deadline limitations
remain; these are host functional checks, not physical listening measurements.
Logs: `build/groove-bank-2.4.14-tests.log`, `build/ui-groove-2.4.14-tests.log`,
`build/firmware-2.4.14-build.log`. Firmware built and served, not flashed by this chat.
