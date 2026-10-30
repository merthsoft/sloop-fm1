# Selective upstream 2.8 integration

## Integration completion

MerthSloop **2.8.0.54** completes the selected upstream 2.8 feature integration
against `b77bf55`. The version now denotes the adopted SLOOP 2.8 behavior;
our independent build counter continues at 54. This is a selective source port,
not a Git ancestry merge or a claim that every upstream UI/content decision was
copied. Local `main` remains the unmodified upstream mirror.

- Partial held-note recording follows each POLY note to its own release, including
  new attacks over a held group and quantized onsets before the next TIE exists.
  MerthSloop recording snap, musical-clock latency and independent ties remain.
- Upstream `SF_LEG` bit 7 records overlapping non-POLY transitions and sustains
  the voice envelope on playback without forcing glide. Continuations take
  precedence; explicit SLIDE retains its separate meaning.
- INFO protocol 16 carries legato in the existing STEP extension's `hi` byte,
  without enlarging STEP replies. Old writers preserve the existing marker;
  aware writers explicitly replace it. Web, preview, Android native snapshots
  and scene validation retain it. Project layout remains upstream FUN6.
- Factory and user preset arps follow the next sound when their eight arp values
  still match the loaded preset. An enabled/edited player arp remains. Engine
  switches compare the previous requested sound before changing its engine.
- HOME shows a themed A–D badge only while an arrangement section is playing.

Earlier batches already integrated the piano roll/action sheets, CC11/CC0,
resonance/fade, drum preferences/tuning, clock, encoder, USB descriptor and
recovery/updater changes. Deliberate differences remain: our editing/navigation,
independent-tie tools, complete sample content, effects, USB playback removal and
return-to-stock installer workflow. FREE TAKE and MIDI-file duration authoring
remain separate enhancements; this batch improves ordinary live overdub.

Measured target: **582,968 / 584,336 B**, **1,368 B flash free**; main static
RAM **86,176 / 98,304 B**, low RAM **28,784 / 29,496 B**, audio pool
**332,876 / 344,064 B**. Relative to .53 this is 544 B flash and 32 B main RAM;
low RAM and the audio pool are unchanged.

Validation includes unchanged 196-render audio goldens; upstream recording tests
for staggered release, early/late entry and legato envelopes; independent ties,
real factory/user arp loading, v16/legacy STEP handling, web seven-bit round-trips,
Android workstation/sequencing tests, preview parity and target DSP budgets.
The new upstream 2.8 fixture is generated independently from the unmodified main
source and survives import/apply/save byte-for-byte.

Broader testing exposed old navigation and preview-clock expectations, and a
TRACKS test that explicitly expected the group-release behavior being replaced.
Those assertions now cover the current pages, musical clock and independent
releases, and their focused reruns pass. Font-test fallback reads the raw branch
of boot-packed assets. Divide-by-zero trap instrumentation passes UI fuzz,
sequencer, independent ties, projects and random live use; the host toolchain
has no working UBSan handler runtime, ThreadSanitizer or AddressSanitizer, so
full runtime-sanitizer coverage is not claimed. The monolithic WSL runner is
not a green result: its initial pass includes the corrected stale assertions,
unavailable sanitizer runtime and missing Node executable. Those functional
cases pass focused reruns; the complete web suite and native/WASM parity run
separately with Windows Node. Hardware recording/listening
remains unverified for this build.

The following sections are historical checkpoints and explain the staged port.

## Current controls follow-up

Build **2.7.1.51** additionally integrates upstream-compatible device-setting
bits, track-filter resonance, punch fade, noise-drum tuning and optional drum
browsing preview/step advance. See [controls and validation](UPSTREAM-CONTROLS-REVIEW.md).
These join the already adapted piano roll, MIDI expression/banks and low-conflict
fixes below. This remains a selective backport, not a complete upstream merge.
Recording/bit-7 legato and preset-owned arp behavior remain pending; independent
ties are compatible with upstream continuation bits and should be retained.

This batch adapts the low-conflict fixes from upstream commit beab3f62d04ff8defcc7b663505bd6bd6ad7836a, reviewed against the last integrated 2.7.1 base (4ec1ac8). Main/origin main mirror upstream at b77bf55; merthsloop is not marked as fully merged with 2.8. Its version remains based on 2.7.1 with an incremented MerthSloop build.

## Included

- Early USB RESCUE scan drives PA2/backlight low before scanning OCT−, using the upstream HAL helper. Real register access remains in hal/.
- Encoder detent counting no longer requires each intermediate state to survive two scan frames. Existing bounce, reverse, skipped-state and rest handling remains; the UI's acceleration preference is unchanged.
- USB Audio Class lists audio streaming before MIDI in the audio-control collection and bumps bcdDevice by 0.08 when audio is enabled. All collection members/interfaces/endpoints and CDC/rate variants are otherwise retained. No USB playback return or queue-size change is imported.
- MIDI clock tempo estimation averages up to eight beats, uses 0.8 BPM hysteresis after filling the window, and resets the history for a change of at least five BPM or clock reacquisition. Musical phase still follows pulses. Existing USB/TRS source filtering, dropout handling, eighth swing and HALF/DOUBLE internal-clock policy remain.
- Browser updater discovers generic Windows loader names by the verified FM-1 handshake, prefers matching port pairs, and waits up to 60 seconds for handoff/reboot. Normal firmware discovery retains its named-port policy. Existing foreign-loader protection and return-to-stock support are retained; upstream's removal of return-to-stock is not included.

