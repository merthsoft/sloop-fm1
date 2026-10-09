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
