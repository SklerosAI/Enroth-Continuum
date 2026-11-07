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

The subsequent two-process native v2 test wrote the sampled real creation
Party to `autosave.ec6` under a validated owner-only disposable system-temp
profile, then selected and staged that exact slot in a fresh null-renderer
process without publishing live owners or changing disk bytes. Writer and
reader each passed **1/1**; host monitors saw zero on-screen game windows and
no focus change (28 and 11 samples). The temporary profile was removed.
Live v2 publication, gameplay reload, original item-record byte parity and
other random creation draws remain open.

A later test-only seed input broadened the no-disk loaded-table check across
nine separate sequential-RNG seeds. All **180/180** starter items admitted:
**155 plain**, **25 standard-bonus**, **zero rejected**. The number of
standard-bonus items varied from zero to four per Party, and two seeds repeated
with identical aggregate counts. Eleven total monitored runs produced no
on-screen window or focus change; they wrote no save slot. This remains a
sample of the editable creation path, not a proof of all random outcomes or
native original-game record parity. See the implementation packet for the
strict environment input and seed list.

See also: [New Game Autosave](MM6-F4-New-Game-Autosave.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Raw Cursor Owner](MM6-F4-Raw-Cursor-Owner.md).
