# Measured branch headroom

## Integrated review build

Removing obsolete parameter-icon lookups and generating only the two header
icons actually drawn by NEON reduces the same-version target image from
**584,336 to 581,296 bytes**, recovering **3,040 B (2.97 KiB)**. Main static
RAM remains **89,128 B**, low RAM **28,280 B**, and the audio pool **332,876 B**.
The subsequent combined review build is **579,744 B**, leaving **4,592 B** free,
with main static RAM **86,128 B** (3,000 B below the icon-cleanup baseline).
This includes correctness fixes as well as measured cleanup; isolated savings
must not be summed to infer its result. See the
[measurement and validation](OPTIMIZATION.md#unused-parameter-icons-after-the-neon-overhaul).

The subsequent upstream controls batch adds 1,004 B: **580,748 B**, leaving
**3,588 B** free, with the same main/low/pool static RAM totals.

The live progression/input follow-up is **582,048 B**: **2,288 B** flash free,
**86,144 B** main static RAM used. Relative to .51 this combined follow-up adds
1,300 B of application data/code and 16 B of main static RAM. Low RAM and the
audio pool are unchanged.

The appended fourteen-instrument pack is **582,424 B**, leaving **1,912 B**
flash free. It adds **376 B flash** and **504 B low RAM**, with no new DSP
buffers or synthesis code. Low RAM is **28,784 / 29,496 B** (712 B free);
main static RAM and the audio pool remain unchanged.

The completed selected 2.8 integration is **582,968 B**, leaving **1,368 B**
flash free. It adds 544 B flash and 32 B main static RAM; low RAM and the audio
pool are unchanged. The comparison below reflects this build.

The LOFI experiment pack is **583,348 B**, leaving **988 B flash**. It adds
380 B flash and 144 B low RAM, without new audio buffers. Low RAM is 28,928 B
(568 B free); main RAM and pool are unchanged. The tape/sampler refinement recovers another 24 B, reaching 583,324 B with 1,012 B free. The subsequent sampler gain correction and gentler drive curve add 64 B, reaching 583,388 B with 948 B free. The first ambient headroom integration reaches **581,996 B** (2,340 B free), main RAM **86,320 B**, and pool **331,852 B**. The first ambient feature slice adds 664 B flash and 112 B main static RAM: **582,660 B**, leaving **1,676 B flash**. DRIFT and three sustained starters are included; pool and low RAM are unchanged. The DRIFT depth/LOFI/display refinement then reaches **583,180 B**, leaving **1,156 B flash** with static RAM unchanged. Tap-toggle DRONE then reaches **583,716 B**, leaving **620 B flash** with static RAM unchanged. Independent reverb tail HOLD reaches **584,000 B**, leaving **336 B flash** with static RAM unchanged. The metadata packing and source cleanup batch, including requested independent HOLD removal, reaches **582,804 B**, leaving **1,532 B flash** and **8,896 B main static RAM**. The subsequent HOME accents and empty-step move fix reach **582,980 B**, leaving
**1,356 B flash**, with static RAM unchanged. The subsequent build-time LZ4
search improvement reaches **582,212 B**, leaving **2,124 B flash**. HOME lane
background removal is included; RAM remains unchanged. EVOLVE adds 752 B flash
and 48 B main static RAM: **582,964 B**, **1,372 B flash free**, and
**8,848 B main RAM free**. Sustain, expanded/synced EVOLVE cycles and the next
build-time parser improvement combine at **584,064 B**: **272 B flash free**,
main static **89,616 B** (8,688 B free). Cold persistence/editor sharing and weighted voice admission plus FM6 eight, including steal-fade retrigger protection, then reach **583,776 B**: **560 B flash free**, with all static RAM totals unchanged. The subsequent PIANO/STRINGS/LOFI sustain characters and DAMP/AMOUNT controls reach **584,204 B**, **132 B flash free**, main static **89,936 B** (8,368 B free). Low RAM and pool remain unchanged. The subsequent adjustable voice budget and per-pitch drone recovery, funded partly by native ordered clamping, reach **584,280 B**: **56 B flash free**, main static **90,336 B** (7,968 B free). Low RAM and pool are unchanged. Extending admission to five levels through 48 units adds 20 B flash and no RAM, reaching **584,300 B**, **36 B flash free**. The table reflects this final combined build.

Both builds use the installed JieLi 20250324.1 toolchain and pinned AC79 SDK.
Main was exported to an isolated directory, generated its own assets and ran
its own normal build, including loader/package and target checks. No branch
checkout or main ref update was needed.

| Region | MerthSloop working release | Local main |
| --- | ---: | ---: |
| Application bytes | 584,296 | 567,308 |
| Application capacity | 584,336 | 581,564 |
| **Flash free** | **40 B (0.04 KiB)** | **14,256 B (13.92 KiB)** |
| Main static RAM used / capacity | 90,336 / 98,304 | 59,004 / 98,304 |
| **Main static RAM free** | **7,968 B** | **39,300 B** |
| Low RAM used / capacity | 28,928 / 29,496 | 22,772 / 29,488 |
| **Low RAM free** | **568 B** | **6,716 B** |
| Audio pool used / capacity | 331,852 / 344,064 | 333,900 / 344,064 |
| **Audio pool gap** | **12,212 B** | **10,164 B** |

The pool retains an 8,192-byte build reserve; the entire gap is not freely
spendable. The ordinary/low/pool regions cannot simply be added to obtain one
allocatable RAM budget. These are static linker numbers, not stack high-water,
heap availability or CPU percentages. Low RAM capacity differs because the
preceding RAM-resident sections have slightly different extents.

MerthSloop is build **2.8.0.64**, including the integrated review/headroom work and first ambient slice. Main is
**b77bf55e3bf804665b8f5dffb5955bd8443311f0** (the upstream 2.8 source).
Fetched upstream/main **ec9e537** adds only a GUIDE change over that local main,
so it has no compilation-source changes. Main uses its normal **-Os**;
MerthSloop uses its reviewed **-Oz** and explicit hot helper inlining.

This is each branch's real configuration, not a controlled optimization delta:
MerthSloop has extra features and assets. Its sample generator emits 51 zones
and 303,054 ADPCM bytes; main emits 49 zones and 283,212 bytes. MerthSloop also
uses startup metadata restoration and the larger validated package extent.
Do not attribute the 12,436-byte application difference solely to our new code.

The earlier .40 release had 4,024 B flash free, .41 had 4,276 B and .42 had
3,684 B. The integrated progression performance page/source/lifecycle adds
2,516 application bytes and 16 main static RAM bytes over .42, leaving
**1,168 B flash free** at .43. Radio/Dark/Dub and direct chord performance add
a further 608 image bytes / 16 main RAM bytes, leaving **560 B flash free** at .44. The response/transition tuning adds another
384 image bytes, leaving **176 B flash free** with unchanged linked RAM. The frozen same-version redundancy comparison remains
a separate measurement.

Private evidence:

- `build/main-headroom/source/`: isolated main sources, generators and final
  `build/felucca.{bin,elf,dis,fwsc}`.
- `build/redundancy-audit/results/baseline/` and `results/both/`: controlled
  256-byte UI sharing comparison, RAM/placement and DSP checks.
- `build/felucca.{bin,elf,dis,fwsc}`: final versioned MerthSloop release.

See [architectural headroom](ARCHITECTURAL-HEADROOM.md),
[redundancy audit](REDUNDANCY-AUDIT.md) and
[upstream candidates](UPSTREAM-OPTIMIZATION-CANDIDATES.md).

The local .75 diagnostic-only core-balancing/threshold trial measures 584,316 B (20 B flash free), main static 90,368 B (7,936 B free). Its normal configuration remains 584,300 B / 90,336 B, matching the table. Low RAM and pool are unchanged. [Trial evidence and qualification](VOICE-BUDGET-EXPERIMENT.md#core-balancing-trial-after-the-longer-hardware-run).

Normal .76 restores About and adds configurable EVOLVE octave bounds while retaining the successful audio policy: 584,296 B (40 B flash free), main 90,336 B (7,968 B free), low/pool unchanged. The table remains current.


### Track snapshots and shared ANALOG renderer

The .77 saved-budget build is 584,328 B (8 B free). The combined .78 snapshot feature and shared ANALOG body link at **583,864 B (472 B free)**, a **464 B net reduction** including the new feature. Main static RAM is **91,216 B (7,088 B free)**; low RAM 28,928 B and pool 331,852 B are unchanged. No per-hit decoding or new per-block morph runner is added. The shared sample loop has zero helper calls and weighted cost 640 against the original 638 budget (+0.3%, static estimator only). Hardware cycles/stack are not qualified by this comparison. All 200 audio goldens, snapshot/lock sanitizer checks, dual-core/timeout parity and UI navigation/fuzz pass. See [track snapshots](TRACK-SNAPSHOTS-DESIGN.md).


The .79 ambient-control reorganization and four combined drift targets add 32 B to .78: **583,896 B / 440 B free**, with all static RAM regions unchanged. One drone mode replaces the prior coupled DRIFT/EVOLVE switches; targets reuse the existing motion generator.
