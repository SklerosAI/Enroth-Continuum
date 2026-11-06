---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-New-Game-Autosave]]", "[[MM6-F4-Chest-Map-Load-Handoff]]"]
tags: [mm6, f4, save, chest, transaction]
---

# MM6 F4 Chest EC6 Transaction

The [CHST packet](../project/MM6_F4_CHEST_EC6_TRANSACTION.md) records the seventeenth required `.ec6` section and the remaining first-map movement state.

`CHST` v1 retains the ready-validated MM6 chest LCG, 30 binary generated-artifact flags and the bounded decoded active DLV/DDM sidecar. Old sixteen-section candidates reject. The sidecar name must match `HEAD`; 67-map preflight decodes it against the active base and checks record shape and item IDs alongside the changed MAPS delta. Detached staging returns copies of the title owner and raw sidecar. It does not change process state or commit the live chest vector.

The offline `oute3.odm` autosave builder replaces the sidecar with the decoded trusted default delta while keeping the caller's title-local RNG/ledger seed. New-game seed initialization remains unproved. The active chest adapter must still stage and atomically restore all 20 chests with the sidecar and active map. The live SaveLoad guard stays closed.

The first-map audit identifies `uFallStartZ=340` in `saveNewGame()`, vertical velocity, saved flight Z, flying status, water/lava timer with flags, and running flags as unrepresented `PART` state. Inherited snapshots serialize them, while current movement code reads them; native MM6 meaning and a safe first-frame restore policy still need proof.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [New Game Autosave](MM6-F4-New-Game-Autosave.md), [Chest Map Load Handoff](MM6-F4-Chest-Map-Load-Handoff.md).
