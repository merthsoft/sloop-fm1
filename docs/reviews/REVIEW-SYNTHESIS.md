# Review synthesis and integration order

This consolidates four isolated headroom audits and eight code reviews of the
working source based on `9fd9ddc`, including the icon cleanup subsequently
committed as `43d4a6f`. The reports were committed in `96affc0`. This document
does not implement their proposed fixes or experimental patches.

## Conclusions

The reviews found useful remaining space savings, but no large pool of
interchangeable copied algorithms or dead linked sections. The strongest new
flash candidates are sparse punch-state startup initialization and a bypassed
TRACKS column renderer. Direct legacy conversion also recovers substantial
main RAM without removing legacy compatibility.

Correctness work should precede new feature integration. The eight reviews
report **25 actionable findings**: one P1, 23 P2 and one P3, with different
levels of validation. Several are reproduced with real production components;
others are source-confirmed Android/browser lifecycle or test/deployment gaps.
These are not 25 hardware-reproduced failures. Preserve that distinction when
tracking completion. Existing broad test passes do not disprove the new edge
cases, and incomplete review coverage is not evidence of correctness.

Parent synthesis read all twelve reports and spot-checked the current settings
restore guard, side-record verification, loader completion code, private patch
contents and baseline measurement. It did not independently rerun every
specialist reproduction. Findings below retain their original evidence limits.

## Measured headroom candidates

All new experiments compare independently with **581,296 / 584,336 B**, the
source after the existing **3,040-byte icon cleanup**. Do not count that cleanup
again or sum the rows into a promised combined saving. Final compiler global
layout, alignment, helper code and generator integration can change the result.

| Candidate | Measured flash saving | Main RAM change | Recommendation |
| --- | ---: | ---: | --- |
| Punch + punch-filter state in BSS, original nonzero values assigned before startup | 1,488 B | −16 B | High value. Integrate an explicit common host/target initialization contract, then remeasure. |
| Delete bypassed old TRACKS columns | 496 B | 0 | First simple deletion. Real dispatcher already uses the NEON TRACKS renderer; 113 captures match. |
| Direct legacy conversion + shared validation/bank/select/init helpers | 256 B | −2,584 B | Useful RAM recovery. Keep compatibility; review durable selection changes separately if desired. Direct conversion alone saves 48 B flash and the same RAM. |
| Ragged preset-trim rows | 192 B | 0 | Straightforward representation improvement, but integrate through the generator and preserve measurement comments. |
| Fixed font metrics with proportional fallback | 144 B | −448 B | Small flash/RAM win. Generator-backed integration and full UI parity still required. |
| Boot-expand grain window + FM6 envelope table | 448 B | +816 B | Optional RAM-for-flash trade using the existing decoder before audio starts. Prototype needs integration into the normal generation/init list. |
| Boot-expand factory sample-zone metadata | 528 B | +1,424 B | Later option. This is metadata, not sample/audio decompression; retain direct sample playback. |
| Full storage helper combination including notification/settings tails | 416 B | −2,584 B | Alternative to the 256 B combination, not additional. Needs actual editor commit/WATCH failure tests. |

The punch saving is partly compiler-layout dependent: only 224 bytes are moved
out of the data load payload; changed global placement accounts for much of the
remaining code reduction. Its frozen audio output and hot-loop budgets match,
but that does not guarantee an identical delta after integration. Host tests
must receive the same startup initialization as hardware.

Reject low-value or disproven shortcuts: generic section GC saves **zero**;
`-fdata-sections` saves only 8 B; blanket descriptor packing saves 16 B in the
tested batch; browser-footer sharing saves 16 B. Stripping symbols changes
metadata, not installed load bytes. Callbacks and assembly-referenced handlers
are live. No partition expansion, sample removal, runtime sample decoder or
toolchain replacement is justified by this round.

Evidence: [UI audit](../firmware/HEADROOM-UI-AUDIT.md),
[link/state audit](../firmware/HEADROOM-LINK-AUDIT.md),
[storage audit](../firmware/HEADROOM-STORAGE-AUDIT.md),
[assets audit](../firmware/HEADROOM-ASSETS-AUDIT.md).

## Correctness findings, deduplicated

IDs below are synthesis tracking IDs. Original reports contain exact source
anchors, triggers, reproductions and proposed regressions.

