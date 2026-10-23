# Measured branch headroom

## Current source cleanup (not yet published)

Removing obsolete parameter-icon lookups and generating only the two header
icons actually drawn by NEON reduces the same-version target image from
**584,336 to 581,296 bytes**, recovering **3,040 B (2.97 KiB)**. Main static
RAM remains **89,128 B**, low RAM **28,280 B**, and the audio pool **332,876 B**.
The served .49 package is unchanged; the table below describes that published
package, not the smaller development image. See the
[measurement and validation](OPTIMIZATION.md#unused-parameter-icons-after-the-neon-overhaul).

Both builds use the installed JieLi 20250324.1 toolchain and pinned AC79 SDK.
Main was exported to an isolated directory, generated its own assets and ran
its own normal build, including loader/package and target checks. No branch
checkout or main ref update was needed.

| Region | MerthSloop working release | Local main |
| --- | ---: | ---: |
| Application bytes | 584,336 | 567,308 |
| Application capacity | 584,336 | 581,564 |
| **Flash free** | **0 B** | **14,256 B (13.92 KiB)** |
| Main static RAM used / capacity | 89,128 / 98,304 | 59,004 / 98,304 |
| **Main static RAM free** | **9,176 B** | **39,300 B** |
| Low RAM used / capacity | 28,280 / 29,496 | 22,772 / 29,488 |
| **Low RAM free** | **1,216 B** | **6,716 B** |
| Audio pool used / capacity | 332,876 / 344,064 | 333,900 / 344,064 |
| **Audio pool gap** | **11,188 B** | **10,164 B** |

The pool retains an 8,192-byte build reserve; the entire gap is not freely
spendable. The ordinary/low/pool regions cannot simply be added to obtain one
allocatable RAM budget. These are static linker numbers, not stack high-water,
heap availability or CPU percentages. Low RAM capacity differs because the
preceding RAM-resident sections have slightly different extents.

MerthSloop is build **2.7.1.49**, including the shared UI cleanup. Main is
**b77bf55e3bf804665b8f5dffb5955bd8443311f0** (the upstream 2.8 source).
Fetched upstream/main **ec9e537** adds only a GUIDE change over that local main,
so it has no compilation-source changes. Main uses its normal **-Os**;
MerthSloop uses its reviewed **-Oz** and explicit hot helper inlining.

This is each branch's real configuration, not a controlled optimization delta:
MerthSloop has extra features and assets. Its sample generator emits 51 zones
and 303,054 ADPCM bytes; main emits 49 zones and 283,212 bytes. MerthSloop also
uses startup metadata restoration and the larger validated package extent.
Do not attribute the 16,852-byte application difference solely to our new code.

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
