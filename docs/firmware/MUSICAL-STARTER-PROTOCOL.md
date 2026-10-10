# Firmware musical starter protocol

Command **76**, schema **1**, is additive to INFO protocol version **14**. Command 74 drum grooves and 75 USB playback retain their contracts. Frames use the existing FL SysEx envelope and seven-bit payload bytes. Clients must require protocol >=14 **and** validate command 76 capabilities; a version number alone is not discovery. Unsupported devices show an update message, with no fallback phone generator.

Requests/replies:

| Operation | Request payload | Reply payload |
|---|---|---|
| Capabilities | `0` | `0,0,1,24,7,scaleCount,leaseLow,leaseHigh` |
| Starter names | `1` | `1,0,count,(id,nameNUL)*` |
| Apply | `2,track,id,key,scale,octave,mode,vlead,rotate,offset,sync,feel,confirm` | `2,status` |
| Start audition | Apply fields with operation `3`, followed by `tokenLow,tokenHigh` | `3,status` |
| Maintain audition | `4,tokenLow,tokenHigh,renew` | `4,status` |
| Scale names | `5` | `5,0,count,(id,nameNUL)*` |

Capability flags 7 mean native undo, non-destructive audition and rhythm shaping. Lease is 1500 ms (LSB-first seven-bit encoding). Names are firmware ROM ASCII, at most 20 characters, with consecutive zero-based IDs. Starter IDs and modes match the native 24-entry bank: CHORD=0, BASS=1, ARP NOTES=2. ARP NOTES generates ordinary notes; it does not configure the live arpeggiator.

`track` is 0..2 and must equal actual firmware selection. `key` is 0..11; scale must be in the discovered scale bank. Octave -3..3 is encoded +3; rotate -16..16 is encoded +16; offset -8..8 is encoded +8; feel -32..31 is encoded +32. VLEAD and confirm are 0/1; sync is 0..2. Rhythm shaping applies to all four bars. Schema 1 has no independent rhythm selector: preview and apply explicitly use ORIGINAL, including its ARP NOTES pulse. Requests preserve the native browser's PRESET/RHY choice rather than inheriting or overwriting it. Rhythm-bank discovery/selection in Android remains future work. VLEAD uses the native deterministic voicing, with no live chord history changes. BASS ignores VLEAD.

Status: 0 success, 1 malformed/options invalid, 2 busy/live ownership, 3 replacement confirmation required, 4 stale selection/lease. Errors return only operation and status. Exact payload length and all ranges are checked before mutation. An empty/malformed request reports operation 127 when absent. Discovery errors use the same two-byte form.

Apply requires stopped transport, no pending transport, recording/free take, panic, physical or MIDI-held notes/latches, and no native starter browser. Existing content includes notes, micro timing, fill and locks; confirm=1 explicitly permits replacing it. Successful apply invokes the actual native generator/undo path and replaces only steps, timing, fill/locks and sequence-active state; sound and global tempo are unchanged. Android always asks confirmation, freezes the chosen options, and invalidates its native pattern baseline after success. Hardware EDIT + OCT− restores the complete replaced material. Unknown outcomes require reading hardware before retrying.

Audition uses the current selected track sound and global tempo, without starting transport or changing stored patterns. Nonzero 14-bit tokens identify a single audition owner. A different owner cannot replace an active audition. renew=1 extends the lease; renew=0 stops the matching owner. Unknown tokens return stale and cannot stop another owner. Initial busy checks reuse existing MIDI routing ownership and latch lists. There is no extra pattern, bank or MIDI bitmap allocation.

Cleanup happens before new live note-on ownership, on physical/MIDI synth or drum takeover, transport/recording/free take/panic, selection changes, USB disconnect/reset and lease expiry. A stopped/expired lease cannot be revived by renewal. The audio callback performs bounded live-owner checks; the 2048-byte routing table is scanned only at START/APPLY request time. Main-loop service handles disconnect/expiry even without a new request. Preview cleanup releases only its own sounding notes.

The phone renews every 500 ms through a connection epoch/identity guarded lightweight service, without publishing Busy/Changed on heartbeat. Closing the dialog, changing track/workspace or stopping/destroying the Activity cancels renewal. A canceled initial request cannot create a background renewal loop. Normal dialog cancellation allows an already-sent maintenance request to drain under the connection epoch token, avoiding an ambiguous late reply fault; it suppresses future renewal and queues stop. Listen stays disabled until its active start/loop completes; press Stop before changing options and listening again. Device cleanup remains bounded if app cancellation, MIDI transport or USB removal prevents a stop frame.

Implementation: `firmware/src/editor_musical_starters.c`, included by `editor.c`; `Sloop.Protocol/EditorClient.MusicalStarters.cs`; Android `MainActivity.MusicalStarters.cs` and `Services/Fm1Connection.MusicalStarters.cs`. Integration uses existing `sequence_preview_tick/end`, native apply/undo and `midi_sel_on` bookkeeping (fixed channels included and panic clears ownership). Lease state is two uint32_t plus one uint16_t: 10 bytes of fields, approximately 12 bytes with target alignment; final combined target RAM/flash is measured by release integration.

Focused checks: `tests/run_musical_starters.sh` exercises the real firmware/UI/audio harness and `Sloop.MusicalStarters.Tests` exercises capability negotiation, names, truncation, options, exact wire encoding, lease and acknowledgement validation. No phone is required. Audible quality remains the native generator's established behavior; no Android audio renderer or phone bank is introduced.
