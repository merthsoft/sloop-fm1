# Web editor, installer, preview and build-tool review

**Implementation follow-up, 2026-10-23:** WEB-1 through WEB-7 are implemented
in the assigned sources. The original review below is historical; the final
section records the authorized implementation and supersedes its review-only
restriction. Generated releases remain parent-owned.

Reviewed 2026-10-23 against the current working tree at HEAD
`9fd9ddc2f45d57ed0647270d757c2ead7ba30a0a`, including the uncommitted
obsolete-icon/build-generator changes. This is a read-only production review;
only this report and `build/code-review-web-tools-51/` were written. The
assigned current target size was 581,296/584,336 bytes; I did not rebuild or
independently verify that firmware-size number. Other reviews and experiments
were running concurrently, so this is not an immutable repository snapshot.
No findings below recount the already integrated boot-table, preset, slot-size,
or icon savings in OPTIMIZATION, ARCHITECTURAL-HEADROOM and REDUNDANCY-AUDIT.

## Ranked confirmed findings

### 1. [P2] Disconnect resets a counter still owned by pending operations

**References:** `web/editor.html:5996`, `:6003`, `:6040`, `:6091`, `:8929`,
`:9012`, `:9052`.

Unplug during the initial INFO request in `load()`: `load` has incremented
`busy`, `disconnect()` closes the link and resets `busy = 0`, then the rejected
load executes `finally { busy--; }`. The resulting value is **-1**. A subsequent
successful reconnect increments/decrements that value without restoring zero.
Polling and reload processing test `busy` as a boolean and stay suppressed;
the panning wait loop can also wait indefinitely. An ordinary operation can
temporarily increment -1 to zero, allowing polling during that operation.
This is also relevant to other outstanding operations with the same decrement
pattern, not only initial load.

**Reproduction:** `node build/code-review-web-tools-51/editor-busy.mjs` runs
the actual extracted `load()` and `disconnect()` functions with a pending INFO
promise rejected by `link.close()`. It asserts and logs `busy === -1`.
This is a control-flow reproduction, not a browser/USB test.

**Minimal fix:** stop zeroing a counter whose outstanding tasks will still
release it, or make the counter/session ownership explicit and release only
within the session that acquired it. Preserve the existing disconnect UI
reset. If a session token is used, old async error handlers must also avoid
disconnecting/repainting a replacement session. A clamp alone can hide
overlapping operations rather than fix ownership.

**Regression:** disconnect during INFO and during a multi-request edit, drain
the rejected promises, reconnect, and assert the counter is zero and a polling
tick actually issues DUMP. Exercise reconnect before old cleanup completes.
The Node suite extracts the protocol block, while these connection/UI functions
are outside it; its passing protocol tests do not exercise this lifetime.

### 2. [P2] Installer page bypasses generic-loader recovery

**References:** `web/index_pkg.html:268`–`:273`, `:299`–`:305`;
`web/fm1ota.js:150`–`:169`, `:220`, `:259`.

For a device already stuck in update mode with the Windows name
`USB MIDI Device`, the page's initial `up.find(IS_OTA)` uses `deep=false`.
That name is intentionally outside `FM1_PORT`. The page therefore chooses
`install()`, which searches for running firmware and reports `notfound`.
`resume()` would find and finish this exact loader because it uses deep search,
but the page never reaches it. The stock-return button has the same initial
routing gap.

**Reproduction:** `repro.mjs` reuses the repository's FakeFM1 and actual
Updater. For that port name it verifies initial discovery returns null,
`install()` throws `notfound`, and direct `resume()` succeeds. See `repro.log`.

**Minimal fix:** use deep loader discovery in both page entry points (while
retaining their own-loader/official-return checks), or make the recovery attempt
itself the routing operation and avoid the duplicate discovery. Preserve the
foreign-loader rejection and the official-return exception.

**Regression:** execute the page click routing against a generic loader, for
both own-firmware recovery and stock-return recovery. Existing
`web/test_web.mjs:1735` tests direct `resume()` successfully, so it misses the
page dispatch failure.

