# Official MVAVE FM1 Open review

Reviewed 2026-10-19. Official repository: https://github.com/m-vave/fm1_open .
Pinned source: `4d266850f8a46bd5154a0d495c1f60317156c9f7` (README: FM1 Open V14).
Inspected in ignored `build/mvave-review-source`. No official firmware code, samples,
SDK libraries or boot images imported into MerthSloop; no external build scripts run.
This is a source review, not a sonic comparison, target benchmark or hardware test.

## What is actually open

The APP and browser engine/synth sources are public. The hardware instrument engine
is supplied in `sdk/lib/libfm1_engine.a`; `sdk/include/fm1_engine_control.h` exposes
its render/transaction API. `app/board/audio.c` calls that binary engine. Browser
implementations are in `web/engine` and `web/synth`; do not assume identical device
execution, timing or memory placement from those sources.

`LICENSE.md` declares company APP code GPL-3.0-only with an SDK linking permission.
`THIRD_PARTY.md` separately lists component terms, a proprietary FM1-only engine
grant, and Apache-2.0 boot/recovery resources. Our engineering recommendation is to
adapt isolated public algorithms/patterns with their notices, not link the binary
SDK into MerthSloop. The exception on their APP does not supply an exception for
unrelated upstream code in our existing firmware. Any proposed port needs a
file-specific provenance/terms check; no blanket asset permission is assumed.

## Priorities

| Priority | Candidate | What is useful | Fit and limits |
| --- | --- | --- | --- |
| 1 | MIDI overflow recovery | `app/midi_router.c` notices lost events, flushes stale work, and sends bounded per-channel pedal/all-sound-off/controller recovery. Source/owner accounting stops one holder's Note Off killing another holder's same pitch. | Adapt the policy to our own queues. Our USB MIDI output ring is restored to 512 events for maximum PULSE bursts, but `usb.c:midi_out_event` still drops events when full; a stalled host still merits an explicit recovery policy. Do not copy their three queues and 32-owner-per-port tables wholesale. Stress full chords, arps, sustain, disconnect and queue wrap. |
| 2 | External clock loss and position | `web/engine/sequencer.c` stops synchronized playback after clock loss; routing locks clock input to a chosen/auto source. `app/midi_router.c` preserves paused output-clock phase on Continue. | Our input clock currently falls back to internal advance after a 500 ms gap. Offer an explicit policy rather than silently changing existing sessions. Audit SPP and Continue end-to-end, including TRS parsing and outgoing transport. Their 2 s timeout is a policy example, not a value to copy automatically. |
| 3 | Per-track alternate tunings | `app/instrument.h` specifies main and alternate 12-note pitch-class tables, whole-cent offsets -127..127, applied at note-on. Existing voices keep their pitch. | Distinct musical feature. Both tables for our three synth tracks require 72 bytes before voice, serialization and UI costs. A practical first version is one main table per track (36 bytes), then optional alternate selection. Requires a common pitch-path audit across all engines. No claim that we already have this feature. |
| 4 | Preserve a live instrument across scene changes | `app/track_session.inc` separates NORMAL, PLAY and OFF roles, preserving live timbre while keeping dormant project state for saves. Solo grouping follows track role. | Useful for a lead over changing accompaniment. Substantial ownership/project design: must preserve held voices and prevent the live edit from leaking into scene saves. We should adopt the behavior selectively, not their eight-track architecture or allocation code. |
| 5 | Audio DMA ownership/deadline guard | `app/board/audio.c` renders into scratch, checks ownership/time before committing, and records rejected/late commits. | Our ISR renders directly into the free DMA half and records lateness afterward. Investigate a compatible guard/commit design and fault-injection test. Extra scratch and copy cost must be measured; their 128-frame callback differs from our block/half layout. Keep our existing two-core split. |
| 6 | Fixed-time delay alongside tempo sync | Official README describes fixed delay and tap controls; browser FX is in `web/engine/effects.c`. | Potentially useful, but we already have tempo delay and tap tempo. A fixed-ms mode is the actual new behavior. Preserve existing saved TIME IDs; smooth changes and fit the existing delay line rather than extending their advertised 2040 ms blindly. |
| Later | AY/YM PSG character | `web/synth/psg.c` adapts emu2149 with shared per-chip noise, eight-note allocation and a DC blocker. | More specific than our LOFI chip voice, but a new engine competes for scarce flash and voices. Only pursue after listening demonstrates value over LOFI. Prefer original emu2149 plus narrowly needed fixes. |

## Things to keep rather than replace

- Keep our current stereo FDN and new optional SPRING. The public browser FX uses
  a cheaper four-comb reverb and decimated/interpolated processing; no evidence it
  sounds better than ours or is the same DSP used by the device binary.
- Keep our FM6/MSFA implementation, DX7 import and user-bank features. Their
  `app/fm_library.c` is worth studying for VMEM validation and exact raw round
  trips, not wholesale replacement. Original V15 factory bank data is separately
  identified in their notices; do not silently replace our sounds.
- Keep the existing synth/drum/pluck engines. Their 303/808/pluck roles overlap
  ours; different source is not evidence of a sonic improvement.
- Do not reintroduce phone/computer audio playback: it was intentionally removed.
  BLE MIDI is a separate possible feature, but the public SDK declarations do not
  provide a self-contained open Bluetooth driver we can drop into our HAL.
- Do not adopt the SEA sample codec without quality/seek/CPU measurement. Its
  public decoder accepts 5/6-bit chunks; our current direct ADPCM path is 4-bit.
  This is not an obvious compression saving and would change playback costs.
- The official source contains eight-track project, voice-library and sample
  layouts. Firmware switching does not make those files compatible with ours.

## Suggested next batch

First audit and fix MIDI overflow recovery, then define external-clock-loss/SPP
semantics. These improve reliability without a new synth or large audio buffer.
Next propose a small tuning editor/table format. Live track roles and DMA commit
changes deserve separate designs and target measurements. Do not port any new
sound engine until flash headroom is reclaimed deliberately.

The approved HALF/combinations/SPRING work remains separate. Its pending target
has only a few KiB of flash free; exact final figures and verification belong in
`EFFECTS-IMPLEMENTATION.md`. No official feature is included merely by this review.

## Pinned primary-source links

- [APP MIDI routing](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/midi_router.c)
- [Track roles](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/track_session.inc)
- [Tuning API](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/instrument.h)
- [DMA/audio ISR](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/app/board/audio.c)
- [Browser sequencer](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/web/engine/sequencer.c)
- [Browser effects](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/web/engine/effects.c)
- [PSG adapter](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/web/synth/psg.c)
- [License](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/LICENSE.md), [component terms](https://github.com/m-vave/fm1_open/blob/4d266850f8a46bd5154a0d495c1f60317156c9f7/THIRD_PARTY.md)