## Not included

No whole-file replacement or upstream merge was performed. Recording/legato bit 7, preset-owned arp behavior, resonance, punch fade, wholesale UI replacement and sample-content cuts remain separate candidates. CC11/CC0 support and the native NOTES/action sheets are now integrated through MerthSloop-specific helpers. The new ROOM/PLATE/HALL and dual-path SPRING work remains intact. Existing settings/project prefixes are unchanged by this backport.

The piano roll preserves independent ties and modifier gestures; see PIANO-ROLL-INTEGRATION.md for implemented controls and remaining recording compatibility work. The initial candidate ranking is in UPSTREAM-CANDIDATES.md.

## Verification

Recovery boot-key ordering and fast-encoder regressions pass. The complete
sequencer regression passes, including upstream's minute-long jitter, half-BPM,
tempo ramp and jump cases. Nine CDC/rate descriptor variants and a compile-time
44.1-only build pass. The latter exposed an old test assumption that SET_CUR
could select 48 kHz even when compiled out; its expectation now respects the
advertised capability.

All 182 audio goldens are unchanged. Swing, HALF/DOUBLE combinations, durable
section/startup saves and the canonical upstream project fixture pass. Native
and WASM audio match across all engines and five reverb models. Hot DSP target
budgets and inlining checks pass. The full web/update suite passes, including
generic-loader install/resume, interrupted handoffs, foreign-loader rejection,
matching-port behavior and retained return-to-stock simulations.
The web parity test also caught stale OCT UP/MONO labels for our existing
DOUBLE TIME/VINYL slots; the web controls now match the native effect IDs.

Firmware 2.7.1.36: image 580,304 bytes, 1,260 bytes free; main static RAM
85,688/98,304, low RAM 22,772/29,496, pool 332,876/344,064. Relative to the
reverb build, the selective backports add 192 image bytes and 32 static RAM
bytes. No audio buffer is added and the reverb algorithms are unchanged.

Host checks cannot establish physical rescue availability, fast-knob behavior,
USB enumeration on every host or real audio deadline margin. These remain
hardware validation points; no automatic firmware flash is performed.

## Editor and MIDI integration completion

The subsequent integrated build also adapts the piano-roll visual/editing ideas
while retaining native independent ties, chord merges, library pages and controls.
CC11 expression and bank-aware Program Change are integrated through actual drum
audio and user-record loading. The fork still is not a full 2.8 merge: upstream
POLY recording/bit-7 legato semantics remain deferred. Current production image
is 581,140 bytes (424 free), main static RAM 89,064/98,304. See
[editor integration](PIANO-ROLL-INTEGRATION.md) and [MIDI](MIDI-EXPRESSION-BANKS.md).

## Selective upstream 2.8.3 safety and hold timing

The next selective batch takes the 350 ms layer-display threshold and second-core
hardening from upstream `5eb24c1`. Normal page taps still use the existing 450 ms
tap limit. Held key/knob use opens a layer immediately; temporary LFO/ENV panels
use the same longer threshold. The chord/arp latch gesture remains 700 ms.

Worker jobs carry a function/argument/sequence check word; core 1 accepts only the
next job and keeps its completed sequence in a register. Each core writes its own
64-byte mailbox area. Wrong jobs are refused; timeout or refusal holds the worker
and bars it across warm resets until power-off. A main-core crash beside an active
worker sets the same bar. Boot holds any leftover worker before clearing RAM.
Core 1 installs its own stack bounds; a worker fault records and parks instead of
trying to draw/reset from the worker and freezing itself.

MerthSloop retains its weighted voice budget, automatic render split, timeout
recovery and audio diagnostics. The shared crash recorder avoids duplicating its
common field writes for the two cores. The crash record keeps its layout; the new
eight-byte bar follows the existing `.noinit` records. Console `cpu1_gave_up`
reports refusal (1), timeout (2), or a retained bar (3). Extra upstream BAD display
and console fields are omitted to conserve flash; job validation is unchanged.

`tests/cpu1_hal_test.py` compiles the production HAL and fault handler with only
MMIO, barriers and terminal fault loops doubled. It checks 20,000 handoffs, corrupt
function/argument/check/sequence rejection, no replay, timer wrap/timeout, startup
failure, stack-register setup, warm-reset blocking and both fault paths under
UBSan. UI checks cover page taps around the old threshold, the 350 ms boundary,
immediate knob use and the existing navigation/fuzz suite. These checks do not
qualify silicon cache ordering, physical stack traps or audio deadline margin.

That batch did not import the upstream drum allocator, sample retrigger changes, MIDI
queue-overflow recovery or new overlapping-note editor/recording implementation.

The subsequent [.96 drum playback integration](DRUM-PLAYBACK-INTEGRATION.md)
imports the shared upstream allocator for sampled and synthesized drums, retaining
same-sound retrigger priority, kick protection, six voices, hat choke and declicking.
The other exclusions above remain separate work.