### 3. [P2; potentially high consequence] Newly discovered loader is not checked before writing

**References:** `web/fm1ota.js:231`–`:233`, `:246`–`:250`, versus the explicit
foreign-loader check at `:261`–`:264`; `tools/fm1_install.py:322`–`:326`,
`:359`–`:365` (Python has the same post-transition gap).

After stage 1, `install()` accepts the first identity matching any `ota-`
loader and calls `write()` unconditionally. The foreign-loader policy applied
to resume is not applied here. With a second FM-1 loader available during the
transition, or a generic foreign loader skipped by the initial page check,
discovery can choose the foreign device. It then sends UPGRADE and serves the
image to a loader whose layout the source explicitly says may differ. This
also shows discovery is not bound to the original physical device. Final
firmware discovery similarly accepts the first running FM-1; an equal product
identity is not proof it is the device just flashed.

**Reproduction:** `repro.mjs` executes actual `install()` orchestration with
discovery/transport stages supplied by stubs. It supplies `ota-FM-1_015` after
an own-firmware start and verifies `write()` is called with that identity. No
hardware was written; cross-device damage has not been experimentally tested.
The foreign-resume test at `web/test_web.mjs:1791` covers a different branch;
the full-install fake always creates `ota-FM-1_900` after stage 1.

**Minimal fix:** enforce the expected loader family immediately before stage 2,
closing a rejected link. Own packages require OUR_LOADER; preserve the pinned
official-V15 installation path, which legitimately stages an official loader.
Do not require `ota-${product}` for own packages: the repository loader has a
fixed `ota-FM-1_900` identity (`firmware/loader/loader.c:15`). For multiple
matching physical devices, reject ambiguity or explicitly retain a selectable
device association rather than silently picking a different device. Apply the
same policy in the CLI.

**Regression:** inject a foreign loader after stage 1 and assert zero UPGRADE
or data responses to it; include two-loader ordering and generic port names.
Retain successful own-loader and exact official-V15 paths. Test a second
running device during final verification and avoid false success.

### 4. [P2] Deployment verifies firmware metadata but permits stale or unusable editor/audio assets

**References:** `tools/check_pages.py:7`–`:19`,
`.github/workflows/pages.yml:25`–`:35`, `web/make_site.py:157`–`:159`,
`tests/pages_deployment_test.py:25`–`:27`.

The deployment job publishes checked-in `docs` after checking three installer
metadata strings and the mere existence of `sloop-preview.wasm`. It does not
compare generated editor/installer code against `web`, validate required
preview JS/service-worker files, or bind the WASM to its firmware/generator
inputs. `make_site` copies any existing build/preview WASM without a provenance
check. A source-only fix can pass CI while Pages continues serving old code;
a fresh editor can likewise be paired with an old DSP build after rebuilding
firmware without rebuilding preview. A content-hashed service-worker stamp
identifies those bytes but does not establish that they belong together.

**Reproduction:** `pages-provenance.py` copies the three files that the checker
needs into a private site, supplies `b'not even wasm'` as the audio build,
an obsolete editor, and no worklet, preview.js or service worker. `check()`
still reports `Pages package verified: 2.7.1.49, 609912 bytes`.
This proves the gate gap; it does not prove the currently deployed WASM is stale.
Current deployed preview.js/worklet bytes equal source in this checkout.

**Minimal fix:** compare deterministic generated pages/assets against a private
regeneration before deployment. Have preview building record hashes of its
transitive firmware/generated inputs and compiler/options, and reject stale
preview input manifests when packaging/checking Pages. At minimum validate
required file presence and WASM structure, but those alone do not establish
freshness. Avoid trusting timestamps or a version string alone.

**Regression:** change web source while leaving docs untouched, omit a required
module, and change a DSP input while retaining the old WASM; all must fail the
deployment check. The existing passing fixture deliberately accepts arbitrary
`b'preview fixture'` bytes and only tests the missing-WASM case.

