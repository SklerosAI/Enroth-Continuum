---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[MM6-F4-NPC-State]]", "[[Source-MM6-Save-Field-Register-F2]]"]
tags: [mm6, f4, save]
---

# MM6 F4 Known Save Assembly

The [assembly packet](../project/MM6_EC6_KNOWN_ASSEMBLY_F4.md) originally defined a partial sixteen-section candidate. The provisional v1 schema now requires `CHST` for MM6 chest title state and the active raw delta sidecar plus `SCLK` for source-proven shop/guild refresh clocks, making eighteen required sections. Older seventeen/sixteen-section candidates reject before mutation. See the [CHST transaction packet](MM6-F4-Chest-EC6-Transaction.md). It is a pure encode/decode and copy-assignment boundary, not the final whole-save manifest or a live MM6 save path.

Every `PART`, `CHAR`, `QEST`, `EFFT`, `ITEM`, `HEAD`, `TIME`, `UIST`, `VISU`, `MAPS`, `RESC`, `RSTS`, `HELD`, `BUFF`, `HIRE`, `RCVR`, `CHST` and `SCLK` section must be present once and required. Optional extras are rejected pending an explicit preservation policy. The container recognizes standalone `SHOP`, `BEAC` and `INVR` section tags and accepts optional `IFOV`, but the eighteen-section known assembly still rejects all four before mutation; a focused linked test covers them. The decoder validates all field families and cross-checks the party clock across `PART`, `HEAD` and `RCVR`, plus active-character selection, before assigning any output. The independent `TIME` event clock is not required to equal party playing time.

`CHAR` v4 preserves seven signed stat bonuses, AC/level/age modifiers, birth year, seven pure-stat-potion gates, quick spell and last spellbook page per character, plus the exact eight bytes of current-runtime attack/health/mana bonus members. A title-specific adapter reads these from actual `Character` objects and stages them on copies. The bonus bytes are an explicit raw current-runtime preservation contract, not native MM6 identity or lifetime proof. `PART` v3 preserves current-runtime fall, flight, hazard-timer, flag and justice-counter state on detached Party copies; older provisional versions reject. See [Party Movement Save](MM6-F4-Party-Movement-Save.md) and [Justice Counter Save](MM6-F4-Justice-Counter-Save.md). Offline `CHAR` v1/v2/v3 payloads are superseded; no live `.ec6` slots exist.

`MAPS` requires 67 unique deltas and a name matching the active map's `.dlv`/`.ddm` companion. This checks structural completeness only; a later transaction must compare the expected map-name set and validate each delta against its base map. Raw resistance, held item, buff, hireling and recovery values are included, but their MM6 gameplay meaning, shop/map items, generated NPCs and other runtime state remain outside the candidate. SaveLoad integration, atomic slots and F5 save/quit/relaunch remain open.

## Coordinated raw candidate (EC6 v2)

`MM6RawSaveFields` assembles a separate nineteen-section v2 candidate: it replaces logical `ITEM` with exact four-character `INVR` images, replaces logical `HELD` v1 with raw `HELD` v2, and requires physical `SHOP` alongside `SCLK`. The remaining sixteen section families carry forward, with `QEST` and `EFFT` reused independently of the v1 `ITEM` codec. Both candidates share cross-field clock, active-character, 67-map and active-delta checks. Every v2 section is required; missing, optional, duplicate, extra or mixed v1/v2 section shapes reject before publication. Decode stages the entire record and assigns the destination only after all codecs and cross-checks pass. The title argument must be MM6.

The v1 known-save decoder explicitly requires container v1. Existing atomic slot reading and New Game autosave still use that provisional v1 path; the v2 candidate does not silently migrate old files or become a live save. V2 still needs beacon state, owner staging, expected map-key/base validation and one global commit transaction before SaveLoad integration. Synthetic linked tests passed four new v2 cases plus the nine v1 known-save and three container cases; no private game file was used.

See also: [F4 Offline Seams](MM6-F4-Offline-Seams.md), [NPC State](MM6-F4-NPC-State.md), [F2 Save Fields](Source-MM6-Save-Field-Register-F2.md).

The [transaction checklist](MM6-F4-Save-Transaction-Checklist.md) maps the remaining live owners and explains why the complete mandatory manifest and global commit are still gated. `TIME`/`VISU` have a joint detached runtime capture/stage adapter, and the represented Party families stage together; map PID reconciliation and beacons remain open. The [SCLK section](MM6-F4-Shop-Guild-Clock-Save.md) covers only shop/guild refresh deadlines; stock, bans and bounty remain open.
