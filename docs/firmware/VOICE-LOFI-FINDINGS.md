# VOICE and master LOFI: investigation synthesis

Investigation baseline is the actual uncommitted .92 working tree, frozen under
`build/voice-lofi-investigation/baseline/`. Three specialists examined VOICE DSP,
master processing, and allocation/deadline scheduling. Production audio source,
version and installer were unchanged by the investigation itself. The subsequent
[implemented scheduling and recovery fixes](VOICE-LOFI-FIXES.md) are packaged
separately; the findings below retain their baseline meaning.

## Established findings

1. **An overload can leave ordinary held notes permanently missing.** The actual
   ISR requests shedding after two consecutive halves above 95% of its wall-time
   budget. The allocator deliberately fades upper POLY chord tones first after
   eligible release/unison voices. Only native drone pitches are remembered for
   recovery. A still-held ordinary VOICE or sequencer note is not restored when
   subsequent halves are safe; it needs another onset. Tests render the real
   engines/effect while injecting clock/DMA costs into the real ISR. Two 5600 us
   halves followed by 501 safe 4000 us halves leave one VOICE chord tone absent.
   Those times prove a policy mechanism, not measured FM-1 performance.
2. **The automatic partition is based on fixed estimates, not measured work.**
   VOICE and GRAIN have fixed weights and the worker receives at most one synth.
   A deterministic injected-cost counterexample shows an automatic partition
   shedding a GRAIN tone with LOFI while another legal partition retains all
   seven notes. Its specific costs are synthetic; the chip's actual ranking is
   not established. A higher admission budget does not prevent CPU shedding.
3. **An empty second VOICE track blocks useful offloading.** The dispatcher
   counts configured VOICE engines even without active voices, then excludes
   them all to protect their shared breath RNG. Parent's actual-code probe holds
   four notes on track 1, leaves track 2 configured as VOICE but silent, and gets
   worker mask 12 (empty track 3 plus drums). Changing only empty track 2 to
   ANALOG gives mask 1, offloading the sounding VOICE. Shared-RNG protection is
   needed for concurrent rendering, but configured-engine count is too broad.
   Any fix must handle current onsets, fades and engine adoption safely; the
   preceding block's voice count alone is insufficient.
4. **LOFI's startup fast-path gate is unnecessarily broad.** Its noise-bed ramp
   takes 4096 frames, about 93 ms, and keeps the entire processor on its changing-
   control path even after other controls settle. That is avoidable repeated
   setup, not proof that this work alone exceeds the hardware deadline. Exact
   private prototypes reduce repeated work but have not established a target
   speed improvement and are not installed.
5. **No deterministic VOICE-state corruption was reproduced.** Across 196
   scenarios and 686,000 mixer blocks, all seven VOICE presets, all seven LOFI
   settings, vowel DRIFT/MORPH off/on and drums off/on preserve pre-master audio,
   complete voice state and breath RNG exactly relative to OFF. Three GRAIN and
   four VOICE gates stay intact. Fourteen UBSan follow-ups pass. This isolates
   ordinary processing from the external hardware deadline callback; it does
   not model that callback's timing or exclude downstream spectral masking.

VOICE still has a four-voice local cap, and GRAIN a three-voice cap. A fifth held
VOICE pitch causes the same replacement with LOFI OFF and TAPE, so that existing
limit does not explain an effect-dependent loss in the tested four-note cases.

## Important correction about postmix cost

Master LOFI runs after `cpu1_wait`, so it adds serial main-core work after both
render partitions finish. Merely omitting that common cost from partition
comparison does not make one split worse than another: it adds to both choices.
The actionable scheduling weaknesses are inaccurate unit estimates, restricted
partitions and overly broad shared-state exclusions. LOFI can still push total
wall time over the threshold. The CPU display smooths render time and excludes
nested TIMER5 time, while shedding uses full wall time; a modest display reading
does not rule out occasional shedding.

## Recommended fixes, in order

1. Tighten the empty-VOICE exclusion with an explicit current-render eligibility
   proof, including queued engine changes and fading old voices. Keep genuinely
   shared RNG, LFO-wrap, PHYS and MORPH exclusions.
2. Replace guessed partition costs with bounded per-unit hardware timer profiles
   published after the worker join. Evaluate legal masks using those profiles,
   handle new onsets/engine generations and avoid unstable switching. The
   specialist design estimates roughly 32 bytes of profile state; actual linked
   size, timer overhead and device behavior must be measured before adoption.
3. Reduce LOFI transition setup only where exact output/full-state oracles pass
   and emitted target work improves without slowing the settled path. Preserve
   every character, control slew, phase, history and RNG advance.
4. Consider recovery for CPU-shed ordinary notes only after the processing fix.
   Re-admit only notes with a live logical owner and real headroom, without
   stealing another voice. Do not restart naturally finished samples, released
   notes or expired sequence gates, or create a repeated shed/restart loop.

Do not disable overload protection, tune another arbitrary engine weight, raise
the voice budget blindly, or label synthetic timing as a physical root-cause
measurement. No more manual diagnostic-screen readings are needed to establish
the software mechanisms above.

## Evidence

- [VOICE engine and morph/state isolation](VOICE-LOFI-ENGINE-INVESTIGATION.md).
- [Actual ISR/allocator counterexamples and scheduling design](VOICE-LOFI-SCHEDULING-INVESTIGATION.md).
- [Master LOFI routing, transitions and private probes](VOICE-LOFI-MASTER-INVESTIGATION.md).
- Parent probe: `build/voice-lofi-investigation/empty-formant-probe.c` and `.log`.
  These and specialist reproductions are private evidence, not installed code.
