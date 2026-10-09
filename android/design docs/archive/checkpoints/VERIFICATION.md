# Parallel implementation verification

Independent review: 2026-10-07. Reviewed the FM6 sound-design foundation, sequencing
foundation, sample encoding foundation, USB playback firmware, and their design/status
updates. This records the pre-integration checkout, not physical FM1 acceptance.
This records the original review before integration. All three reproduced findings below are now corrected with regression coverage. See [INTEGRATION.md](INTEGRATION.md) for the completed fixes, Android consumers and current validation. The descriptions below preserve the original reproduction evidence.

## Closed findings — original reproductions

### P2: unchanged hardware edits create false changes and reject locked steps

In [HardwareEditor.cs](../../../src/Sloop.Sequencing/HardwareEditor.cs), synth transpose/level
and drum edits rebuild immutable slot/lane arrays even when every value is unchanged.
Record equality compares the array backing storage, so the step and track appear changed.

Reproduction: create a blank drum step and propose `SetLevel(HitLevel.Normal)`.
It reports one `TrackChange`; applying it makes `CanUndo` true despite changing no
musical data. Protecting that empty step with an event lock rejects the same unchanged
operation with "Step is locked." Zero transpose and setting an existing synth level
use the same array-replacement path.

Required correction: reuse the original step/arrays when contents are equal, before
checking locks or calculating diffs. Add tests for no-op synth/drum operations, unchanged
locked steps, and history remaining empty. Actual changes must still reject protected data.

### P2: negation can produce the opposite requested sound edit

[PhraseParser.cs](../../../src/Sloop.SoundDesign/PhraseParser.cs) matches recognized fragments
without recognizing negation. `Parse("not brighter", Keys, 42)` returns
`HasSupportedRequest=true`, `Brightness=High`, and an unsupported-word notice for "not".
The documented integration example proceeds whenever `HasSupportedRequest` is true,
so displaying the notice alone does not prevent the opposite proposal.

Required correction: conservatively reject unsupported negation scopes or implement an
explicit bounded negation grammar before extracting positive edits. Add tests for
"not brighter", "don't make it brighter", preservation wording and compound clauses.
The parser remains a limited offline vocabulary; this finding does not require a model.

### P3: finite gain can overflow resampling intermediates

[PcmConversion.cs](../../../src/Sloop.SampleEncoding/PcmConversion.cs) validates finite gain
and post-gain source samples, but not FIR accumulation or converted samples. Convert 100
mono samples of `1.0` at 44,100 Hz using first-channel downmix and gain `1e308`: conversion
succeeds with infinite `PeakBeforeQuantization` and saturated PCM. The finite-input
contract alone does not protect downstream reports and persistence from non-finite output.

Required correction: reject non-finite resampler/quantizer values, or impose a documented
gain bound. Test huge finite gains and preserve normal intentional clipping behavior.
This is a robustness edge case; ordinary gain conversion and golden artifacts passed.

## Original pre-integration checks (historical counts)

| Area | Result |
| --- | --- |
| SoundDesign console suite | 1,322 assertions pass: bounds, factory/wire parity, SysEx, recipes, locks, stale apply and undo/redo |
| Actual firmware FM6 host suite | Rebuilt and passed algorithms, envelopes, macros, patch formats, voices and bank checks |
| Sequencing console suite | 16/16 scenarios pass: app/native preservation, edits, locks, chains, compound history |
| SampleEncoding console suite | 101 assertions and nine Python golden fixtures pass |
| Existing protocol/simulator/sampling suite | 400 checks pass |
| Shared Android solution | Build succeeds with zero warnings/errors, explicit SDK/JDK and fresh build processes |
| USB playback host suite | Rebuilt and passed CDC=0/1/2, plain/serial-off descriptor comparison, loader, capture ordering, drift, malformed input, suspend/reset and ring wrap |
| USB playback undefined-behavior traps | Rebuilt and passed |
| JieLi target rebuild | Link, RAM, flash, RAM-code and MMIO checks pass |

The target image is 566,284 bytes. Data+BSS is 88,180/98,304 bytes; pool allocation is
334,560/344,064 bytes. No firmware was flashed. The host USB tests model registers and
DMA; they do not prove physical endpoint duplex behavior or Android USB routing.

Independent edge-case probes live in ignored `build/verification-dotnet/`; focused
firmware logs/artifacts are in ignored `build/verification-usb/`. Existing fixtures were
not regenerated, budgets were not raised, and no device content was replaced.

