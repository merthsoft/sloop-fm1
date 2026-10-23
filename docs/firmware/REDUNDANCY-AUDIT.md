# Firmware redundancy audit

## Scope and method

Reviewed the native firmware, concentrating on branch-added performance layers,
starter browsers, piano-roll actions, effects and NEON rendering. A private
source scan compared 510 nontrivial function bodies and repeated 48-token
windows. This is a candidate finder, not a proof of no duplication: conditional
compilation, renamed variables and equivalent algorithms require manual review.
Overlapping matches are not independent savings and are never summed as bytes.

Each accepted change is measured with the real pinned JieLi compiler in an
isolated snapshot with identical assets, flags and version. The baseline
reproduces the served application exactly at **580,312 bytes**. Evidence and
the scan live in ignored `build/redundancy-audit/` and
`build/headroom-study/clones.{txt,json}`. No production audio algorithm,
sample format, storage format or compiler flag changes are needed.

## Findings

| Area | Finding | Disposition |
| --- | --- | --- |
| MIDI and panel step entry | The same entry-open/undo/tie cleanup transaction was copied in two callers. | One cold `step_entry_begin` helper; both keep their input-specific note/chord handling. **96 B saved** alone. |
| HOME and ordinary parameter columns | Descriptor formatting, icon, gauge and card rendering were implemented separately. | One cold `draw_parameter_column` helper. Context-specific overrides remain guarded; HOME retains its original labels and boolean gauges. **160 B saved** alone. |
| Vibrato/tremolo/wah/pan | Already use one `live_mod` state shape, grouped knob handling, shared waveform renderer and grouped layer display. | No four copied engines to remove. Retain their different signal processing. |
| NOTE ACTIONS / HIT ACTIONS | Already share draw, selection, stop checks, undo and action dispatch in `ui_note_actions.c`. | Their musical-chain and drum-lane mutation helpers operate on different data and have different collision rules; retain those. |
| Drum grooves / musical starters | Already share pattern metadata reset/rhythm-shaping infrastructure. Their browser bodies, audition setup and confirmations retain parallel scaffolding. | Candidate for a bounded browser/footer/confirmation helper; no measured additional saving claimed. Avoid a generic browser framework. |
| STEP / NOTES editing | Parallel editing paths contain overlapping pitch/length/move behavior. | Larger consolidation opportunity, but whole-step movement preserves locks/microtiming and legacy tie rules; selected-note movement has independent ties and polyphony/collision rules. These are not interchangeable implementations. |
| Shared page controls | Input/help/display already obtain values through `page_desc`. Dynamic label overrides still live separately in drawing. | Resolve label/value metadata together in a follow-up. Current help can show a descriptor label rather than the TAPE TONE override; centralizing that would prevent this drift. |
| Reverb models | STEREO/SPRING/ROOM/PLATE/HALL share infrastructure and model parameters. | Multiple selectable models do not mean five copies of the reverb loop. |
| ADPCM decoding | Sample playback and texture sample playback share a mathematical decode step but have different state, loop, release and wrapping policies. | Intentional hot-path specialization. Do not add per-sample generic calls merely to reduce source repetition; require target cycle/call and output proof first. |
| Scan false positives | Debug console reads, optional SLICE rendering and USB preprocessor alternatives appear similar in source. | Console/SLICE are excluded from this target; conditional alternatives cannot be counted as multiple active implementations. |

This audit did not find evidence of large verbatim copies of the main new
features. It does identify maintenance duplication and several opportunities
for deeper UI consolidation. That is narrower than a guarantee that the entire
firmware is free of redundant logic. The earlier architectural UI estimate is
still unmeasured; the **256-byte** result here does not validate a multi-KiB claim.

## Controlled target measurements

| Snapshot | Application bytes | Saved | Main / low / pool RAM |
| --- | ---: | ---: | --- |
| Baseline | 580,312 | — | 89,080 / 28,280 / 332,876 |
| Entry transaction shared | 580,216 | 96 | Unchanged |
| Parameter column shared | 580,152 | 160 | Unchanged |
| Both | **580,056** | **256** | Unchanged |

The combined candidate leaves **4,280 bytes** in the 584,336-byte app slot.
The target placement/safety checks, DSP budgets and all 24 hot-function
loop-call caps pass. This changes UI code only; no new audio-loop calls.

Validation compares **116** baseline and candidate framebuffer captures byte-for-byte,
runs the full actual-UI 20,000-frame fuzz for each, and exercises UI refresh,
delay controls and both step-entry callers. A new regression test covers HOME
after every page and LOFI type, plus physical/MIDI entry replacing stale cell
state once and accumulating a chord within one undo transaction. This guards
against a common sharing regression: leaking special-page labels onto HOME.

The cleanup is packaged separately as build 2.7.1.41, rather than replacing
the previous package with different bytes under the same version. The final
versioned image is 580,060 B (4,276 free); the four-byte difference from this
frozen comparison includes the new build/version. See the
[headroom comparison](HEADROOM-COMPARISON.md).

## Next worthwhile work

The later NEON icon audit removes a dead UI lookup/asset dependency rather
than a copied algorithm: the cards retained icon selection after icon drawing
was removed. Together with an atlas containing just the two visible header
icons, this saves **3,040 linked bytes**, with 113 identical framebuffer
captures and no RAM increase. See
[the measured cleanup](OPTIMIZATION.md#unused-parameter-icons-after-the-neon-overhaul).

1. Share page field metadata, including display labels, rather than resolving
   related state independently in help, graphs and input.
2. Prototype one browser confirmation/footer path across the two starter
   browsers, measure it, and keep it only if the total target shrinks.
3. Refactor selected-note/whole-step editing only after explicitly documenting
   their different metadata and collision policies. Share proven primitives;
   do not flatten away useful behavior to get one implementation.

See [architectural headroom](ARCHITECTURAL-HEADROOM.md) for the larger program,
and [upstream suggestions](UPSTREAM-OPTIMIZATION-CANDIDATES.md) for changes that
are useful outside this fork.
