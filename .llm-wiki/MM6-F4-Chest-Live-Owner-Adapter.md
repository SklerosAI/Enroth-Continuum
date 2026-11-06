---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Handoff]]"
tags: [mm6, f4, chest, runtime, save]
---

# MM6 F4 Chest Live Owner Adapter

The [public packet](../project/MM6_F4_CHEST_LIVE_OWNER_ADAPTER.md) records an adapter from the detached twenty-record handoff to actual `vChests` and the active indoor/outdoor decoded delta sidecar. A scoped [map-load route](MM6-F4-Chest-Map-Load-Route.md) can now call it; the default MM6 guard remains closed and no production caller enables the scope.

`ChestTitleState` is MM6-only: `ready`, a 32-bit original chest LCG state and thirty generated flags for artifact IDs 400–429. Its process accessor starts unready. `restoreChestTitleState` accepts only MM6 and binary flags; `resetChestTitleState` clears it. This keeps chest rolls and artifact uniqueness separate from Mersenne Twister `grng` and the MM7 artifact flag band. The `.ec6` save agent needs this exact typed interface and must initialize/reset/restore it atomically with the active map sidecar and chest vector. The original new-game seed and party-owned flags are still unproved.

The opt-in seed and ownership helper now has an explicit personal-port new-game policy. It draws a 32-bit value once from local `std::random_device` for an unready MM6 owner, clears generated flags, and expects `.ec6` CHST to persist the evolving LCG state. An injected entropy callback makes tests deterministic; foreign title, already-ready owner, no seed and provider exception leave the owner unchanged. The original initial source remains unproved. The bounded party scan marks IDs 400–429 in all four character inventories, equipped records and the held cursor item, ignoring ID 430. This is the selected safe personal-fork policy, deliberately distinct from the original overlapping raw-memory scan. Neither helper is wired into new-game or map entry. The adapter still takes owned flags explicitly.

The reusable binding parses a decoded DLV/DDM sidecar with exactly twenty records and checks the pre-generation runtime chest vector against the raw headers, first 24 item bytes and all 140 grid cells. It then stages generation, twenty `Item`/`ChestInventory` projections and a copied sidecar with only chest records replaced. The final publish swaps `vChests` and sidecar and copies LCG/flags after all checks succeed. Late projection, stale runtime, unready/foreign title, corrupt wire and invalid flags leave every bound owner unchanged. Synthetic indoor/outdoor tests pass; an optional private-backed test staged **52 indoor plus 15 outdoor** retained decoded map sets using a synthetic table and one-cell footprints. This checks structure and owner matching, not exact loot or real item sizing.

The global `generateChestItemsForTitle` guard stays closed by default until CHST/new-game state and decoded first-map sidecar are staged before the transition, the scoped route passes an integrated headless first-map/rollback test, and first-open chest interactions survive save–quit–relaunch–load. The port seed policy and party-owned scan are staged but not live-normal. Title-specific icons, placement timing, traps, gold, pickup and graphical UI remain separate blockers.

See also: [MM6 F4 Chest Map Load Handoff](MM6-F4-Chest-Map-Load-Handoff.md), [MM6 F4 Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
