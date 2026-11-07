# Repo-owned LuaJIT source snapshot

This is the exact tracked source tree of [LuaJIT/LuaJIT](https://github.com/LuaJIT/LuaJIT) at commit `97813fb924edf822455f91a5fbbdfdb349e5984f`, the gitlink previously recorded by Enroth Continuum. On 2026-11-07, all 232 original tracked blobs were copied from that commit and verified against their Git blob IDs without source edits.

The original `COPYRIGHT` is retained in full. It includes the LuaJIT and Lua 5.1/5.2 MIT grants and the dlmalloc public-domain statement. This provenance note and `ENROTH_RELVER.txt` are Enroth Continuum metadata, not part of the original snapshot.

`ENROTH_RELVER.txt` records that commit's Unix committer timestamp, `1727870382` (2024-10-02), which the prior LuaJIT CMake recipe obtained with `git show -s --format=%ct`. The local recipe now uses this recorded value, so an offline source checkout generates the same `luajit.h` without an embedded Git repository.
