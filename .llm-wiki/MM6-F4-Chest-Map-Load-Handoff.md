---
type: synthesis
up: "[[MM6-F4-Chest-Generation]]"
tags: [mm6, f4, chest, map-load, transaction]
---

# MM6 F4 Chest Map Load Handoff

The [public packet](../project/MM6_F4_CHEST_MAP_LOAD_HANDOFF.md) describes a detached all-twenty-chest owner bundle that composes generated 28-byte records, `Item`/`ChestInventory` projections, MM6 LCG state and 400–429 artifact flags.

`stageChestMapLoadHandoff` validates the MM6 title, binary artifact flags and shared item table, runs the detached generator, then projects all twenty records locally. `applyChestMapLoadHandoff` publishes the entire candidate only after it succeeds, using compile-time `noexcept` swaps and scalar copies. A chest-20 projection failure after earlier artifact selection leaves every source record, old chest, RNG state and generated flag unchanged. It rejects MM7, malformed IDs, nonbinary flags even on marker-free maps, and mismatched item-table ownership. The safe direct-artifact policy still excludes ID 430 and the original out-of-bounds write.

An optional native test read only ignored decoded retained map baselines and staged **52 indoor plus 15 outdoor** chest sets with a synthetic regular-item table and one-cell footprints. This verifies record topology and bounded projection; it does not verify exact original loot or real item sizes. No private records entered Git and the original GOG install was not opened.

The current `Engine::_461103_load_level_sub` still calls the guarded MM6 entry. `grng` is Mersenne Twister without MM6 LCG state access, and `Party::isArtifactGenerated` is indexed by MM7 artifact IDs. A later [opt-in live-owner adapter](MM6-F4-Chest-Live-Owner-Adapter.md) now stages actual `vChests` and the decoded active-map sidecar with a separate MM6 title state, but is not wired into the guarded load call or `.ec6`. The global guard stays closed. The [first-open interaction gate](MM6-F4-Chest-Interaction-Gate.md) still needs MM6 placement timing, icon lookup, trap, pickup, gold and UI validation, followed by save–quit–relaunch–load and visible play.

See also: [MM6 F4 Chest Generation](MM6-F4-Chest-Generation.md), [MM6 F4 Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [MM6 F4 Chest Live Owner Adapter](MM6-F4-Chest-Live-Owner-Adapter.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
