# UI reuse audit: parameter controls and page navigation

The recommended patch is now integrated with the drawing and held-page candidates. [Actual combined measurements and validation](UI-REUSE-INTEGRATION.md) supersede standalone savings for the production build; the audit evidence below retains its frozen .98 baseline.

The frozen dirty MerthSloop **2.8.0.98** tree has a measured **256 B** reuse candidate: **584,288 -> 584,032 B**, leaving **304 B** in the 584,336 B application slot. It changes no controls, visible labels, page order, saved/wire parameter IDs or static RAM. This is an isolated review patch, not an integrated or installed build.

## Frozen scope and measurements

Read FLASH-91-INTEGRATION.md, FLASH-91-UI-AUDIT.md and DUPLICATE-CODE-AUDIT.md before selecting candidates. Existing cold drawing wrappers, parameter cards, runtime-field resolution, startup literal packing and earlier DSP deduplication are already integrated and are not claimed again.

The audit froze the current dirty firmware, tools, tests, generated headers and three pinned SDK inputs under `build/ui-reuse-98/controls/source/`, never exported HEAD. `manifest.json` and `frozen-check.json` cover **385 unchanged files**. Only this report and private audit files were written. Production sources, shared build outputs, installer, version and Git were untouched. No data decoding, RAM caches, compiler flags or feature removals finance the gain.

Private `run.py` calls only the snapshot's `build_app()` and `check()`, then the target-budget checker. It does not generate loaders, packages or a site. Every target invocation explicitly uses `FELUCCA_AUDIO_DIAG=0`, existing production flags and pinned `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`. Result directories retain commands, symbols, sections, disassembly, checks, budgets and SHA-256.

| Variant | Linked image B | Saving vs baseline B | Free flash B |
|---|---:|---:|---:|
| Frozen dirty .98 | 584,288 | -- | 48 |
| Shared parameter-turn arithmetic | 584,176 | 112 | 160 |
| Previous + one HOME macro identity resolution | 584,064 | 224 | 272 |
| Previous + division-label prefix reuse | **584,032** | **256** | **304** |
| Confirmation-arm helper alone | 584,288 | 0 | 48 |
| Explicit page-policy outlining alone | 584,288 | 0 | 48 |
| Turns + explicit page-policy outlining | 584,176 | 112 | 160 |

The first three savings are cumulative measured links; the 112/32 B incremental deltas are not standalone claims. All rows retain main static **97,612 / 98,304 B**, low **28,928 / 29,392 B**, pool **335,424 / 344,064 B**, and `.noinit` **15,472 B**. Main free is 692 B; low free 464 B; pool gap 8,640 B retains the existing 8,192 B reserve. Boot literal destinations/payload are unchanged.

Baseline application SHA-256: `2e8ab6189ef96ba24565961c2d8e2b5d3659deabd1e32f1a0d483df266ffd238`.

Final application SHA-256: `31ea5e30b80c3faa489d90992cb6838b9e4b31923b74de6c2288186205f3356a`.

## Recommended changes

Patch: **`build/ui-reuse-98/controls/final.patch`**. Complete candidate: `division-source/`. Only `firmware/src/ui_input.c` and `firmware/src/params.c` differ. Apply against the frozen .98 source in a review copy, normalizing mixed C line endings to LF if needed. Do not apply rejected arm/page-policy experiments along with the patch.

**Share ordinary encoder arithmetic.** Candidate `ui_input.c:304` defines `param_turn`, the exact previous clamp/acceleration expression. TRACKS edits, normal parameter edits and HOME macros call it at lines 339, 577 and 1429. Those sites duplicated descriptor range/name handling and clamp setup. The special ESYNC/bit-depth/rate `names_turn` branch remains in `edit_param`; existing action, undo, MIDI, lock and runtime-family side effects remain with callers. The helper is 52 target bytes. `tracks_edit` falls 182 -> 142 B and `edit_param` 960 -> 942 B; other call-site savings and final layout produce **112 B net**.

**Resolve HOME macro identity once.** At candidate `ui_input.c:1431`, engine/macro lookup is stored in an automatic `uint32_t id`, reused for parameter-lock selection and MIDI learn. The same expression previously occurred three times around calls. Track/engine ownership cannot change between these operations: `p_lockable` only classifies an ID and MIDI learn receives it afterward. Synth-only ownership stays explicit; drums still do not learn/select a synth parameter. Incremental final-link gain: **112 B**.

**Reuse subdivision names.** `params.c` removes six-pointer `N_DIV`, which duplicates the first six entries of `N_SDIV`. ARP RATE and DUCK RECOVERY point to `N_SDIV` with explicit max `NDIV_SHORT-1` (5), while sequencer DIV retains max 8 (candidate lines 101/205). Enum indices, names, defaults, limits and persistence IDs remain identical. No external pointer-identity consumer used `N_DIV`; special name-pointer handling only concerns ESYNC/IBITS/IRATE. The deleted pointer table is 24 B; incremental linked reduction is **32 B** including layout.

