# Scene RAM transaction extension, revision 1 (command 72)

Implemented staging contract, October 8, 2026. This is a reserved extension under
the existing `F0 7D 46 4C command args F7` envelope. All arguments are 7-bit.
Protocol 11 advertisement in parent-owned editor.c is still required. Hosts must
not probe this on INFO <=10: old firmware can time out and fault EditorClient.
The shipped source currently retains INFO 10, so production UI uses its existing fallback.

Unsigned integers use little-endian base128 digits: u14 = two bytes, u28 = four.
CRC-32/ISO-HDLC uses five digits, with final digit <=15 (full 32 bits).
Tokens are 1..16383, strictly increasing in a connection epoch; reconnect before
exhaustion. One transaction is retained. Epoch is 1..0xfffffff and increments on
USB reset/suspend/loss. Exhaustion disables transactions until reboot. Reboot
starts a fresh connection; clients cannot carry prior tokens across reconnect.

| Op | Request arguments after op | Meaning |
|---|---|---|
| 0 Capabilities | none | Discover schema/limits/current revision/installed hooks |
| 1 Begin | epoch u28, token u14, expected revision u28, length u14, CRC u35 | Reserve single RAM staging slot |
| 2 Data | epoch, token, offset u14, pack7 data | Contiguous <=96 raw bytes, duplicate identical chunks allowed |
| 3 Prepare | epoch, token | CRC, schema and installed engine/dependency validation |
| 4 Commit | epoch, token, absolute tick u28, boundary kind | Queue an engine-validated future boundary |
| 5 Cancel | epoch, token | Cancel receiving/prepared/queued; cannot undo applied |
| 6 Status | epoch, token | Resolve queued vs applied/failed/canceled |

Every reply is exactly 24 argument bytes:
`op, rc, state, epoch[4], token[2], received[2], revision[4], boundary[4],
kind, maximum[2], chunk, flags`.
Flags 7 mean installed atomic sound/pattern/tempo hooks; flags 0 prohibit staging.
Revision is current engine revision in capabilities, expected baseline otherwise,
and observed post-apply revision in Applied. No asynchronous push is introduced.
Status of Failed carries its failure reason in rc. Boundary remains the requested
tick; Applied is possible only at that exact tick. Queued is acceptance, not success.

RC: 0 OK, 1 Invalid, 2 Unsupported, 3 Busy, 4 Conflict, 5 Missing,
6 TooLate. States: 0 Idle, 1 Receiving, 2 Prepared, 3 Queued, 4 Applied,
5 Canceled, 6 Failed. Kind: 0 beat, 1 bar, 2 phrase.
Wrong epoch/token cannot mutate staging. Identical Begin, chunk, Prepare and Commit
retries do not apply twice. A repeated commit must specify the identical boundary.
Only the current transaction is cached; beginning a newer one makes older tokens
unqueryable. Malformed/out-of-order/oversized chunks never advance the cursor.

## Canonical raw schema

Exactly 2956 bytes; maximum allocation 3072 bytes. Header: `version=1,
tempo unsigned u14 (20..300), synth mask=7`. Three ordered synth records,
tracks 0..2, each 984 bytes:

| Offset within track | Bytes | Field |
|---|---:|---|
| 0 | 128 | FM6 packed patch; each byte <=127, not pack7 |
| 128 | 14 | Seven existing macros, each signed14 offset by 8192 |
| 142 | 1 | Native pattern length 1..64 |
| 143 | 1 | Existing N_SDIV index 0..8 |
| 144 | 704 | 64 records: notes[4], n, time, flags, velocity, packed level, packed ratchet, micro+32 |
| 848 | 16 | Four 2-bit fill conditions per byte; reserved condition 3 rejected |
| 864 | 120 | 24 records: step (255 free), param ID, signed14 value[2], reserved=0 |

All 64 steps are owned, including inactive tail steps. Notes <=127, n <=4,
time <=2, flags <=3, velocity <=127, micro encoded 0..63. Level/ratchet are raw
8-bit bytes and therefore require pack7 during transport. Locks use 0..63 steps
or 255 free; installed engine validator must additionally enforce lockable params,
parameter ranges, uniqueness, FM6 packed field ranges, macro ranges and destination
engine compatibility. The standalone validator checks structural bounds only.

Samples, drums, native project references, engine switching and flash saves are
outside this schema. Missing/unsupported sample dependencies explicitly reject
host preparation. Verified flash dependencies must already exist; the engine
validator must independently verify actual slots and prevent conflicting writes.

## Ownership and recovery

Main loop owns staging; audio may read it only in Queued. Critical hooks serialize
descriptor commit/cancel/reset/status with the audio ISR. CRC and semantic validation
run outside critical sections. Commit publishes payload/descriptor with a compiler
barrier, and Queued is volatile; no heap, flash or USB work occurs at application.
An ISR-observed USB loss latches cancellation immediately, before deferred main-loop
epoch reset. Hook registration occurs once while stopped before USB servicing.

Parent must provide every hook, integrate exact sequencer boundary callbacks before
events/locks, and cancel on stop/seek/restart/project adoption and inactivity expiry.
The schedule hook must reject past ticks, wrong musical boundaries and epochs, and
ensure a callback cannot skip the selected tick. If a callback is late, it fails
closed without applying. apply must perform the entire validated sound/pattern/tempo
swap and outgoing-note cleanup without failure, then publish a fresh revision.
Queued storage cannot be reused until canceled/applied/failed.

EditorClient remains conservative: timeout/cancellation faults the host connection;
no blind mutation retries. A lost commit acknowledgement requires reconciliation.
Reconnect alone is not proof that a scene was applied: reset may cancel it, or it
may have applied before loss. Read complete actual state before publishing success
or resuming fallback; partial/unknown recovery uses existing stopped-only behavior.
