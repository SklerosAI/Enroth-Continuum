---
type: synthesis
up: "[[MM7-Self-Contained-Source]]"
related: ["[[MM7-Mac-Dependency-Recipe]]", "[[MM7-Runtime-Path-Cutover]]"]
tags: [mm7, build, mac, offline]
---

# MM7 Clean Offline Build

A fresh extraction of tracked Enroth Continuum source built an arm64 MM7 helper
and staged Atlas under a network-denied macOS sandbox on 2026-11-07. The
repository now provides one command for repeating that route.

The [build packet](../project/MM7_CLEAN_OFFLINE_BUILD.md) records the route and
limits. The five tracked media sources produced nine static arm64 archives;
CMake verified their hashes, source pins, FFmpeg flags and 23 notices. The
local-only lane used tracked libdwarf and an opt-in Mac iconv adapter in place
of the unresolved ztd.text fetch path. The clean MM7 target linked against the
nine local archives, the staged Atlas and helper passed code-signature checks,
and no private game files were bundled. A bounded headless startup found the
private MM7 data root; a freshly built MM7 headless GameTest control passed
1/1 and the Russian-text suite passed 13/13. All build, package and control steps ran with network access denied.

The tracked [`tools/build_mm7_macos.py`](../tools/build_mm7_macos.py) command
builds the pinned media libraries, configures the local-only MM7 target,
checks all nine link inputs and system-only dynamic links, and stages Atlas in
a new output directory. A second fresh `git archive HEAD` extraction ran this
one-command route end to end with networking denied. The arm64 helper and
staged Atlas passed strict code-signature verification; the helper has valid
`0.1.0` bundle versions and the archive build records an honest unknown Git
revision without a configure error. No private data or saves entered the bundle.

This is a clean **tracked-source extraction**, not a remote Git clone. It did
not test visible gameplay, audio, save/quit/reload, install or a complete MM7
campaign. The ordinary CMake lane still references an OpenEnroth release and
the remaining ztd.text gitlink; the dedicated Mac MM7 wrapper selects the
proven local-only lane. It does not change the shared MM6/MM8 or non-Mac build
defaults.
The iconv adapter is scoped to MM7's production encodings and is not full
legacy/CJK API parity. Preserve historical OpenEnroth credit and source
provenance when changing the build default.

A later [hidden OpenGL diagnostic](MM7-Offscreen-Frame-Capture.md) rendered one
first-map frame using a GameTest rebuilt in that temporary extraction after
copying newer test/startup files. The extraction was marked modified for that
diagnostic; the earlier clean build and staged Atlas evidence above are separate.

The later [disposable save/reload gate](MM7-Disposable-Save-Reload.md) used a
locally linked GameTest in two fresh headless processes. It did not exercise
the staged Atlas helper.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [MM7 Mac Dependency Recipe](MM7-Mac-Dependency-Recipe.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [Agent Handoff](Agent-Handoff.md).