### 5. [P2] PWA regression test still assumes the old generated names

**References:** `tests/pwa_test.mjs:70`, `:79`–`:103`, `:124`, `:146`;
`web/make_site.py:118`, `web/editor.html:10`;
`tests/run_tests.sh:321`–`:322`.

The generator now writes `merthsloop-2.7.fwsc` for the test runner's version
argument and the editor title is `MerthSloop Editor`. The browser test still
asserts a `/firmware/sloop-*.fwsc` cache entry, the old editor title, and later
unconditionally reads `firmware/sloop-2.7.fwsc`. A generated site therefore
cannot pass the installed-Playwright path: it records false failures and reaches
ENOENT at that read. The title replacement used to test online cache refresh
also no longer changes anything. Missing Playwright exits zero before these
checks, and the Pages workflow does not run this browser test at all.

**Validation:** statically compared the actual generator output naming and
editor title against the executable assertions; `pages-provenance.py` includes
assertions for these mismatches. The browser itself was not run in this review.

**Minimal fix:** read the firmware URL/version from the generated installer or
service-worker file list, and update the intended product/title assertions.
Use a dedicated page marker for the update-cache test rather than an obsolete
branding literal. Ensure a CI job actually installs/runs the browser dependency
and distinguishes a deliberate skip from a pass.

**Regression:** run the existing browser sequence on a newly generated private
site, verifying offline installer/editor/audio assets, changed firmware under
the same URL, and refreshed-page fallback after the server stops.

### 6. [P2] Downloaded single-file editor retains external preview dependencies

**References:** `web/make_site.py:41`–`:60`, `:149`–`:150`;
`web/editor.html:5754`–`:5755`; `web/preview.js:31`–`:34`;
`tests/site_test.py:36`–`:38`.

The advertised single-file offline editor embeds its font and removes the PWA
parts, but retains `await import("./preview.js")`. Its mock-mode preview controls
still call that code. Downloading only `sloop-editor.html` and enabling audio
therefore attempts to import an absent neighboring module; the module also
requires an external worklet and WASM. Device editing can remain usable, but
the browser preview is not a self-contained part of this downloaded artifact.

**Validation:** `pages-provenance.py` calls the real `offline_editor()` and
asserts that its output retains the relative dynamic import/startPreview path.
No browser was run for this case. The passing single-file test checks relative
HTML `src`/`href` and CSS URLs, not dynamic JavaScript imports.

**Minimal fix:** include the preview modules, worklet and WASM in a supported
self-contained offline artifact, with the worklet loading route tested under
the documented downloaded-file context. If that delivery format cannot support
preview, clearly state that limitation at the preview control and point to the
offline-installed PWA; do not leave an apparently functional control that only
fails on an absent module. Preserve the device editor and its existing export.

**Regression:** open the downloaded artifact with no sibling files/network,
enter mock mode and start audio; assert an audible finite buffer or an explicit
supported-format handoff. Include JavaScript dependency URLs in offline-asset
validation, not only HTML/CSS links.

### 7. [P3] Updater error paths retain their MIDI handler

**References:** `web/fm1ota.js:226`–`:227`, `:249`–`:250`;
`tools/fm1_install.py:313`–`:315`, `:333`–`:335`.

`serve()` throws `badreq` on a bounds violation, or a supplied progress callback
can throw. Both callers close only on normal return. The browser Link callback
then remains attached, accumulating subsequent SysEx into an unconsumed queue;
the CLI also skips closing its backend link. This is separate from the
intentional decision to keep reusable WebMIDI ports open between probes.

**Reproduction:** the actual FakeFM1/Updater path in `repro.mjs` sends a request
past the image end, observes `badreq`, and asserts `onmidimessage` is still a
function. Existing bad-request tests check the error, not link cleanup.

**Minimal fix:** `try/finally` around serving each owned link, preserving the
current browser `Link.close()` semantics (detach callback, do not close reused
ports). Mirror backend cleanup in Python.

**Regression:** after bounds and callback errors assert the handler/backend is
released, no further packets accumulate, and a second attempt can reconnect.

