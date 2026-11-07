---
type: synthesis
up: "[[MM7-Self-Contained-Source]]"
related: ["[[MM7-Clean-Offline-Build]]", "[[MM7-Offscreen-Frame-Capture]]"]
tags: [mm7, save, mac, headless]
---

# MM7 Disposable Save and Reload

A local-only MM7 GameTest saved to an isolated temporary profile and reloaded
the file through the normal main-menu Load path in a separate headless process
on 2026-11-07.

The [gate packet](../project/MM7_DISPOSABLE_SAVE_RELOAD_GATE.md) describes the
strict profile and exact-filter checks. The writer and reader passed 1/1 each;
the loaded map and test-only Party value matched, and the save file was
byte-identical after reload. A wrong filter and unexpected profile contents
were rejected before startup. The temporary profiles were removed. Ordinary
MM7 headless controls still passed 2/2.

This is a null-renderer GameTest using locally built dependencies, not a
visible run of the staged Atlas helper or an installed-app save check. It
covers one first-map fixture, not an existing personal save or the campaign.
The current integration source repeated the gate with its own source-linked
GameTest under network denial on 2026-11-07: writer and reader passed 1/1 each,
the disposable slot was byte-identical after reload, and focused MM7 controls
passed 3/3. The temporary profile was removed. This newer result does not
change the graphical or campaign limits.
The [hidden OpenGL frame gate](MM7-Offscreen-Frame-Capture.md) separately
proved rendering without disk save/reload. The [runtime cutover](MM7-Runtime-Path-Cutover.md)
still requires collision-safe review and visible candidate play before any
installed-app change. No proprietary data, saves or hashes entered Git.

An [isolated native-slot helper](MM7-Atomic-Save-File.md) now passes synthetic
fault tests. This disposable cycle still exercised the inherited SaveLoad
writer; the helper is not yet wired into normal MM7 saves or loads.

See also: [MM7 Clean Offline Build](MM7-Clean-Offline-Build.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md), [Agent Handoff](Agent-Handoff.md).
