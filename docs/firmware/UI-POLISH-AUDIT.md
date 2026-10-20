# Native UI polish audit

The review uses fresh 240 × 240 captures from the production renderer, not earlier
mockups or phone screenshots. A local capture sweep renders every engine's page
set, the five drum views, all four sequence-library subpages and all six settings
sections. The regular UI suite supplies held-key layers, confirmation states,
recording, user themes, preset browsers and all seven visualizers. Repeated engine
pages are compared as a family. Actual navigation tests remain the authority for
which pages are reachable; forcing the SEQUENCES descriptor without opening its
browser is not a user-visible screen.

## Findings and changes

| Screen family | Review result |
| --- | --- |
| Shared transport header | Add a four-pixel gutter between the tape icon and track badge. Keep battery/USB space and octave readouts. |
| Settings, all six sections | Replace overflowing full-name tabs with SCR / LGT / AUD / MIDI / TIME / SYS; the header retains the full selected section name. |
| Palette settings | Give five swatches fixed space beside the value; long custom names use the smaller font when needed. |
| About | Put version and MERTHSOFT in the header. Use full-height credit lines, split the repository URL, and increase credit contrast. Preserve author, instrument maker, licenses and CPU diagnostics. |
| Ordinary engine and global pages | Keep complete summary values such as SMOOTH. Select a smaller font when the complete value plus unit would cross the cell boundary. Card labels can use all six available characters. |
| Envelope/LFO destinations | Scale signed bars to descriptor limits so maximum positive/negative values remain inside the guide. No audio modulation behavior changes. |
| Held step controls | Use RATCH rather than a clipped RATCHE label. |
| HOME | Retain the sunset, beat pulse, engine label and track lanes; use the shared corrected header. |
| Envelope/LFO curves, delay/filter guides | Retain current schematics; confirm captions and shared cards clear the footer. |
| Sequencer note editor, pattern and record settings | Retain note/tie grid and OCT-held tools; review empty, populated and held states. |
| Drum GRID / KIT / SOUND / GROOVE / RHYTHM | Keep the current page-specific layouts and controls; inspect labels, grids and audition/apply footer. |
| Sequence library / rhythm / pitch / custom progression | Keep full progression/rhythm names in the body and compact controls above; review all four pages. |
| SONG / record-ready / count-in / free-take | Retain the preceding shared-card pass; verify longest arrangement and state transitions. |
| Held key / mix / roll / song / punch and confirmations | Preserve pad layouts, lock/condition state and action hints. |
| Seven visualizers | Retain their distinct analytical layouts; no new rendering or DSP work in this pass. |
| Page help and custom light/dark themes | Preserve control ordering and actual hold hints; validate incremental/full redraw parity. |

## Verification

`tests/ui_refresh_test.c` searches for complete rendered glyphs rather than merely
checking that drawing stayed in bounds. It covers all six tabs in each selected
section, an eight-character custom palette name with all five swatches, the About
version/attribution and final credit, full summary values/units, modulation guide
extrema and the header gutter. Existing tests cover full/incremental parity,
palettes, recording transitions, arrangement extremes, popup state isolation and
the full delay caption. `tests/ui_pages_test.c` also runs 20,000 random frames with
undefined-behavior diagnostics enabled.

Fresh captures are shown in the [UI design/gallery](UI-REFRESH-DESIGN.md). The
review does not measure physical LCD refresh timing or audio scheduling on hardware.

## Deferred design work

Engine-specific diagrams remain deferred. A useful next design pass could improve
empty-pattern guidance and specialized preset-browser navigation, but should be
reviewed separately from clipping fixes. Do not reintroduce the withdrawn FM6
routing prototype or add a second UI framework. Skiff effect selection, transport
HALF behavior and simultaneous effects are separate audio/input workstreams.
