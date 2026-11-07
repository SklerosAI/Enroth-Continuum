---
type: synthesis
up: "[[MM6-F4-New-Game-Autosave]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-Raw-Cursor-Owner]]"]
tags: [mm6, f4, new-game, inventory]
---

# MM6 Creation Item Seed

This page records the bounded adapter for fresh MM6 starter items and its headless loaded-table check.

The [source-backed implementation packet](../project/MM6_F4_CREATION_ITEM_SEED.md)
records the detached adapter added after the native v2 first-map candidate.
`seedMM6CreationItems()` converts the post-creation Party's plain and bounded
bonus backpack items into canonical 28-byte MM6 records without changing live
owners. Its result carries the Party copy with raw item provenance, physical
`INVR`, and an empty title-local `HELD` cursor for the offline v2 writer.

Three linked synthetic tests passed, including full v2 candidate composition.
This removes the missing raw-carrier
obstacle for supported starter draws only. It rejects unsupported specials,
potions, artifacts, equipment placement and any seeded Item whose decoded
logical fields differ. Native original-game byte parity, further loaded-table
creation outcomes, all-owner first-map commit and fresh-process live reload
remain open. Do not open the normal MM6 New Game or SaveLoad guards based on
this seed alone.

The later isolated loaded-table Create Party GameTest passed twice with
sequential RNG: the complete Party and all **20/20** starter items passed the
adapter (**16 plain, 4 standard-bonus, zero rejected**). The null-platform
test wrote no save slot and host monitors observed no on-screen game window or
focus change. This samples one deterministic creation draw; other possible
ring outcomes and original native record parity remain unproved. See the
implementation packet for the test seam and exact limits.

See also: [New Game Autosave](MM6-F4-New-Game-Autosave.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Raw Cursor Owner](MM6-F4-Raw-Cursor-Owner.md).
