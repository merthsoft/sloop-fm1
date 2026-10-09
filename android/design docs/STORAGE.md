# Sessions, assets and recovery

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Library checkpoint (2026-10-07): named immutable session snapshots, portable ZIP archives,
saved performance presets, name filtering, and grouped workspace/recovery files are implemented.
Live session adoption is integrated and Library Load is enabled. See
[verification record](VERIFICATION.md).

Status: per-workspace persistence, immutable source assets, capture recovery, sound proposal
archives and sample-slot backup/restore are implemented; checkpoint 2026-10-07. Schema migrations,
persisted/background-maintained indexes and asset collection remain future work. Shared-shell transactional adoption is integrated.

Composition drafts use a separate validated phone-local catalog with atomic replacement;
they are not included in portable session archives. See [COMPOSITION.md](COMPOSITION.md).
Retained sample browsing opens owned assets in the active session. The cross-session sample
library separately indexes only manifest-listed WAVs and associated edit/kit/chop-audio sidecars
in named snapshots; saving a session still captures the current referenced WAV, rather than every
retained WAV. No archive boundary is expanded by browsing. Index results are bounded transient
metadata, not a persistent database, and reuse rechecks every source hash.

Reuse stages unchanged original bytes and exact sidecars in the active sample directory, validates
the staged files, then publishes sidecars followed by the WAV and atomically replaces `current.txt`
with a flushed unique temporary file. Cancellation before pointer publication retains the current
sample; cancelled copying cleans its staging directory. A process kill can leave unpublished stage
files or sidecars, which are ignored as library samples and never overwritten on the next copy.
Existing identical full-state copies deduplicate; copies with subsequent user edits remain intact.
Source snapshots are read-only inputs and stale/missing sources fail reuse. Generated FM1 images
are excluded, and successful adoption requires preparing a fresh kit. See [SAMPLING.md](SAMPLING.md)
for browse limits and focused validation coverage. This retains the existing flushed-file/atomic
visibility guarantee; directory fsync and automatic orphan collection remain unimplemented.

## Implemented named sessions

`Sloop.Workstation.SessionStore` stores schema-1 JSON manifests in immutable GUID directories.
A manifest contains the stable session ID, display name, UTC creation time, SHA-256/length/path
for each asset, and scene/preset IDs. Saving creates a new snapshot ID, even for the same name;
there is no implicit overwrite or asset deletion. Scene documents remain owned by the scene
component; callers supply their files through `extraAssets`, alongside scene IDs.

Android `SessionWorkspace` captures workspace documents/proposals, original DX imports,
the referenced current WAV and its edits/kitsettings/FM1 asset, current performance settings,
and all saved performance presets. It reads the saved sample pointer, requires its original,
and never rewrites the source WAV. Optional generated/edit assets are included when present.
Session save and archive construction are bounded and synchronous on the Library action.
Archive import and copying the completed export to the platform destination run off the UI
thread. Moving snapshot saves/archive construction to workers and adding progress/cancellation
remain UI improvements.

Archive import first spools the compressed input to app-private staging with a size limit.
It rejects absolute/traversal/backslash/drive paths, empty segments, unsafe/reserved names,
case-insensitive duplicates, symlinks, extra unreferenced files, bad lengths/hashes, unknown
schemas, more than 512 assets, assets above 256 MiB, aggregate expansion above 512 MiB,
and manifests above 1 MiB. Copies enforce actual streamed lengths. Android validates existing
sound/pattern/native formats, current sample edits and performance options before publishing.
Import assigns a fresh ID and leaves the current pointer and editors unchanged.

Every staged file is flushed before a same-volume directory move. The optional `current`
pointer is written through a unique flushed temporary file and replacement; its previous
value is retained as `current.previous`. Snapshot creation/import and activation are separate.
Failed staging is removed without touching user assets or existing snapshots. Process-killed
staging directories are ignored by listing and retained; automatic cleanup is not implemented.
Directory fsync is not exposed by the managed API, so guarantees are flushed-file durability
and atomic same-volume visibility, not a claim of surviving every filesystem/power failure.

Presets preserve the existing Android `PerformOptions` through an explicit validated string
mapping; no competing performance model is defined. Loading validates every range/enum before
stopping active notes and assigning options. Preset files and current performance settings are
portable assets. Session snapshots intentionally do not persist undo stacks or device handles.

