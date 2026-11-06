---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[MM6-F4-NPC-State]]", "[[Source-MM6-Save-Field-Register-F2]]"]
tags: [mm6, f4, save]
---

# MM6 F4 Known Save Assembly

The [assembly packet](../project/MM6_EC6_KNOWN_ASSEMBLY_F4.md) defines a revised partial v1 candidate for the sixteen `.ec6` sections currently landed. It is a pure encode/decode and copy-assignment boundary, not the final whole-save manifest or a live MM6 save path.

Every `PART`, `CHAR`, `QEST`, `EFFT`, `ITEM`, `HEAD`, `TIME`, `UIST`, `VISU`, `MAPS`, `RESC`, `RSTS`, `HELD`, `BUFF`, `HIRE` and `RCVR` section must be present once and required. Optional extras are rejected pending an explicit preservation policy. The decoder validates all field families and cross-checks the party clock across `PART`, `HEAD` and `RCVR`, plus active-character selection, before assigning any output. The independent `TIME` event clock is not required to equal party playing time.

`CHAR` v3 preserves seven signed stat bonuses, AC/level/age modifiers, birth year, seven pure-stat-potion gates, quick spell and last spellbook page per character. A title-specific adapter reads these from actual `Character` objects and stages them on copies. `PART` remains v1. The changed offline payload supersedes prior `CHAR` v1/v2 samples; no live `.ec6` slots exist. MM6 rule semantics and the remaining character state are separate gates.

`MAPS` requires 67 unique deltas and a name matching the active map's `.dlv`/`.ddm` companion. This checks structural completeness only; a later transaction must compare the expected map-name set and validate each delta against its base map. Raw resistance, held item, buff, hireling and recovery values are included, but their MM6 gameplay meaning, shop/map items, generated NPCs and other runtime state remain outside the candidate. SaveLoad integration, atomic slots and F5 save/quit/relaunch remain open.

See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [NPC State](MM6-F4-NPC-State.md), [F2 Save Fields](Source-MM6-Save-Field-Register-F2.md).

The [transaction checklist](MM6-F4-Save-Transaction-Checklist.md) maps the remaining live owners and explains why the complete mandatory manifest and global commit are still gated. `TIME`/`VISU` now have a joint detached runtime capture/stage adapter; map PID reconciliation and all-party staging remain open.
