---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[MM6-F4-NPC-State]]", "[[Source-MM6-Save-Field-Register-F2]]"]
tags: [mm6, f4, save]
---

# MM6 F4 Known Save Assembly

The [assembly packet](../project/MM6_EC6_KNOWN_ASSEMBLY_F4.md) originally defined a partial sixteen-section candidate. The provisional schema now requires a seventeenth `CHST` section for MM6 chest title state and the active raw delta sidecar; old sixteen-section candidates reject before mutation. See the [CHST transaction packet](MM6-F4-Chest-EC6-Transaction.md). It is a pure encode/decode and copy-assignment boundary, not the final whole-save manifest or a live MM6 save path.

Every `PART`, `CHAR`, `QEST`, `EFFT`, `ITEM`, `HEAD`, `TIME`, `UIST`, `VISU`, `MAPS`, `RESC`, `RSTS`, `HELD`, `BUFF`, `HIRE` and `RCVR` section must be present once and required. Optional extras are rejected pending an explicit preservation policy. The decoder validates all field families and cross-checks the party clock across `PART`, `HEAD` and `RCVR`, plus active-character selection, before assigning any output. The independent `TIME` event clock is not required to equal party playing time.

`CHAR` v4 preserves seven signed stat bonuses, AC/level/age modifiers, birth year, seven pure-stat-potion gates, quick spell and last spellbook page per character, plus the exact eight bytes of current-runtime attack/health/mana bonus members. A title-specific adapter reads these from actual `Character` objects and stages them on copies. The bonus bytes are an explicit raw current-runtime preservation contract, not native MM6 identity or lifetime proof. `PART` remains v1. Offline `CHAR` v1/v2/v3 payloads are superseded; no live `.ec6` slots exist.

`MAPS` requires 67 unique deltas and a name matching the active map's `.dlv`/`.ddm` companion. This checks structural completeness only; a later transaction must compare the expected map-name set and validate each delta against its base map. Raw resistance, held item, buff, hireling and recovery values are included, but their MM6 gameplay meaning, shop/map items, generated NPCs and other runtime state remain outside the candidate. SaveLoad integration, atomic slots and F5 save/quit/relaunch remain open.

See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [NPC State](MM6-F4-NPC-State.md), [F2 Save Fields](Source-MM6-Save-Field-Register-F2.md).

The [transaction checklist](MM6-F4-Save-Transaction-Checklist.md) maps the remaining live owners and explains why the complete mandatory manifest and global commit are still gated. `TIME`/`VISU` have a joint detached runtime capture/stage adapter, and the represented Party families stage together; map PID reconciliation, beacons and MM6-keyed SHOP/bounty state remain open.