Validation: standalone `Sloop.Sessions.Tests` covers 32 assertions including native application
sound/pattern round trips, original/edit assets, traversal/duplicates, corrupt hashes, extra
entries, unsupported schema, manifest/count limits, rejected staged content, failed activation,
and retention of the previous pointer. These checks do not exercise live editor adoption,
physical low-storage/power-loss conditions, or every maximum-size archive limit.
The earlier standalone packaging failure is resolved in the combined build. APK packaging,
Pixel installation, session load and restart recovery pass; see PARALLEL-INTEGRATION.md.

## Ownership and format

Store workspace documents and immutable source audio in app-private storage. The current
WorkspaceFiles formats are bounded, versioned binary records: SLOOP-SOUND-1 (patch/macros/
revision), SLOOP-PROPOSAL-1 (accepted prompt/provenance/before-after context), SLOOP-PATTERN-1
(IDs, PPQ and full notes), and a versioned complete native pattern. Per-asset kit settings use
SLOOP-KIT-1 text; WAV sources, edit documents and generated .fm1 images are separate assets.
Draft identities use deterministic binary hashing, not JSON reflection. The unified
session manifest is implemented as described above; add a database only if indexing justifies it.
Session and scene IDs are independent of display names. Each save creates a fresh snapshot ID;
assets are identified by portable relative paths within that snapshot. Asset content hashes
detect changes; content deduplication is not implemented.

A portable session export contains its manifest, workspace documents, referenced sample assets,
conversion settings, performance settings/presets and original patch imports. Device snapshots
and scene documents require explicit extra assets from their owners. Absolute paths, Android
content URIs, and platform device handles are not portable references. Schema and creation UTC
are manifest fields; PPQ and audio format remain in their existing document/source formats.
Application-version and structured device metadata remain future manifest extensions.
Show dates in the user's local timezone; use monotonic clocks only for playback scheduling.

## Saves and recovery

Current sound/pattern/native edits save immediately through a candidate document, flushed
temporary file and same-volume replacement before adopting live state. Accepted sound
proposals are archived separately. Undo/redo remains session-local; saved content restores
after process exit. Invalid workspace files are copied aside before defaults are created.
A debounced unified save remains a target. Transactional live-editor generation switching is
implemented: named loads copy into an independent writable generation, prepare all three sound
documents, app/native patterns, sample edits/peaks/conversion options, performance options,
sequence tempo and generic output/edit channel mappings, then atomically replace the durable
workspace pointer before adopting editor references. A failed preparation leaves live editors
and the pointer unchanged. Startup resolves this pointer; a malformed/missing current generation
recovers the previous pointer. The earlier live generation remains on disk, including unsaved-to-
Library edits. Immutable Library snapshots are never used as writable editor directories.
Named snapshots already retain earlier saves; activation retains the previous pointer.
Write a new document, flush, and replace using the platform's safe same-volume mechanism;
retain a last-known-good generation. Do not delete assets until saved references no longer
need them and a retention policy permits collection. Save failures preserve dirty state.

Recordings use chunk/journal recovery from AUDIO.md. Startup automatically adopts recoverable
complete frames as a labelled interrupted WAV; a finalize/discard list remains planned.
Capture stops/finalizes when leaving the foreground. Low storage/backpressure stops capture
and preserves complete written frames. Transfers retain their backups after interruption.
Current slot-backup filenames identify the destination slot and operation; before/replacement
images and a pending/verified text journal are retained. Persistent device identity, structured
phase tracking and full restore journals remain improvements. No record implies resumable firmware transfers.

## Backup and device state

FM1 backup is a separate artifact using the existing validated backup object format. Record
firmware/protocol/project format and object fingerprints. Do not interpret binary firmware
objects through unsafe C# struct casts. Backup read consistency needs stopped/stable state or
revision checks; a mixed-revision backup must be rejected or explicitly labelled incomplete.
Restore validates compatibility before mutations, shows affected resources, requires safe
transport state and reports each committed object. Partial restore remains visible/recoverable.

## Import/export and portability

Use platform-selected streams. Validate archive entry paths, sizes and counts before extraction;
reject traversal and uncontrolled expansion. Import into staging, validate references/hashes,
then publish the immutable snapshot. Name collisions always create new IDs in this implementation.
Publishing an imported snapshot does not load it into live editors.
Keep original audio separate from normalized/constrained FM1 artifacts. WAV/slice/kit export
can work without FM1 connection. Windows/iOS reuse the document schema, not Android storage APIs.

## Migrations and acceptance

Schema 1 is supported; other schema versions are rejected without changing current work or
the source archive. Forward migrations and read-only viewing of newer schemas are planned.
Remaining acceptance includes interrupted process/power-loss saves, physical low storage,
maximum-size files, physical live editor adoption, migrations, backup incompatibility and partial device
restore. No cloud/account is required.


