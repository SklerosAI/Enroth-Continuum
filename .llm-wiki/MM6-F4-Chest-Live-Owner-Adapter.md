---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Handoff]]"
tags: [mm6, f4, chest, runtime, save]
---

# MM6 F4 Chest Live Owner Adapter

The [public packet](../project/MM6_F4_CHEST_LIVE_OWNER_ADAPTER.md) records an opt-in adapter from the detached twenty-record handoff to actual `vChests` and the active indoor/outdoor decoded delta sidecar. It is **not called from the guarded load path**.

`ChestTitleState` is MM6-only: `ready`, a 32-bit original chest LCG state and thirty generated flags for artifact IDs 400–429. Its process accessor starts unready. `restoreChestTitleState` accepts only MM6 and binary flags; `resetChestTitleState` clears it. This keeps chest rolls and artifact uniqueness separate from Mersenne Twister `grng` and the MM7 artifact flag band. The `.ec6` save agent needs this exact typed interface and must initialize/reset/restore it atomically with the active map sidecar and chest vector. The original new-game seed and party-owned flags are still unproved.

The new opt-in seed and ownership helper permits only a caller-supplied deterministic new-game seed; retained traces prove the MM6 LCG step but not its original initial source. The helper rejects unverified source, MM7 and missing owners without mutation, and resets generated flags when starting a new game. Its bounded party scan marks IDs 400–429 in all four character inventories, equipped records and the held cursor item, ignoring ID 430. This is the selected safe personal-fork policy, deliberately distinct from the original overlapping raw-memory scan. Neither helper is wired into new-game or map entry. The adapter still takes owned flags explicitly.

The reusable binding parses a decoded DLV/DDM sidecar with exactly twenty records and checks the pre-generation runtime chest vector against the raw headers, first 24 item bytes and all 140 grid cells. It then stages generation, twenty `Item`/`ChestInventory` projections and a copied sidecar with only chest records replaced. The final publish swaps `vChests` and sidecar and copies LCG/flags after all checks succeed. Late projection, stale runtime, unready/foreign title, corrupt wire and invalid flags leave every bound owner unchanged. Synthetic indoor/outdoor tests pass; an optional private-backed test staged **52 indoor plus 15 outdoor** retained decoded map sets using a synthetic table and one-cell footprints. This checks structure and owner matching, not exact loot or real item sizing.

The global `generateChestItemsForTitle` guard stays closed until the process owner is initialized from a selected explicit MM6 seed policy or a proved original source, party-owned flags are supplied to map entry, `.ec6` captures/restores state with the active map sidecar, and first-open chest interactions survive save–quit–relaunch–load. Title-specific icons, placement timing, traps, gold, pickup and graphical UI remain separate blockers.

See also: [MM6 F4 Chest Map Load Handoff](MM6-F4-Chest-Map-Load-Handoff.md), [MM6 F4 Chest Runtime Projection](MM6-F4-Chest-Runtime-Projection.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