## Existing sharing and rejected options

The baseline emits one **434 B `page_desc`**, **60 B `page_shown`**, **86 B `page_for_drum`**, **128 B `open_family`**, and **156 B `page_walk`**. Explicit `minsize,noinline` on page visibility/eligibility yields a **byte-identical application**: the compiler already shares these policies.

`PAGES` is already a compact **636 B** recipe table used by knob resolution, card drawing, help and family navigation. DRIFT/DRONE/SUSTAIN/MORPH values share one contiguous-scope resolver with explicit strides. `track_desc` centrally selects engine-dependent labels/ranges. TP/GP tables are 1,136/992 B and carry distinct meanings, labels, formats and limits; they are not copy-pasted page implementations.

Three repeated confirmation blocks (`ui.arm`, 90-frame timeout, AGAIN message) looked promising in source. Extracting `param_arm(action,label)` yields **zero net flash savings**. Retain current ownership rather than claim source-line deletion as a firmware improvement.

Navigation paths intentionally differ: family taps wrap, SELECT clamps at ends/excludes native drums, and `page_fix` falls back after engine visibility changes. A single general navigator needs explicit policies to preserve those semantics; emitted sizes do not support claims of large duplication here. Undo sessions, browse audition, action confirmations, lock/MIDI identity and dynamic field bindings likewise cannot be replaced by generic pointer writes without ownership review.

## Validation and costs

Pinned Zig 0.13 host builds have assertions enabled. All ten final programs pass: `audit_param_turn`, `audit_home_identity`, `ui_pages_test`, `ui_shared_paths_test`, `upstream_controls_ui_test`, `seq_navigation_test`, `duck_ui_test`, `track_fx_ui_test`, `delay_ui_test`, and `ambient_drift_ui_test`.

- **218,120 exact parameter comparisons:** every TP/GP descriptor, four knob roles, positive/negative/stationary deltas, acceleration on/off, initial/continuing/reversed/slow history. Returned values and touched encoder acceleration words match the original expression. Short/long subdivision range and shared-name assertions also pass.
- **HOME identity:** every engine's four actual knob edits preserve lock IDs and MIDI-learn IDs/track ownership. Drums remain excluded (`identity-host/audit_home_identity-default.log`).
- **79 byte-identical PPM captures** between baseline and final `ui_pages_test`; `pixel-comparison.json` records them. The real suite passes its **20,000-frame fuzz** plus pages, layer holds/locks, drums, REC, SONG, undo/MIDI and navigation checks.
- Target placement, RAM and hot-loop budgets pass for every linked row. No audio-loop calls or sound data change.

Initial delay/ambient invocations omitted their screenshot directory and stopped at the harness `ppm` file assertion. Rerunning the same binaries with a private existing output directory passed (`final-host/*-rerun.log`). Original failed logs remain; source or test expectations were not changed to hide failures. `host.py` now supplies those arguments for reproduction.

`param_turn` adds a cold encoder-rate call layer; its target prologue saves `{rets,r7-r4}` (**20 B**) and restores it before tail-calling the existing clamp. The existing acceleration call remains one per edit. Deepest live stack can grow by this helper frame. HOME ID is an automatic scalar; subdivision reuse adds no startup restoration. No hardware speedup is claimed. Physical stack high-water and hardware UI qualification remain integration checks.

## Unmeasured follow-ons

A resolved-field model containing descriptor/value pointer plus explicit format policy and action/learn identity could reduce future special-case duplication between drawing/help/input. This was not implemented or measured. The existing 434 B resolver is already shared; a larger recipe/callback layer must pay its own ROM/dispatch cost. Historical multi-KiB UI estimates are not measured gains.

HOME/SELECT tempo arithmetic could potentially use the shared helper after making encoder role explicit; this is outside the measured patch. Preserve owner-specific acceleration ranges and state. Independent agents audit drawing and held/menu pages. Combine patches and repeat a linked target build instead of summing reports.

## Reproduction

```powershell
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/code/sloop-fm1/build/ui-reuse-98/controls && FELUCCA_AUDIO_DIAG=0 python3 run.py baseline-repeat'
wsl -d Ubuntu -- bash -lc 'cd /mnt/c/code/sloop-fm1/build/ui-reuse-98/controls && FELUCCA_AUDIO_DIAG=0 AUDIT_SOURCE=division-source python3 run.py final-repeat'
```

Private `host.py` accepts `AUDIT_SOURCE=division-source`, `AUDIT_NAME=final` and comma-separated `AUDIT_TESTS` selecting the ten programs above. The explicit HOME identity suite has its retained separate `identity-host/` result. Never run audit experiments against shared production outputs.