## Size/speed and code-quality opportunities (unmeasured)

- `web/preview-worklet.js:30`–`:33` allocates two comparison arrays and performs
  two JSON serializations per step on each noninitial state sync. At 4 tracks
  × 64 steps that is 512 step serializations even for a one-knob update, plus
  locks/FM6 comparisons. This runs on the audio worklet's thread. Compare fixed
  step fields/arrays directly or send changed records; benchmark worklet edit
  bursts and audio callback deadlines before claiming a speedup. Do not move
  sample decompression into the rendering path.
- `web/preview-worklet.js:91` builds transport arrays and a string every audio
  callback, even when no position changes. Reuse fixed position storage and
  compare scalars if profiling shows a useful benefit. Preserve independent
  track playheads and update notification behavior.
- Installer/page discovery is duplicated across both buttons and resume,
  contributing to finding 2 and extra handshakes. A small shared discovery
  operation with explicit loader policy is sufficient; no generic transport
  framework is warranted.

No new target byte savings or measured browser speed savings are claimed.

## Coverage, validation and clean areas

| Area | Actually inspected / exercised |
| --- | --- |
| WebMIDI | Editor Link queue/close, fm1Ports, connect/disconnect/load/onState, updater discovery/handshake/serve/install/resume and page click routing. Private orchestration and disconnect reproductions. |
| Browser imports/state | Backup/song length+CRC checks, format preflight before restore, object ordering, sample restore routing, DSYN import normalization, mock state snapshots and parameter descriptors; broad existing protocol tests executed. No exhaustive parser fuzzing. |
| Preview parity | preview.js lifecycle, worklet state adapter/render/transport, firmware/host/preview.c parameter/step/lock/sample/kit entry points, build_preview.py and preview tests. Existing native/WASM binaries exercised without rebuilding. |
| Generation | make_site and service worker content stamps/keep list, optional WASM retention/removal; build.py current generation/packing and SDK checks; read gen_samples cache keys/sorted inputs, gen_waves seeds, drumkit/FM6 generators, pack_presets, pack_boot_tables and pack_uac_fir. Not a full line-by-line audit of every mathematical generator. |
| Packaging/install | fm1pkg_make app/config capacity arithmetic, CRC/encryption/directory layout, logical-image/product mapping, Python package_damage checks and updater flow; maximum-capacity encrypted package test executed. |
| CI/Pages | Entire Pages workflow and checker, checker unit tests, site generation tests and browser PWA test source. No deployment or network request performed. |

Private validation logs:

- `web-tests.log`: **web tests passed** (full `node web/test_web.mjs`).
- `preview-tests.log`: **32 native/WASM profiles matched with maximum delta 0**;
  sequence, live edits, playheads, uploaded samples and inline edits passed.
  These use pre-existing native/WASM binaries, so do not establish freshness
  against every current uncommitted DSP change.
- `pages-tests.log`: **5/5 passed** after directing temporary files into the
  private evidence directory. Initial default Windows temporary-directory
  sandbox errors were environmental and were resolved for this run.
- `site-tests.log`: **offline editor/site generation tests passed**. Their
  temporary sites were private; no shared docs regenerated.
- `capacity-tests.log`: **maximum extent, encrypted data/CRCs, oversize and SDK
  drift PASS**, using `build/deps/ac79`.
- `pages-check.log`: checked-in Pages metadata check passed; its limits are
  finding 4.
- `install-tests.log`: 25 preceding CLI/protocol assertions passed; the final
  Python/JS comparison failed on Windows because the test uses a `C:\\...`
  ES-module import specifier rather than a file URL (`tests/install_test.py:297`).
  No all-green CLI-suite claim. The full web suite separately passed JS/Python
  logical-image comparison on the available package.
- `repro.log`, `editor-busy.log`, `pages-provenance.log`: targeted findings
  reproduced/asserted as described above, including the retained offline import.

