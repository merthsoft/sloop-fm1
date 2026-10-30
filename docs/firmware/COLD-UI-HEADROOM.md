# Cold UI and engine-table headroom

The .105 pass reduces the same-version .104 application image from **582,936 to 582,196 B**, recovering **740 B net**. The final .105 image has **2,140 B free** out of the 584,336 B application allocation.

| Resource | Before | After | Change |
| --- | ---: | ---: | ---: |
| Application flash | 582,936 B | 582,196 B | -740 B |
| Main static RAM | 97,724 B | 97,340 B | -384 B |
| Low RAM | 29,288 B | 29,288 B | unchanged |
| Audio pool | 335,424 B | 335,424 B | unchanged |

The existing 8,192 B pool reserve remains. No sounds or features were removed, and no playback decoding was introduced.

## Measured steps

These are sequential target links, not estimates from C source size:

| Change | Linked image | Additional flash saved |
| --- | ---: | ---: |
| Baseline | 582,936 B | — |
| Engine metadata layout | 582,840 B | 96 B |
| Obsolete UI lowercase conversion | 582,760 B | 80 B |
| Unreachable glyph aliases | 582,408 B | 352 B |
| More existing startup literal packing | 582,196 B | 212 B |

Engine metadata drops two unread fields (`color`, `fil_page`) and groups the small flags. Target `engine_t` shrinks from 152 to 144 bytes. All thirteen initializers use designated fields so callbacks and capability flags remain explicit.

The text renderer already folds case to capitals. UI helpers no longer lowercase text immediately before rendering. At font generation time, unreachable lowercase/control slots alias the glyph the renderer already selects. The original BDF and renderer are unchanged. The packed small-font bitmap shrinks from 3,952 to 2,822 B; restored table alignment yields 1,136 B main RAM savings. This deliberately changes unreachable raw glyph slots, not displayed characters.

Of that recovered RAM, 750 B funds the existing startup-only literal pool: its main-RAM limit rises from 5,500 to 6,250 B, with its pool-RAM limit unchanged at 2,500 B. Linked main RAM still decreases by 384 B after alignment. Larger limits were rejected: another 250 B RAM bought only 15 B payload reduction. Startup restoration already exists; audio processing does not change.

A shared LZ4 length-decoding helper was also measured and rejected because it grew the target image by 64 B. The original decoder remains.

## Verification

- 79 full-frame screen captures match the frozen pre-change UI byte for byte at the same version; UI fuzz and live editing/control checks pass.
- All 210 existing audio goldens and health checks pass without changed expectations. All 181 factory preset records match both raw and startup-restored baselines.
- Target DSP instruction, loop, call and division metrics are identical to the frozen .104 disassembly. Inlining and master LOFI loop guards pass.
- Every rendered input character matches an independent reference from the original BDF, at both scales with clipping/color cases; the legacy four-bit renderer also passes.
- Descriptor metadata, all formatted values/units and actual editor descriptor replies match the frozen original hashes. Optional SLICE metadata/display equivalence passes too.
- Startup literal validation checks 1,412 complete original objects, final relocated pointers, three cold restores, guard bytes and UBSan.
- Persistence, upstream project compatibility, explicit SONG SAVE, snapshot, preset browser, LOFI, reverse-delay and split-mixer checks pass.

Private evidence is under `build/headroom-105/`. These are build and host checks; hardware cold-boot qualification of the final image remains useful.
