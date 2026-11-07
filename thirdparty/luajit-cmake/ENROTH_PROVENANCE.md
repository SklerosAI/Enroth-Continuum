# Repo-owned LuaJIT CMake recipe snapshot

This source is a snapshot of [zhaozg/luajit-cmake](https://github.com/zhaozg/luajit-cmake) at commit `64462217e05164b46ca44c10ed1d12918eee8fa5`, the exact gitlink previously recorded by Enroth Continuum. On 2026-11-07, all 24 original tracked files were copied from a clean local checkout at that commit and verified against its Git blob IDs without source edits.

The original MIT `LICENSE` is retained in full. This provenance note is Enroth Continuum metadata, not part of the original snapshot. The Mac build includes `LuaJIT.cmake` from `thirdparty/CMakeLists.txt`. Its local adaptation reads `ENROTH_RELVER.txt` from the exact pinned LuaJIT source snapshot to generate `luajit_relver.txt` without requiring a source `.git` directory. Other dependencies remain external until migrated separately.
