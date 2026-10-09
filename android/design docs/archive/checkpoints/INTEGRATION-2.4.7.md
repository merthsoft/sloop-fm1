# Integration verification — 2.4.7, October 8, 2026


All 20 registered domain runners passed after fixing null-entry rejection in chop sidecars.
Executed totals: 8,728 checks/scenarios plus nine Python encoding fixtures (the two 79-case
kit mapping runs are counted separately). Logs: `build/integration-Sloop.*.Tests.log`.

| New or extended runner | Passing checks |
| --- | ---: |
| Sampling pitch/audio | 27 + 79 kit regressions |
| Piano roll | 44 |
| Offline composition | 423 |
| Performance controls | 3,024 |
| Sound audition/recovery | 40 |
| Scene transaction scaffold | 25 |
| Session settings/chop adoption | 21 |

Combined solution build: zero warnings/errors. APK installed and cold-launched on Pixel 7a
(1,258 ms). Device checks reached piano-roll controls, generated a C-minor/seed-42 four-bar
92-note draft with keep/edit/review/regenerate/discard actions, then discarded it; XY ranges,
curves and release defaults were reachable and settings were cancelled. No user material was
applied or overwritten. The phone subsequently locked; remaining UI checks are not a renewed
hardware acceptance gate. Final APK rebuild includes the archive rejection fix, passes with
zero warnings/errors and is installed successfully. APK: 44,853,637 bytes; SHA-256
`7abb4d5a255569f82ea13f57bcb6f7e5f7eb997e568d355a0b6a69e03a3d653b`.

Firmware 2.4.7 Merthsoft passes fresh target build, scale/chord/latch/project/sequence tests,
scene scaffold enabled/disabled tests, USB playback/loader variants and UI 20,000-frame fuzz.
Production scene extension defaults off; INFO remains protocol 10. See [RELEASE-NOTES.md](../releases/RELEASE-2.4.7.md) for
package metrics and the verified local installer.
