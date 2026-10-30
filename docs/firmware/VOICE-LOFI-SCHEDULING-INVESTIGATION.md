# VOICE + master LOFI: allocation, scheduling and deadline investigation

This is a read-only audit of the frozen actual .92 tree in `build/voice-lofi-investigation/baseline/source`. Private source/tests are under `build/voice-lofi-investigation/scheduling/source`; production sources, shared outputs and the baseline are untouched. No hardware diagnostic readings are required for these findings.

## Confirmed loss mechanism

`audio.c:64` consumes a pending overload request before rendering the next DMA half. `audio.c:86` sets it after two consecutive half renders exceed 95% of the approximately 5.8 ms half-buffer deadline. Full wall time includes nested TIMER5 work; the displayed smoothed `song.cpu_q8` excludes that nested work. Therefore a CPU display below 95% cannot rule out this path. A pending request is still consumed when the following half would be fast: the next half's cost is not known yet.

`voice.c:282` deliberately sheds a voice: quiet released tails first, otherwise extra UNISON voices, otherwise the oldest held POLY note except the lowest note of each part. `voice_kill` clears gate/sustain, sets a two-control-block stage-4 fade, resets recovery cooldown and counts the kill. This is a specific mechanism for chords becoming fewer notes, often eventually only their bass. A higher voice budget cannot prevent CPU shedding.

Only stolen native-drone pitches are remembered (`voice_lost`, `drone_held` predicate). `seq.c:2299` restores only those still latched native POLY drone notes after a quiet quarter-second, with an inactive slot and admission room. Ordinary live/sequencer notes are not restored when CPU recovers, even if they remain logically held. They need another note-on. Turning master LOFI OFF does not directly restore them either; a subsequent sequence onset or key press does. This distinction can identify immediate restoration versus retrigger restoration without asking for more counters.

## Deterministic proof using the actual ISR, mixer and allocator

`build/voice-lofi-investigation/scheduling/source/tests/scheduling_counterexample_test.c` includes the frozen `hostsim.c`, then the real `audio.c` ISR with deterministic injected clock/DMA reads, following the maintained `audio_shed_timing_test.c` pattern. It renders real GRAIN and VOICE instruments and the real master effect. Timing values below are **synthetic counterexamples**, not measured FM-1 costs and not claimed hardware reproductions. Synthetic per-half unit costs are GRAIN 667 us per held note, VOICE 1000 us per held note, drums 1200 us and postjoin LOFI 1000 us. Ordinary actual rendering still happens; the clock model replaces elapsed time only.

| Scenario | Result |
| --- | --- |
| Automatic partition, LOFI OFF | Three GRAIN + four VOICE notes, worker GRAIN, injected 5200 us, no shed |
| Same notes, LOFI ON | 6200 us causes a shed; an older GRAIN chord tone is removed; count-based ranking then switches worker to VOICE and stabilizes at 5000 us with the note still missing |
| VOICE worker from the beginning, LOFI ON | All seven notes retained, 5000 us, no shed |
| Two injected 5600 us halves, then 501 halves at 4000 us | Exactly one ordinary held GRAIN tone remains absent throughout the safe period |
| Same transient, only the VOICE chord sounding | One of four VOICE notes remains absent throughout 501 safe halves |

Both optimized and UBSan variants pass (UBSan variant includes the VOICE-only case). Output is in `build/voice-lofi-investigation/scheduling/scheduling-counterexample.log`. This proves that static work misranking plus ordinary-note nonrecovery is sufficient to reproduce the symptom; it does not establish that these particular synthetic costs describe the hardware.

## Scheduling findings

`fx.c:998 units_split` estimates work from the preceding render's voice counts and a fixed engine weight. GRAIN weighs 6 per voice; VOICE/FORMANT weighs 3. Four VOICE notes therefore rank below three GRAIN notes. Actual VOICE performs four signed-64-bit resonators per sample and substantial block control setup, so the ranking merits measurement rather than an assumption that it is correct. The worker gets at most one synth in automatic mode; drums are assigned beside the lighter estimated synth workload. Different voices, control transitions, grain spawning and tail populations can change the actual cost without changing the score proportionally.

**Important correction:** master LOFI is after `cpu1_wait`, after worker/main mix merge and effect buses. Its time is serialized with the slower parallel branch. Omitting that cost from the partition comparison is not independently a bug: the same postjoin cost is added to every partition. LOFI can expose an already suboptimal partition by consuming the remaining deadline margin, but one cannot pretend its work overlaps worker synth rendering.