| ID / priority | Finding and consequence | Evidence / source |
| --- | --- | --- |
| PERSIST-1 / P1 | Settings restore with unchanged arrangement can write flash during playback, silencing/interrupting audio. | Extracted unchanged restore function reproduction plus inspected production dispatch/backend. [Persistence F1](REVIEW-PERSISTENCE.md). |
| PERSIST-2 / P2 | Side-record bytes are not read back before header commit; silent program failure can report a successful save while losing local preferences/theme/favourites. | Real storage implementation with successful-return/dropped-program fault injection. [Persistence F2](REVIEW-PERSISTENCE.md). |
| UPDATE-1 / P2 | Loader ignores record-cleanup read/erase failures and can claim completion while the persistent update record remains. | Actual loader with fake-NOR erase failure. [Hardware 1](REVIEW-HARDWARE.md). |
| UPDATE-2 / P2 | Loader accepts a framed negative final host body instead of requiring `success\0`. | Actual loader fake host; original test still passes with the negative body. [Hardware 2](REVIEW-HARDWARE.md). |
| WEB-1 / P2 | Installer page does not request deep discovery, bypassing recovery for a generic Windows loader name. | Real updater/FakeFM1 orchestration; direct resume succeeds while page route fails. [Web 2](REVIEW-WEB-TOOLS.md). |
| WEB-2 / P2 | Browser/CLI post-stage-one discovery can write to a loader family that resume would reject; multiple devices also lack reliable association. | Stubbed discovery around real orchestration; no hardware write. [Web 3](REVIEW-WEB-TOOLS.md). |
| SEQ-1 / P2 | Phone step writes omit live-ratchet reconciliation; a changed pitch can sound outside the four-note release ownership list and survive REST. | Unchanged extracted wire helper plus actual scheduler; not full SysEx dispatch. [Sequencing 1](REVIEW-SEQUENCING.md). |
| SEQ-2 / P2 | Arp modifier revoice overwrites raw-key release pitches without reconciling raw MIDI. Root-first release strands the old tone. | Real physical event path and per-channel/pitch balances. [Sequencing 2](REVIEW-SEQUENCING.md). |
| SEQ-3 / P2 | Changing output channel while holding physical synth/drum/chord keys sends release on the new channel, stranding the old one. | Real keyboard/channel-change path. Generated-note ownership does not cover raw keys. [Sequencing 3](REVIEW-SEQUENCING.md). |
| SEQ-4 / P2 | Releasing one overlapping arp chord removes a shared pitch still owned by another key. | Actual C+G physical-key source-pool trace. [Sequencing 4](REVIEW-SEQUENCING.md). |
| AUDIO-1 / P2 | SPRING FREEZE return × gain can overflow before the master limiter; crossfade ranges also need checking. | Actual hot-input spring/FREEZE stress and arithmetic oracle. Ordinary-preset frequency not established. [Audio 1](REVIEW-AUDIO.md). |
| AUDIO-2 / P2 | PLAY → bypass → STOP leaves LOFI noise envelope stale; re-enabling while stopped emits noise. | Public-control sequence using production audio path. [Audio 2](REVIEW-AUDIO.md). |
| ENGINE-1 / P2 | MIDI pitch zero is treated as empty mono state, breaking fallback and legato retrigger decisions. | Production allocator reproduction. [Engines 1](REVIEW-ENGINES.md). |
| ENGINE-2 / P2 | Several rebuilt oscillator/sample increments discard the caller's fine-pitch factor, weakening tuning/glide/unison consistency. | PHASE, LOFI and SAMPLE reproduced; ANALOG source-backed; GRAIN/NOISE extensions need focused checks. [Engines 2](REVIEW-ENGINES.md). |
| APP-1 / P2 | Failed note send leaves LiveNotes owner/count state inconsistent and can remove panic's cleanup obligation. | Actual C# class with injected sender exception. [Android core 1](REVIEW-ANDROID-CORE.md). |
| APP-2 / P2 | Repeated song-load handshakes replace the hardware renewal owner without stopping the prior timer/task. | Actual Android adapter source compiled with a platform shell; cancellation ownership reproduced, battery cost not measured. [Android core 2](REVIEW-ANDROID-CORE.md). |
| UI-1 / P2 | Activity recreation during SysEx picking loses the captured synth destination and defaults to Synth 1. | Source-confirmed lifecycle flow; Android recreation test pending. [Android UI 1](REVIEW-ANDROID-UI.md). |
| UI-2 / P2 | Activity recreation during export loses pending payload/cache ownership and can leave an empty chosen document. | Source-confirmed; provider/recreation test pending. [Android UI 2](REVIEW-ANDROID-UI.md). |
| UI-3 / P2 | Async sample preparation/pitch continuations can show dialogs on a destroyed activity. | Source-confirmed missing lifetime/identity checks; device crash not induced. [Android UI 3](REVIEW-ANDROID-UI.md). |
| UI-4 / P2 | Canvas pads and hardware-gesture buttons expose no functional accessibility activation path. | Source-confirmed event/action mapping; TalkBack test pending. [Android UI 4](REVIEW-ANDROID-UI.md). |
| WEB-3 / P2 | Disconnect resets `busy` while pending operations still decrement it, leaving −1 and suppressing polling after reconnect. | Actual extracted load/disconnect functions with rejected pending request. [Web 1](REVIEW-WEB-TOOLS.md). |
| WEB-4 / P2 | Pages verification accepts stale editor code and invalid/incomplete preview assets; preview provenance is not bound to source. | Checker accepts intentionally invalid private site. Not proof current deployed WASM is stale. [Web 4](REVIEW-WEB-TOOLS.md). |
| WEB-5 / P2 | Browser PWA test uses obsolete Sloop title/package names and can skip before exposing failures if Playwright is missing. | Generator/assertion mismatch confirmed; actual browser run pending. [Web 5](REVIEW-WEB-TOOLS.md). |
| WEB-6 / P2 | Downloaded single-file editor retains external preview dependencies that are not included. | Actual offline artifact inspected; standalone browser audio test pending. [Web 6](REVIEW-WEB-TOOLS.md). |
| WEB-7 / P3 | Updater error paths fail to detach/close their owned MIDI links. | Actual malformed-request fake-device reproduction; CLI source traced. [Web 7](REVIEW-WEB-TOOLS.md). |

