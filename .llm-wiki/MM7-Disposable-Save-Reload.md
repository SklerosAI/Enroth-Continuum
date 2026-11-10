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

The previous paragraph records the 2026-11-07 state. The later native-slot
helper and load preflight were integrated on 2026-11-09; see the
[current campaign status](Campaign-Status.md) for the newer acceptance boundary.

## Current staged-candidate gate — 2026-11-10

The integrated-source headless writer and fresh-process reader each passed
1/1 with a disposable owner-only MM7 profile. A staged Atlas then launched
MM7 into a marked temporary profile. In visible play, a newly named party
saved through the menu, quit, relaunched through Atlas and loaded the named
slot. A bought shield and the reduced gold total survived another
fresh-process Load. A copy of an existing installed-baseline save also loaded
in the disposable profile. Captures, save files and checksums stayed outside
Git. The original normal-profile saves were not loaded or written.

The Atlas disposable-profile code was reviewed and integrated after that
visible run. Its final resolver refuses a set-invalid override for Play and
save actions; the current integrated Swift resolver check and optimized Atlas
compile passed. A rebuilt staged app still needs a visible check. One
accidental direct launch during the earlier visible session modified the
normal profile's settings file; prior bytes were unavailable, so it was
preserved. The accidental new log was removed. This gate does not establish
full campaign completion or installed-app cutover.

## Rebuilt-castle checkpoint — 2026-11-10

On integrated source, a disposable-profile writer completed the positive
Castle rebuild interaction and normal Save; a fresh headless child loaded the
slot through the normal menu. Each passed 1/1, the slot digest stayed
unchanged, and the one-missing-rescue and prior disposable writer/reader
controls each passed 1/1. This proves this bounded interaction checkpoint,
not a continuous human campaign or a visible check of the final staged app.

See also: [MM7 Clean Offline Build](MM7-Clean-Offline-Build.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md), [Campaign Status](Campaign-Status.md), [Agent Handoff](Agent-Handoff.md).