Other boundaries:

- A part whose LFO wraps this block stays on core 0 because it draws shared RNG, even for an LFO whose audible depths are zero. With two FORMANT-assigned tracks, both stay on core 0 because their breath RNG is shared. The latter counts engine assignments even if the other track is empty. Neither is directly a LOFI gate mutation; both can reduce available parallelism.
- A DRIFT-to-MORPH track is pinned to core 0 only if A/B can change LFO rate/sync, preserving shared RNG ordering. Vowel-only morph with stable LFO timing is eligible.
- Manual `cpu1_units` is still filtered by those safety conditions.
- Worker faults or the 8 ms HAL timeout permanently hold CPU1 and fall back to one core; the interrupted block can glitch and subsequent work can overload. Hardening deliberately does not restart an unknown worker automatically. A master LOFI toggle itself does not set this condition.
- New events are applied before dispatch; the previous-block voice count can lag a new attack or completed fade by a block. Hardcoded weights do not describe within-block spikes.

## Voice disappearance inventory in this scope

| Path | Trigger / effect | LOFI relationship |
| --- | --- | --- |
| Shared weighted admission | `voice_room`, `voice_alloc`, `voice_start`; steals/reuses voices to admit new notes | Master LOFI does not change the 24–48 unit limit or engine cost |
| Per-engine cap | `trk_nvoice`; GRAIN 3, VOICE 4, physical slots up to eight; POLY reuses/replaces within cap | Master LOFI does not change these caps |
| Overload guard | `shed_voice -> voice_kill -> env_tick(stage 4)` | Added master work can cross threshold; confirmed synthetic causal chain above |
| Regular note-off | `trk_note_off -> voice_release`, then envelope release | No direct master LOFI connection found |
| All-off/STOP/panic | `trk_all_off` clears gates and puts active voices into release | No direct master LOFI connection found |
| Sustain expiration | `sustain_gain` ends finite tails | No direct master LOFI connection found |
| Natural envelope/sample completion | Release stage ends, sampled/owned-envelope engine completion | Engine-specific; a master effect is downstream |
| Preset/engine transition | `engine_block` fades and clears old voices, then starts queued attacks | LOFI type changes do not request track engine changes |
| Silent sustain / modulation / mute | Voice may remain active but rendered amplitude is zero or attenuated | Can sound like missing notes without allocation loss; compare pre-master state/audio |
| HAL worker failure | Fallback can repeat partially completed state for its failed block; holds worker thereafter | Load consequence, not a LOFI-triggered gate write |

## Recommended next steps

1. Use the VOICE DSP audit's exact control-coefficient caching and the master LOFI audit's verified bypass/settling improvements first. Avoiding an overload is better than hiding a deliberate voice cut. Test mode-enable/control settling as well as steady playback: a brief expensive transition is sufficient for lasting ordinary-note loss.
2. Replace/qualify fixed core scores using actual per-unit target timings, with a bounded previous-block estimate and existing RNG/morph safety exclusions. A small exhaustive legal mask search can include more than one worker synth, but timing instrumentation and profile lifetime must be costed. Do not permanently hardcode the synthetic VOICE/GRAIN ratio from this report or force the worker mask for every song.
3. Treat ordinary held-note recovery as a separate policy improvement. It needs logical source ownership (physical keys, sequencer independent ties, MIDI/ARP/chords), no natural-one-shot restart, current pitch/velocity, engine-transition cancellation, free slots/admission, and sustained deadline headroom before re-admission. Reusing the drone bitset blindly or restarting every inactive voice is unsafe. The existing quarter-second cooldown alone cannot stop repeated shed/restart oscillation in sustained overload. No production recovery change is proposed on incomplete ownership evidence.
4. Keep the real overload guard and existing bass protection. Raising the budget or deleting protection can turn missing chord tones into broken audio deadlines instead of addressing the cause.

This audit supports a causal mechanism and a reproducible policy weakness, while actual target timing attribution remains open. There is no claim that all reported dropouts are explained by the clock model.

### Bounded measured-cost scheduler design

The strongest scheduling improvement is per-unit measured work, rather than another guessed engine multiplier. Each `mix_units` unit surrounds the real `mix_part`/`mix_drums` call with the same timer already used for CPU1 work accounting. Each unit is owned by exactly one core for that block. Core 0 consumes completed samples only after the existing wait/join barrier; it must not read the worker's in-progress sample. Timeout fallback records its own completed measurement after holding the worker, and should invalidate stale profile confidence.

