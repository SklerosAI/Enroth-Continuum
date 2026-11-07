# MM6 creation item seed handoff

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
logical fields differ. Native original-game byte parity, actual loaded-table
creation outcomes, all-owner first-map commit and fresh-process live reload
remain open. Do not open the normal MM6 New Game or SaveLoad guards based on
this seed alone.
