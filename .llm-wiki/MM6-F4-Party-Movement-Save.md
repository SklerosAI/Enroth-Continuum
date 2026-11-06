---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-New-Game-Autosave]]"]
tags: [mm6, f4, save, party, movement]
---

# MM6 F4 Party Movement Save

`PART` v2 introduced the current Party movement and hazard state needed for a faithful first frame after `.ec6` restore; v3 retains those fields and adds justice counters. See the [core field packet](../project/MM6_EC6_CORE_FIELDS_F4.md).

The former 74-byte v2 payload extended the original 42-byte fields with vertical velocity (float32), saved flight Z (int32), water/lava timer ticks (int64), fall-start Z (int32), flying (uint32, 0 or 1), and two raw 32-bit Party flag words. Position and vertical speed must be finite and bounded; flight and fall Z are bounded. Unknown flag bits round-trip without assigning native MM6 semantics. The current v3 payload is 90 bytes; v1 and v2 reject. There is no live `.ec6` save to migrate.

The MM6-only capture adapter reads these fields from `Party`. The stage adapter applies them to a detached Party, sets horizontal velocity to zero because current input derives X/Y, and resets `floor_face_id=-1` and `currentWalkingSound=SOUND_Invalid`. `DoPrepareWorld()` already resets floor face before map loading because face IDs are map-local. First-frame pressure-plate and audio behavior still needs a headless or rendered reload check.

`saveNewGame()` assigns position Z and fall-start Z to 340 for `oute3.odm`; the offline autosave builder now requires both values before reading the default map archive. This does not prove new-game chest seed, commerce, beacons, active-world reconstruction, or all-owner commit. Live SaveLoad remains guarded.

The source and focused unit cases syntax compile with the integrated arm64 flags. A standalone native codec probe passed nonzero v2 round-trip, exact 74-byte length, superseded v1 rejection and late truncation without destination mutation. The runtime adapter and new-game tests require a clean integrated rebuild because the prior static objects carry the smaller `CorePartyFields` layout.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [New Game Autosave](MM6-F4-New-Game-Autosave.md).