## Performance acceptance remains open

Reran `tests/target_budget.py` against both playback and comparison-baseline disassembly.
Both return failure. Audio ISR weighted static cost is 281 with playback versus 268
without it, against the checked-in budget of 174. FM6 render functions also lack budget
entries. The 13-unit increase is approximately 4.9% of the comparison metric; it is not
an actual CPU utilization measurement and does not include all called playback work.

This agrees with the USB implementation note. Do not treat passing functional tests or
linking as CPU signoff. Extend the shared budget coverage and measure worst-case target
audio deadlines, nested USB service, underruns, latency and fidelity under synth/FX load.
Physical EP4 IN/OUT FIFO/DMA behavior, Android/Windows enumeration, sustained duplex audio,
removal and restart still require FM1 testing. ASan remains unverified.

## Integration resolution

All three findings were corrected and have regression coverage: unchanged immutable native
edits retain their original data/identity and avoid false locked changes; unsupported negation
rejects the entire sound request; non-finite resampling intermediates reject huge finite gains.

The libraries/test projects are now in Sloop.slnx and Android consumes them through
Sloop.Workstation. Manual FM6 and prompt proposals, native/app sequencing, sample
conversion/preview/backup transfers, local persistence and the chord/performance instrument
are accessible in the installed app. See [INTEGRATION.md](INTEGRATION.md) for current counts
and [IMPLEMENTATION.md](IMPLEMENTATION.md) for the remaining work.

Current validation: 1,327 sound assertions, 17 sequencing scenarios, 102 encoding assertions
with nine golden fixtures, 400 protocol/simulator/sampling checks and 53 Workstation checks.
Pixel checks cover prompt apply, note insertion/restart persistence, synthetic WAV conversion
and encoded preview, chord latch/joystick, drum/keyboard/ribbon/arp controls and touch cancellation.

The shared firmware runner now includes playback tests and static diagnostics include
uac_play_mix (reported cost 2,810 without an accepted budget in the validation build).
The audio ISR budget failure and physical gates above remain open. Combined USR3+4,
full arrangement/scene/model workflows and hardware bank/A-B integration remain planned.
Current firmware remains three synth tracks plus one drum track.

## UI refinement verification — 2026-10-07

The refined APK builds with zero warnings/errors and is installed on the Pixel 7a; its final
cold launch succeeds. All 53 Workstation checks pass. Phone checks confirm waveform tap/drag
cursor placement, a split increasing the chop count, Undo restoring the prior edit, chop
audition reaching Preview complete, selected-chop export state, readable two-line pad labels,
Chop-to-Send navigation resetting scroll, kit conversion (2,270/81,408 bytes for the retained
trim), collapsed optional name/root settings and sequence bar 4 selecting step 49. Screenshots
were reviewed and bottom-tab wrapping corrected. FM6 sections and selected operators remain
reachable. Physical slot transfer/progress and FM1 USB audio still require hardware acceptance.

## Octave and generic MIDI checkpoint — 2026-10-07

61 Workstation and 410 protocol/simulator/sampling checks pass. Tests cover signed hardware
octave values -3..3, malformed state replies, empty-argument state requests, app fallback,
fixed octave, selected-register voice leading, held-note octave replacement and standard
channel-16 note bytes. Android builds with zero warnings/errors and cold launches on Pixel.
Phone checks show direct Oct+ changes the readout from 3 to 4 and the latched C chord to
60/64/67. The corrected connection dialog shows SLOOP, generic, simulation, disconnect and
status details separately. Generic selection reaches no-destination guidance while no MIDI
instrument is attached. Slot confirmation text and slot choices now use separate dialogs.

The protocol-10 firmware package builds successfully (569,420-byte image; 90,612-byte RAM;
334,560-byte pool) and is retained as build/sloop-mobile-protocol10.fwsc. It has not been
flashed. Real FM1 OCT-button tracking and a non-FM1 generic destination remain pending;
passing codec/ownership/UI checks does not establish these physical behaviors.

Branding rebuild: default FELUCCA_VERSION is 2.4.1 Merthsoft; target image is 569,404 bytes with unchanged 90,612-byte RAM and 334,560-byte pool usage. The package is also retained as build/sloop-2.4.1-Merthsoft.fwsc. No device was flashed.
