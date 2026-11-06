---
type: synthesis
up: "[[MM6-F4-Chest-Generation]]"
tags: [mm6, f4, chest, runtime, save]
---

# MM6 F4 Chest Runtime Projection

The [public packet](../project/MM6_F4_CHEST_RUNTIME_PROJECTION.md) records the bounded bridge from one resolved MM6 chest record to inherited `ChestInventory`/`Item` and the extended indoor/outdoor changed-grid writers.

`stageChestRuntimeProjection` is opt-in and MM6-only. It preflights title, chest type, IDs, item sizes, active 9×9 grid references and zero values in the remaining 59 cells. It reconstructs a local `Chest`, snapshots it, and accepts only an exact supported-field/grid round trip. Negative generation markers reject; the global map loader does not call this stage. `stageChestRuntimeRecord` overlays a live staged `Chest` onto its raw sidecar: flags, first 24 item bytes and all 140 signed grid cells change; item bytes 24–27 remain from MM6 wire because the inherited MM7 `Item` mapping is not proved equivalent. A synthetic stashed-item → first-grid-placement → reopen test passes with an opaque item byte preserved.

The indoor `.dlv` and outdoor `.ddm` encoders now stage changed grid cells, including cell 139, and complete 28-byte direct DTO item records. Active positive references are bounded to restorable 9×9 item slots, negative cells point to prior positive anchors, and any nonzero grid requires the placed flag. Live `Chest` capture keeps the original four-byte MM6 item tail. Synthetic indoor/outdoor tests pass for changed cells and malformed references. The private retained-file tests were not run for this slice.

The live `generateChestItemsForTitle` guard remains closed. A later [detached map-load handoff](MM6-F4-Chest-Map-Load-Handoff.md) stages all twenty records, and an [opt-in live-owner adapter](MM6-F4-Chest-Live-Owner-Adapter.md) can stage actual `vChests` and the active decoded map sidecar with an MM6-only title state. It is not invoked from engine load or persisted by `.ec6`. The inherited `Chest::open`/`PlaceItems`/pickup/GUI path and save–quit–load remain unproved; no MM6 process or graphics test was run. Keep MM7 and MM8 routes isolated.

See also: [MM6 F4 Chest Generation](MM6-F4-Chest-Generation.md), [MM6 F4 Chest Grid Wire](MM6-F4-Chest-Grid-Wire.md), [MM6 F4 Chest Map Load Handoff](MM6-F4-Chest-Map-Load-Handoff.md), [MM6 F4 Chest Live Owner Adapter](MM6-F4-Chest-Live-Owner-Adapter.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [MM6 F4 Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