Four last-sample values plus four smoothed estimates need approximately 32 bytes of static RAM (before optional generation/voice-count confidence metadata); that fits the reported 804-byte main gap, but a target link must verify the real layout. Both direct-mapped per-unit arrays and a measured-cost partition search need flash cost measurement. Four render units, eight CTL blocks per 256-frame half, two timer reads per unit imply at most 64 added register reads per half when all units are instrumented. The actual code/register ordering and target timing overhead must be measured; this is not free and should have a compile-time diagnostic comparison first.

Keep the existing eligibility/RNG/FORMANT/morph exclusions and only evaluate their permitted masks. Exhaustive search is bounded to 16 masks for four units. Minimize `max(sum(worker unit costs), sum(main unit costs))`, retaining an idle/tie policy to avoid unnecessary migration. The shared postjoin master/effect cost cancels from that minimization. Profiling total whole-half time instead would not identify which eligible partition is cheaper.

Use current ownership/activity after `events_block` to qualify stale profiles. Previous `mix_voice_count` is deliberately lagged and is not a safe indication that a new key/event cannot render this block. On onset, engine/preset/source transition, zero-to-active or invalid profile, fall back conservatively to prior engine/count estimates (or scale an established per-voice component) until a completed sample exists. Do not change the RNG eligibility decisions based on a timing estimate. Preserve source/engine generation identity so cheap measurements from a previous preset are not reused confidently for a new expensive sound.

A small standalone search can prove selected masks against supplied costs, but that alone cannot validate actual timer overhead, multicore publication or target deadlines. Consequently this report preserves the real-code injected counterexample rather than presenting an unmeasured instrumentation patch as an integrated fix. A proper candidate should run split-output equality/CHAOS/timeout tests, zero/one/two-FORMANT cases, cold/source-changing GRAIN, control transitions and target link/timer disassembly checks before device qualification.

## CPU-only recovery implementation candidate

The follow-up candidate is in `voice.c`, `seq.c`, `audio.c` and `tests/cpu_shed_recovery_test.c`. Unlike the audit above, these production sources are now changed for parent integration.

- Only actual CPU shedding queues ordinary voices. Budget and local-cap replacement retain their prior behavior.
- Reuses the existing pending-drone byte map: bit 7 denotes ordinary CPU recovery and bits 0–6 retain original velocity; native-drone marker 1 retains its separate path. Small per-track metadata captures engine/sample-source generation. A pending-part mask avoids an idle scan when nothing is queued.
- Explicitly limited to POLY with nonzero sustain, excluding sampled/SLICE engines, PHYS, engines with custom amplitude curves (including LOFI instrument), and own-envelope FM6. Their natural completion/lifetime would require more provenance to restart safely. VOICE/FORMANT and GRAIN are included; existing native drone recovery remains available under its prior contract.
- Recovery requires an existing current MIDI, physical key/chord, chord latch, keyboard hold, current sequencer/tie or sounding generated-output owner. An arp's held root alone is insufficient. Actual note release, new attacks, STOP/alloff, queued engine adoption, GRAIN source changes/sample generation changes, and a switch to non-POLY or zero sustain cancel pending attempts.
- Full-wall headroom must continuously stay at or below 75% for 0.5 seconds, with no DMA-late half. This is a deliberately conservative 25% reserve, not a measured promise that every engine voice fits that reserve. It uses actual audio ISR elapsed time, including nested work; host CPU timing and the smoothed CPU display cannot grant recovery.
- One free physical slot and sufficient shared admission are mandatory; recovery never steals or emits MIDI/recording events. The existing quarter-second no-steal cooldown also applies. After each readmission the headroom history is consumed and backoff advances through 1, 2 and 4 seconds for subsequent attempts. Eight seconds of continuous margin reset the backoff. Continuing overload cannot cause repeated restarts.
- Readmission starts a fresh local attack at its captured velocity; it does not reconstruct the killed oscillator/envelope history. Unsupported percussive/natural-one-shot lifetimes remain excluded instead of approximated. Very short sequencer gates correctly expire before the recovery window and are not resurrected.

Focused injected-ISR checks cover real physical key down/up, real MIDI input ownership, tied sequencer output, generated arp-output expiry, short-lived/absent owners, released/new-generation notes, prolonged overload, insufficient margin, DMA lateness, backoff, STOP/alloff/queued engine/source/sample-generation/zero-sustain cancellation, and sampled-engine/budget exclusions. The test's supplied microseconds exercise policy boundaries; they remain expressly synthetic, not device profiling.
