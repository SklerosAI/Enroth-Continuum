---
type: synthesis
up: "[[MM7-Self-Contained-Source]]"
related: ["[[MM7-Mac-Dependency-Recipe]]", "[[MM7-Runtime-Path-Cutover]]"]
tags: [mm7, build, mac, offline]
---

# MM7 Clean Offline Build

A fresh extraction of tracked Enroth Continuum source built an arm64 MM7 helper
and staged Atlas under a network-denied macOS sandbox on 2026-11-07.

The [build packet](../project/MM7_CLEAN_OFFLINE_BUILD.md) records the route and
limits. The five tracked media sources produced nine static arm64 archives;
CMake verified their hashes, source pins, FFmpeg flags and 23 notices. The
local-only lane used tracked libdwarf and an opt-in Mac iconv adapter in place
of the unresolved ztd.text fetch path. The clean MM7 target linked against the
nine local archives, the staged Atlas and helper passed code-signature checks,
and no private game files were bundled. A bounded headless startup found the
private MM7 data root; a freshly built MM7 headless GameTest control passed
1/1. All build, package and control steps ran with network access denied.

This is a clean **tracked-source extraction**, not a remote Git clone. It did
not test visible gameplay, audio, save/quit/reload, install or a complete MM7
campaign. The ordinary CMake lane still references an OpenEnroth release and
the remaining ztd.text gitlink; only the opt-in Mac local-only lane is proven.
The iconv adapter is scoped to MM7's production encodings and is not full
legacy/CJK API parity. Preserve historical OpenEnroth credit and source
provenance when changing the build default.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [MM7 Mac Dependency Recipe](MM7-Mac-Dependency-Recipe.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [Agent Handoff](Agent-Handoff.md).