Useful existing protections include bounds checking each updater read,
checksumming requests, pinned official-V15 SHA-256/size/product validation,
per-file package CRC checks in the CLI, actual SDK-payload capacity checks,
restore length/CRC preflight and newer-project rejection before writes, and
native-versus-WASM audio comparisons using real firmware DSP. Keep these.
The service-worker generation hashes file contents deterministically and the
site generator removes an old output WASM when the build WASM is absent;
that is useful stale-file handling but not the provenance check in finding 4.

## Integration recommendation and limits

Integrate fixes for reconnect accounting and installer recovery/loader policy
before relying on unattended reconnect/recovery. Repair the PWA test and make
it run in a browser-equipped CI job, then strengthen source/artifact provenance
before treating the Pages gate as verification of a release. Add link cleanup
with the same focused updater tests. Production fixes were deliberately left
to parent integration; private files contain runnable reproductions only.

No real USB/WebMIDI device, multi-device hardware setup, browser audio deadline
profiling, SPL boot, physical flash interruption, or Pages rollout was tested.
No optional video-production files or demo assets were reviewed. The generator
review is selective rather than an exhaustive audit of all Python tools; ZIP,
WAV and MIDI parsers were exercised by the existing tests, not fuzzed. Current
test passes and the clean areas above are evidence of exercised behavior, not
proof of no bugs. No production files, shared outputs, Git refs, packages,
firmware, or other reviewers' reports were modified.

## Authorized implementation follow-up

Implemented after user approval, based on `270c0e3`, within the assigned web,
installer, provenance, workflow and test files. No firmware, version header,
target generator, tests/run_tests.sh, other report or shared output was edited.

| ID | Final change |
| --- | --- |
| WEB-1 | Both actual page buttons request deep discovery; generic Windows loaders enter the correct own/stock recovery path. |
| WEB-2 | Browser/CLI discovery counts responders before mode filtering and rejects ambiguity. Post-stage-one loader-family checks apply before writing. Own fixed `ota-FM-1_900` and exact official-V15 `ota-FM-1_015` paths remain supported. Final browser verification also checks ambiguity. The page requests one connected FM-1 throughout the update. |
| WEB-3 | Pending tasks retain/release busy increments; disconnect no longer resets their counter. Connection epochs and owned callback cleanup prevent stale discovery/load errors from disconnecting a replacement session. Initial-load, step, lock/mixer and queued legacy-panning continuations retain device ownership. |
| WEB-4 | New web_provenance records source/asset hashes, generated-header hashes, compiler/options and WASM hash. Preview build rejects changing inputs during compilation. make_site validates provenance before replacing outputs and copies the manifest. check_pages privately regenerates/comparers all generated bytes and rejects retained assets. Unit fixtures require no target compiler. |
| WEB-5 | PWA test derives current metadata, uses a portable Node server and persistent browser profile, handles install-prompt timing and error-page navigation, restores fixtures, and tests cached preview modules/compilable WASM. Missing Playwright exits 77. Pages CI installs pinned Playwright 1.62.1/Chromium and runs browser/recovery tests. |
| WEB-6 | Download descriptions and preview controls explicitly explain that offline audio uses the installed PWA. A generated standalone-artifact marker disables audio controls and guards all start paths before importing absent modules, including when that single file is hosted. Device editing and complete-site/PWA preview remain available. |
| WEB-7 | Serving and discovery release owned browser/CLI links on errors, including progress callbacks. Browser close still detaches the handler without closing reusable ports. |

Companion test maintenance aligns the mock settings-restore guard with the
parent PERSIST-1 fix, testing unchanged settings through real begin/data/commit
messages during playback. The existing source assertion follows the new guard.
CLI test imports now use file URIs on Windows. Preview comparisons accept
PREVIEW_WASM/PREVIEW_NATIVE overrides while retaining their default paths.

### Validation and evidence

Evidence is exclusively under `build/code-review-web-tools-51/`:

- `recovery-before.log`: the new actual-page regression fails against a private
  `270c0e3` source snapshot with generic-loader `notfound`, as expected.
  `run-before.py` reproduces that comparison. Original `editor-busy.log` records
  the unchanged-source counter failure (-1).
