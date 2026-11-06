# MM6 chest map-load handoff — detached atomic owner bundle

Status: **staged and native-tested, not called by the guarded map loader** (2026-11-06). The load call remains `Engine::_461103_load_level_sub()` → `mm6::generateChestItemsForTitle(gameTitle())`; MM6 still throws before inherited chest/RNG/artifact mutation. MM7 still takes its existing generator route.

## Twenty-record transaction

`mm6::ChestMapLoadOwners` groups the future MM6 title-local owners: twenty raw 4,204-byte chest records, projected `Chest` values, the original MM6 32-bit chest LCG state and thirty generated-artifact flags for IDs 400–429. `stageChestMapLoadHandoff` requires the same `ItemTable` used by the live item-size lookup, then composes the [record generation transaction](MM6_F4_CHEST_RECORD_TRANSACTION.md) with twenty [bounded `Item`/`ChestInventory` projections](MM6_F4_CHEST_RUNTIME_PROJECTION.md). Every result is local until all twenty projections succeed. A malformed last chest rejects the candidate after earlier chests were staged, leaving all caller owners unchanged.

`applyChestMapLoadHandoff` performs that full stage first, then swaps the raw record set and chest vector and copies the LCG/artifact state. The swaps are compile-time checked `noexcept`; scalar copies cannot fail. No mutation can occur after an ordinary stage failure. A late projection failure after a safe tier-seven artifact selection leaves the source marker, existing chests, RNG and all generated flags intact. Foreign MM7 titles, invalid item IDs, nonbinary artifact flags (even on marker-free maps) and table-pointer mismatches reject. The direct artifact policy remains IDs 400–429, ownership/generated uniqueness and a cap of thirteen; it never reproduces the original out-of-bounds slot-index write.

The current global `grng` interface is backed by Mersenne Twister and exposes no serializable MM6 LCG state. The inherited `Party::isArtifactGenerated` is indexed by MM7 artifact IDs rather than MM6's 400–429 band. A later [opt-in live-owner adapter](MM6_F4_CHEST_LIVE_OWNER_ADAPTER.md) now has an explicit MM6 state owner and can stage actual `vChests` and the decoded active-map sidecar together. It is not called from the global load path, and `.ec6` does not yet persist that owner. The guard stays closed until initial seed, party-owned flags, save ownership and first-open behavior are proved.

## Evidence

Focused arm64 native tests cover a successful twenty-chest commit, safe direct-artifact flag publication, failure at chest 20 after partial projection, failure after an earlier artifact roll, malformed/foreign-title rejection and all-owner rollback. An optional retained-data test reads only the ignored decoded F3-backed map baselines; it staged all **52 indoor and 15 outdoor** map chest sets using a deliberately synthetic regular-item table and one-cell item footprints. This establishes record structure and bounded projection across the retained baselines, **not** exact original loot outcomes or real item footprints. The original GOG installation was not opened and no private records were committed.

## Remaining first-open and save gates

- `Chest::open` still calls inherited `Chest::PlaceItems`; the detached MM6 first-open 9×9 placement transaction is not joined to its UI and trap path. Icon-size lookup, gold and cursor pickup behavior need title-specific verification.
- The save writers now support synthetic changed 140-cell grids, but the real active map's raw sidecar, RNG/artifact state, map reset and `.ec6` save–quit–relaunch–load are not a single transaction. Existing `SaveLoad` guards remain.
- A headless MM6 scenario and an owner-visible graphical/audio check are needed after the complete offline engine gate. No MM6 chest has been opened in-game.

See the [generation audit](MM6_F4_CHEST_GENERATION_AUDIT.md), [runtime projection](MM6_F4_CHEST_RUNTIME_PROJECTION.md), [interaction gate](MM6_F4_CHEST_INTERACTION_GATE.md) and [save transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).
