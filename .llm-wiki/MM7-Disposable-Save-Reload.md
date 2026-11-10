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
full campaign completion or installed-app cutover. The owner later authorized
a headless Atlas reinstall. The signed candidate is installed with the old
app retained locally, but no post-install launch or graphical Save/Load check
has run.

## Rebuilt-castle checkpoint — 2026-11-10

On integrated source, a disposable-profile writer completed the positive
Castle rebuild interaction and normal Save; a fresh headless child loaded the
slot through the normal menu. Each passed 1/1, the slot digest stayed
unchanged, and the one-missing-rescue and prior disposable writer/reader
controls each passed 1/1. This proves this bounded interaction checkpoint,
not a continuous human campaign or a visible check of the final staged app.

## Light Walls checkpoint — 2026-11-10

On integrated source, a marked disposable-profile writer followed the
input-driven Light choice, city, Walls pedestal and throne hand-in route and
wrote a normal MM7 slot. A separate fresh headless child selected that slot
through the normal Load menu. Writer and reader passed 1/1 each; persistent
Light, Walls and guild bits, map and selected slot matched, and the slot digest
was unchanged. The refactored Light route and dawn closed-throne controls also
passed 2/2. A transient NPC dialogue field was not used as a persistence
assertion because the branch gate observed it reset after Load. This remains
a bounded chapter checkpoint, not an end-to-end visible campaign.

## Overthruster retrieval checkpoint — 2026-11-10

On integrated source, a marked owner-only disposable-profile writer used the
input-driven Lincoln console and pedestal, retrieved the Overthruster into the
held-item slot, passed the four-wetsuit Shoals exit and wrote a normal numbered
MM7 save. A separate fresh headless process selected that slot through the
normal Load menu. Writer and reader passed 1/1 each. The reader retained
Shoals location, Light and retrieval bits, four equipped wetsuits and the held
Overthruster, while the slot digest remained unchanged. A focused integrated
run of the refactored retrieval case, Xenofex/finale link and two wetsuit/
Shoals controls passed 6/6. The writer fixtures earlier Light chapter state,
so this proves a later chapter checkpoint rather than uninterrupted campaign
continuity. It does not exercise post-install Atlas launch or visible play.

See also: [MM7 Clean Offline Build](MM7-Clean-Offline-Build.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md), [MM7 Playtest Recording](MM7-Playtest-Recording.md), [Campaign Status](Campaign-Status.md), [Agent Handoff](Agent-Handoff.md).