- `recovery-fixed.log`: generic own/stock page recovery, foreign stage-two
  rejection, full official-V15 installation, ambiguity cleanup, serving/callback
  errors, balanced pending load, stale failure versus replacement session and
  unchanged-settings mock parity all pass.
- `cli-lifetime-fixed.log`: 5/5 focused tests pass. `install-fixed.log`: full
  existing installer suite passes, including Windows Python/JS package parity.
- `web-fixed.log`: full web protocol/sample/package/updater suite passes.
- `pages-fixed.log`: 13/13 deployment tests pass without target compilation,
  covering fresh generation, stale web/DSP inputs, absent modules/manifest,
  invalid/changed WASM and retained package/web assets.
- `site-fixed.log`: standalone and offline site generation checks pass.
- `preview-fresh.log`: newly compiled private WASM/native DSP matches all 32
  profiles at maximum delta 0, with live edits, samples, sequencer/playheads and
  inline-edit checks passing. Parent separately owns final .50 native/WASM
  release validation.
- `site-preview.log`: private site packaging accepts the parent's fresh .50
  package/preview. `private-release-check.log`: the real checker, run against
  a private copy of current source inputs and generated site, reports
  **Pages package verified: 2.7.1.50, 610001 bytes**.
- `pwa-fixed.log`: real installed Google Chrome / Playwright 1.62.1 passes the
  complete private **2.7.1.50** browser sequence, including installability,
  offline firmware/editor/font/modules/DSP, aliases, cache replacement,
  same-URL firmware refresh and downloaded-file preview limitation.
  Chromium loopback needed authorized unsandboxed execution after the sandbox
  returned ERR_NETWORK_ACCESS_DENIED. `pwa-skip.log` separately verifies exit
  **77**, not pass, without the dependency. Assigned-file diff whitespace check
  is clean.

### Exact parent hooks and browser paths

Provenance hooks are stable. No tools/build.py hook is necessary: once the
version and firmware/generator/sample sources are final, build preview, then
generate the site with its preview directory, then run check_pages. Subsequent
input changes intentionally require a fresh preview. The required sidecar is
`sloop-preview.inputs.json`; rebuild old outputs rather than hand-editing it.
Parent reports final docs regeneration/check_pages and fresh native/WASM parity
already pass. This chat did not regenerate those shared outputs.

```text
python tools/build_preview.py [--out PRIVATE_PREVIEW_DIRECTORY]
python web/make_site.py PACKAGE VERSION OUTPUT --preview-dir PREVIEW_DIRECTORY
python tools/check_pages.py
python tests/pages_deployment_test.py
python tests/web_installer_lifetime_test.py
node tests/web_recovery_test.mjs
```

Successful installed-browser configuration on this Windows host:

```powershell
$env:NODE_PATH='C:\Users\shaun\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\node_modules'
$env:PWA_BROWSER='C:\Program Files\Google\Chrome\Application\chrome.exe'
node tests/pwa_test.mjs docs
```

The installed runtime is Playwright **1.62.1**. Bundled Node executable:
`C:\Users\shaun\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe`.
The regular `node` also worked. Optional PWA_PROFILE_ROOT controls private
profile placement (default: site's parent); profiles remain for inspection.
Run against a private site when avoiding fixture writes to docs. The test
restores changed files in finally. Parent tests/run_tests.sh already invokes
the new tests and handles browser exit 77 distinctly; required CI installs the
browser and fails if it cannot run.

No hardware flash/WebMIDI or multiple physical-device test was performed. The
protocol lacks a stable serial across re-enumeration: ambiguity is rejected at
discovery boundaries and one connected FM-1 is required, but swapping otherwise
identical devices between discoveries cannot be proven detectable. Explicit
CLI --port selection is preserved. No new firmware size/speed claim is made.
The standalone artifact deliberately delegates audio to the PWA. No commit,
push, version bump, shared release regeneration or publication was performed.