The six updater/installer observations are related but not duplicates:
page routing, loader-family/device selection, protocol finish semantics,
persistent record cleanup, link lifetime and artifact provenance fail at
different boundaries. Similarly, raw MIDI channel ownership and arp chord
revoice overlap, but fixing either alone does not fix the other.

## Proposed integration batches

1. **Durability and recovery:** fix PERSIST-1/2 and UPDATE-1/2; fix WEB-1/2/7
   with end-to-end page/CLI routing tests. Verify unchanged erase ranges,
   preserved resume records on failure, retries and official stock-return policy.
2. **Secure the budget:** retain the committed icon cleanup, delete dead TRACKS
   columns, integrate punch BSS with shared startup semantics, then measure the
   combined target. Introduce direct legacy conversion for RAM if warranted.
   Keep generator-backed trims/metrics as independent follow-ups. Fix the punch
   FREEZE arithmetic before accepting final punch output/cost measurements.
3. **Live musical ownership:** address SEQ-1 through SEQ-4 as separate changes,
   sharing only proven primitives. Assert note balances by channel and pitch at
   REST/release boundaries before STOP; include both key-release orders and
   physical, MIDI, latch and progression source transitions.
4. **Audio and pitch:** fix stale LOFI envelope and note-zero handling. Correct
   fine-factor propagation deliberately, preserving fine=0 results and decode
   caps; review any intended golden changes rather than regenerating blindly.
   Measure widened FREEZE arithmetic on the target, not just the host.
5. **Android ownership/lifecycle:** repair sender-failure cleanup and renewal
   task retirement, then pending import/export identity, destroyed-activity
   continuations and accessibility. Use an emulator for recreation/provider
   regressions; domain tests alone cannot prove window lifetime behavior.
6. **Release assurance:** repair PWA tests, actually run browser-equipped CI,
   bind generated pages/preview to source inputs and clarify or implement the
   single-file preview contract. Regenerate shipped assets only after fresh
   native/WASM parity checks.

No new upstream controls should be folded into these fixes incidentally.
Punch fade, adjustable filter resonance, step advance, drum preview and noise
drum tuning remain the separate [controls plan](../firmware/UPSTREAM-CONTROLS-REVIEW.md).

## Speed, clarity and test quality

No review measured a new hardware speedup. The TRIO engine can potentially use
the supplied fine factor instead of reconstructing it through a divide; its
698,368-input arithmetic sweep matches, but target/render validation remains.
Other unmeasured leads are repeated drum-insert coefficient preparation,
worklet JSON/allocation work, rebuilding unchanged pattern events, sample peak
scanning and repeated UI-side sample-settings reads. Profile before changing
them; do not book source-level simplifications as measured CPU improvements.

Clarity improvements should make ownership explicit: pending picker requests,
renewal task cancellation, note send ambiguity, remembered output channels,
commit/readback stages and common startup initialization. A broad UI, scheduler
or serialization framework is not justified by the audits. Shared algorithms
already serve many paths whose musical/data semantics differ.

Strong existing tests include upstream binary fixtures, byte-exact audio and
native/WASM comparisons, note-chain transaction invariants, fake-NOR torn-write
tests, source ownership checks and pixel/redraw comparisons. Preserve them.
New tests should target the demonstrated missing transitions rather than
merely increase fuzz counts. In particular:

- Exercise actual editor begin/data/commit and phone step commands, not only
  extracted helpers or host configurations that exclude durable settings.
- Inject successful-return/wrong-content flash writes as well as explicit errors.
- Check intermediate silence and per-channel/pitch ownership before panic/STOP.
- Test activity recreation, actual accessibility actions and page click routing.
- Treat missing browser dependencies as a skip, not evidence of a passing PWA.
- Do not describe CPU1 lost-before-start tests as arbitrary partial-job recovery.

No complete codebase proof, hardware deadline result or exhaustive parser/race
audit follows from these reports. Deep FM6/physical-model DSP, real USB/cache
timing, optional SLICE, actual Android lifecycle/device behavior and some
protocol interleavings retain explicit coverage limits. Sanitizer setup failures
in several specialist environments are inconclusive, not clean runs; retain
the successful targeted checks and add checked arithmetic where needed.
