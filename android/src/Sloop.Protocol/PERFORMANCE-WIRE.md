# FM1 remote fills and punch effects

Command **73** is reserved exclusively for this feature. Production discovery requires
INFO protocol **12**. It does not change firmware identity/version. Schema **1** is separate
from INFO version. All bytes are unsigned MIDI-safe 7-bit values; tokens are unsigned u28,
four bytes least-significant first, **not** the offset signed editor-value format.

| Request arguments | Reply arguments | Meaning |
| --- | --- | --- |
| `0` | `0 status schema flags owners leaseLo leaseHi effects` | Discover |
| `1 token[4] kind value` | `1 status token[4]` | Press/retain owner |
| `2 token[4]` | `2 status token[4]` | Release, idempotent |
| `3 token[4]` | `3 status token[4]` | Renew existing owner only |
| `4` | `4 status 0 0 0 0` | Clear host ownership |

Statuses: 0 OK, 1 Malformed, 2 Full, 3 Missing. Unknown opcodes/lengths return Malformed;
no mutation occurs. Discovery returns schema=1, flags=7 (bit0 held fill, bit1 punch,
bit2 next-bar fill), owners=8, lease=750 milliseconds (unsigned u14), effects=16.
Client rejects unknown schemas/capability shapes and never probes protocol <12.

Kinds: 0 held fill (value=0), 1 punch (value 0–15), 2 next-bar fill (value=0).
Punch indices match the existing DSP: Loop4, Loop8, Loop16, Loop32, Stutter, Reverse,
TapeStop, Half, LPF, HPF, Phone, Crush, Alias, Gate, Echo, Wobble.
Tokens must be nonzero, distinct for each fresh gesture, and monotonically increasing
within an EditorClient epoch. Duplicate press must keep kind/value; renewal never creates
an owner. Latest live token wins among host punches. Panel punch has unconditional priority;
panel release reveals an independently held host punch. Fill owners aggregate with panel fill.

Next-bar fill must remain held/renewed until the next transport bar; it starts there and ends
on the subsequent bar or earlier release/lease expiry/STOP/host loss. This intentional held
control avoids an unbounded armed host request at a slow tempo. Multiple owners aggregate.
The remote bar is separate from physical fill_arm/fill_bar_on. Only host owners are cleared
by command4, lease expiry, USB unconfigure/reset. STOP clears all remote ownership too.
DSP transitions use the existing punch_process crossfade and effects, never synthetic keys.
The eight-entry table uses 96 bytes (8 × 12) plus four bytes for USB reset tracking;
no dynamic allocation, flash operation, USB work or unbounded loop runs in audio callbacks.

## Integrated shared hooks

The engine helper is included by punch.c; punch_process already uses panel-priority
`punch.req >= 0 ? punch.req : perf_punch(fm1_ms)`. The parent integrated these hooks:

1. In editor.c, after ed_* response helpers and before ed_handle: `#include "editor_performance.c"`.
2. In command dispatch: `case 73: ed_performance(a, na); break;` (use actual argument-count name).
3. At the beginning of ed_sync, **before WATCH's early return**: `ed_performance_service();`.
   This must execute with WATCH disabled. INFO reports protocol12 with the production hooks installed.
4. In seq_stop: `perf_reset();` before clearing physical fill state.
5. At each detected new bar, before `fill_bar_on = fill_arm`: `perf_take_bar(fm1_ms);`.
6. Per-block fill aggregation: `fill_now = (uint8_t)(fill_held || fill_bar_on || perf_fill(fm1_ms));`.
7. In hardware transport reset/project adoption/panic paths, call perf_reset where STOP does
   not already run. Keep physical cleanup semantics unchanged.

MainActivity's PerformanceControlEditor already invokes AddHardwarePerformanceControls
through AddPerformanceMacro. No duplicate render hook is needed. Integrated Android hooks:

1. After handshake has published DeviceInfo, call
   `await InitializeHardwarePerformanceAsync(editor, current.Token);` for physical FM1.
2. At start of StopPerformance: `ClearHardwarePerformanceTouches();` (includes navigation,
   focus loss, background, settings, track and explicit release paths).
3. Before disposing a MIDI connection/epoch, call CloseHardwarePerformanceAsync. Cleanup is
   best effort; retain its task until complete when transport remains usable. It immediately
   detaches the session and cancels renewal even when the caller cannot await disconnect.
4. Before entering an exclusive device operation, await ClearHardwarePerformanceAsync.
   Controls hide while Busy; existing touches should also be suppressed through StopPerformance.
5. If refresh does not reconstruct Perform on capability discovery, call ShowWorkspace from
   the existing Changed observer. New controls render only when CanPerformHardware is true.

Dedicated host runners: `dotnet run --project android/src/Sloop.HardwarePerformance.Tests`
(46 checks), and compile/run tests/performance_remote_test.c (16400+ malformed cases and
ownership, priority, expiry/wrap, USB loss/reset, STOP). The real-engine runner tests/performance_engine_test.c also passes physical keyboard ownership,
actual punch DSP priority/release, simultaneous panel/host bar fills, release during a host bar,
expiry before a boundary and real transport STOP. Parent verification reports the combined
APK build at zero warnings/errors and the target firmware at 93,332/98,304 RAM.
Phone and physical FM1 acceptance have not been run for this feature.
