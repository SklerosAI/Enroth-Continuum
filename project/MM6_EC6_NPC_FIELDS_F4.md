# MM6 `.ec6` NPC and group state

Status: bounded offline field codec and copy-staging only (2026-11-06). This does not enable MM6 save/load or prove NPC gameplay.

## Evidence and scope

`NPCStats::InitializeMm6` imports the retained MM6 `npcdata.txt` columns for ID, name, portrait, house, profession, join flag, news topic and three event topics. An aggregate check of the ignored F3-matched copy found **398 numeric records, IDs 1–398**, maximum house 557, profession 77, portrait 653 and name length 26 bytes. The F2 profession ledger's 70 NPC records are the **generic-profession subset**, not the whole table. Neither source establishes a native MM6 whole-save layout. The runtime uses a 501-slot `pNPCData` array, 51 `pGroups` entries and an `uNumNewNPCs` active upper bound (399 after this table import). `SaveLoad.cpp` already preserves the two arrays for MM7, while MM6 save/load remains guarded. The retained MM6 icons copy has no `npcgroup.txt`, and MM6 `InitializeMm6` does not call that loader; nonzero group initialization and group semantics still need title-specific evidence.

The version-1 `.ec6` `RESC` payload is a **new logical runtime snapshot**, not a native MM6 structure or MM7 `NPCData_MM7` image. It stores all 501 `NPCData` slots and all 51 group words in slot order, plus the active NPC count. Each NPC stores a bounded name (0–1024 bytes) and every current scalar field: portrait, raw flags, fame, reputation, house, profession, greeting, join flag, `field_24`, all six dialogue IDs, sex, ability-use value and news topic. Unknown flag bits, generic profession IDs 59–77, group word values and fields with unproven semantics round trip numerically. No proprietary name or table row is embedded in Git.

`RESC` is required, versioned and bounded to 547,200 bytes. The decoder checks exact array counts, active count 0–501, name length/NUL, boolean and enum ranges, full payload consumption and outer container integrity before staging. A failed decode leaves the live `NPCStats` untouched. Staging makes a copy of `NPCStats`, replaces the three saved fields there, and retains original source tables, generated-map NPCs, profession metadata and other runtime members. Callers must apply the staged copy only after all other save families and map state validate.

## Remaining acceptance work

- Integrate `RESC` into the complete MM6 `.ec6` transaction and atomic file lifecycle; it is not a live save path yet.
- Reconcile group initialization and group event meaning against retained MM6 evidence and F5 gameplay. The 398 static rows and active bound 399 do not imply that the fixed 501-slot runtime array can be shortened safely.
- Decide whether generated `pAdditionalNPC[100]`, `uNewlNPCBufPos`, NPC sacrifice clocks, party hireling copies and other transient NPC fields need persistence. This codec does not claim to cover them.
- Verify moved NPCs, greetings, hire/dismiss, per-day ability reset, modded names/professions and leave/re-entry across save/quit/relaunch in F5. Exact profession effects and generic-ID collisions remain separate gameplay work.

See [F2 NPC profession audit](MM6_NPC_PROFESSION_F2.md), [save field register](MM6_SAVE_FIELD_REGISTER_F2.md) and [`.ec6` container](MM6_EC6_CONTAINER_F4.md).
