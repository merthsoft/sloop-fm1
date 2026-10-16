# Upstream project oracle

`upstream271.fun6` was produced by `upstream271_project.c` compiled against
unmodified SLOOP commit `4ec1ac829fd2bdf2c571ea215fb3d62477d01220`, including
its own drum-table generator. It is 3,816 bytes, in upstream's native FUN6 format.
The source sets THUMB/WALK/CHORD arp IDs, overlapping four-note continuations,
nudges, a fill condition, a parameter lock and per-drum tune/decay.

Keep this independent of fork serializers. To regenerate deliberately, place that
upstream revision in a temporary directory, generate its headers, and compile the
producer with its `tests` and `firmware/src` include paths. Pass the destination
filename as its first argument. `tests/upstream_project_test.c` loads/applies/saves
the file byte-for-byte; Android's song-library tests use the same fixture.
