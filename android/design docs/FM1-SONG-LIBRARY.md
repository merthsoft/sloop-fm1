# FM1 song library

Implemented in Library → FM1 songs. This catalog is separate from phone-local sessions
and composition drafts. The FM1 keeps its four native A–D slots; the phone holds named
native snapshots without requiring additional firmware RAM or storage.

<img src="../../assets/screens/merthsoft/android-song-library.png" width="280" alt="Fresh Pixel capture of the FM1 song library, offline save guard and import button">

## Save and load

- **Save from FM1** captures the working project, all four A–D sections and their
  arrangement, user preset banks, FM6 bank, SYN drum kits, all four user sample slots,
  and the current edited FM6 voices on synth tracks. All shared sound banks are included
  deliberately, so references in any saved section remain valid.
- **Load onto FM1** confirms replacement of these shared resources. The previous
  complete state is saved as a named recovery song before the first hardware write.
  Sound dependencies are restored first, then sections, arrangement and working project,
  followed by edited track voices. Unchanged objects are skipped. Final objects and live
  patches are read back; success requires matching content.
- **Export FM1 song / Import FM1 song** exchange bounded `.fm1song` JSON archives.
  Import gives the archive a new catalog identity and does not change hardware.
- Device preferences and MIDI configuration are retained. The saved settings snapshot is
  used only for its 36-byte arrangement; those bytes are merged into the current settings.
  Optional MIDI object 10 is captured for evidence but not restored by song loading.

Physical FM1, released notes and stopped playback/recording are required. The initial
implementation requires protocol 14 or later with the supported 71/63 SLOOP 2.6 parameter
layout, FUN6 projects and identical engine ordering. It does not translate between layouts.
After loading, the device connection is refreshed; phone-local sound/pattern drafts remain
separate and must be explicitly read from the FM1 when wanted in their editors.

## Safety and persistence

`Fm1Songs` uses existing BK_LIST/GET/PUT and sample-slot commands. Inventory sizes,
chunk offsets, decoded lengths, object CRCs, native project magic/size/FNV checksum,
arrangement and sample metadata are checked. Native objects use the firmware's A/B
commit. Sample replacement retains its existing upload/readback path.

The phone catalog uses flushed temporary files and atomic publication through
`WorkspaceFiles.StoreBytes`. A failure to retain the recovery archive prevents all device
writes. Firmware restore is atomic per object, **not across an entire song**: an interrupted
load can leave a mixture of old/new banks and sections. The app reports failure, retains
the recovery song and requires reconnection before retrying. It does not attempt automatic
rollback over an uncertain connection. Loading the recovery song restores its musical state
while retaining the device's current preferences/MIDI configuration.

Catalog size is limited by phone storage. Archives are limited to 600,000 bytes, with unique
known object IDs and bounded object sizes. All four sample banks are included, so a song can
be several hundred kilobytes. A corrupt file is reported; nothing is silently discarded.
Browse, export and import work offline. No song deletion UI or native slot-count expansion
is included in this checkpoint.

## Verification

`Sloop.Fm1Songs.Tests` exercises full capture, archive round trip, edited FM6 voice capture
and restore, corruption and incompatible-layout rejection, failed disk backup before writes,
dependency order, arrangement-only settings merge, preserved MIDI state, durable catalog
reopening and failed readback. Actual hardware save/load remains a device validation task.
