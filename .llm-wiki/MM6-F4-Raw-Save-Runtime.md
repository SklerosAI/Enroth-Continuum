---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Known-Save-Assembly]]", "[[MM6-F4-Map-Save-Fields]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, save, raw-items]
---

# MM6 F4 Raw Save Runtime

The [raw owner staging packet](../project/MM6_EC6_RAW_RUNTIME_STAGE_F4.md) describes the first detached runtime join for the nineteen-section EC6 v2 candidate.

`stageRawTitleOwners` requires explicit MM6 title selection and a source-specific 67-name set, base reader and item-template verifier. It decodes the exact v2 manifest, validates every saved map delta against its supplied MM6 base, checks active `CHST` sidecar shape, and retains an owned active indoor or outdoor candidate. The same map pass now serves the v1 and v2 decoders, so their base checks cannot drift independently.

Only after map validation does the function construct fresh `RawInventoryFields`, `RawHeldCursorOwner` and `ShopTitleState` values. Referenced backpack/equipment records, a present cursor and nonempty physical shop records must point to accepted MM6 item templates. Grid/equipment indices must stay within the 138-record inventory and cannot reuse a source record; negative grid footprint cells must identify a positive anchor. Unreferenced inventory records and zero-ID shop records retain all 28 native bytes, including opaque tails. No process global, live `Party`, file or profile is changed. `captureRawTitleOwners` overlays detached owner edits onto the staged candidate and repeats item and 67-map/base validation before returning fields for EC6 encoding.

`preflightStagedRawSaveSlot` connects the schema-isolated [native v2 slot selector](MM6-F4-Atomic-Save-File.md) to the complete raw map/item stage without disk repair or live mutation. A semantically invalid primary can yield a valid backup with `needsRecovery=true`. This still does not validate every session owner, so it is not a production `SaveLoad` adapter or a menu eligibility check. The linked raw-runtime plus atomic-file suite passed **11/11**, including a new synthetic invalid-primary/valid-backup case and both-invalid rejection with disk bytes unchanged.

`PART`/`CHAR` core fields and `QEST`/`EFFT` progress fields now have direct decoded-field Party-copy adapters shared with the v1 byte route. The core stage rechecks widths/ranges; the progress stage rechecks condition clocks and leaves physical inventory to the raw Party adapter. Their linked core/durable/raw-runtime suite passed **17/17** with direct-vs-v1 equivalence. A full v2 Party coordinator, remaining field families and live publication remain separate work.

`stageRawPartyCoreProgressInventory` composes these three families on one detached Party. It requires the saved and staged physical inventory images to agree and rejects unsupported late item unions without changing the caller's baseline. The linked raw-runtime, raw Party inventory and Antique suites passed **11/11**, including a four-character raw recapture. Resistance, buffs, hirelings, recovery and other session/world owners are still outside this bounded result.

The raw stage also retains active actor/sprite/chest counts from the 67-map pass. `stageRawWorldAndVisuals` then shares the v1 NPC/map buffer adapter and stages timer, overlays and CHST on copies. Focused linked raw-runtime/world-runtime tests passed **14/14**, including a late invalid saved actor overlay with prior NPC/timer unchanged. This stage requires an intact validated raw candidate; it does not reconstruct or publish the live map, UI image or renderer resources.

`stageRawPartyRepresentedFields` now continues the Party copy through decoded resistance (`RSTS`), buffs, hirelings and recovery (`RCVR`) after core/progress/raw inventory. Focused linked raw/effect/hireling-recovery checks passed **12/12**, including a late recovery clock mismatch with the baseline untouched. All currently represented Party families can stage, but the result still lacks beacon, cursor/shop and live UI/world ownership; no complete v2 session coordinator or production load exists.

`preflightStagedRawSessionFields` now places native primary/backup selection around the represented Party and world field stages, so a structurally valid primary that fails a **later** item/Party check can yield a valid backup without repairing either file. The linked atomic/raw/effect/hireling set passed **20/20**, including that fallback and both-invalid refusal with disk bytes unchanged. Caller baselines must be fresh MM6 tables/state; this is not full-session admission or a live SaveLoad path.

The v2 session preflight now also prepares an owned outdoor geometry/terrain/spawn plan from its validated active map before selecting a slot. It shares the v1 plan builder, checks the same CHST delta identity and MM6 tile-table bounds, and leaves renderer/resource publication for a later gate. A synthetic outdoor primary with unusable tile selectors falls back to a valid indoor backup without changing either file; a corrected MM6 base selects the outdoor primary with a populated plan. The focused raw/world runtime set passed **11/11**. Indoor active-world construction, final actor identity and normal load remain open. This supersedes the earlier statement that the represented-field coordinator has no outdoor plan.

The [BEAC/IFOV v2 admission policy](../project/MM6_F4_RAW_V2_BEAC_IFOV_ADMISSION.md) keeps the exact nineteen-section format fail-closed: `captureRawTitleOwners()` now requires a source Party and an explicitly observed indoor-face override candidate, rejecting occupied beacons or nonempty IFOV before recapture. The raw Party stage rejects a beacon-bearing baseline so a v2 load cannot silently inherit one. The decoder already rejects BEAC and now has an explicit IFOV-extra regression check. The caller still must derive IFOV from the verified indoor base; direct structural `encodeRawSave()` does not capture live state. No BEAC/IFOV field or production load was admitted.

The [raw cursor/shop handoff](MM6-F4-Raw-Cursor-Shop-Handoff.md) adds a detached `HELD`/`SHOP`/`SCLK` join and an exact equality check against decoded v2 fields. The represented-session preflight now calls that check before selecting a slot. The linked raw/schema/cursor-shop set passed **18/18**. This does not publish a live cursor image, shop service or UI state.

This is still a raw owner candidate. It does not project every source item kind into `CharacterInventory`, reconstruct the active live world, include beacon state, or make the simultaneous Party/world/UI/commerce commit failure atomic. The existing v1 atomic/New Game route remains separate; v2 is not a production save or playable-campaign claim.

The [detached Party inventory stage](MM6-F4-Raw-Party-Inventory-Stage.md) now projects the supported subset of `INVR` into all four 138-slot runtime inventories on a Party copy. It rejects unsupported nonzero item unions and rarity effects, and keeps the full raw owner for recapture. It does not close the remaining item semantics or live commit gates.

The integrated linked Mac run passed the three new raw owner cases, four ordinary map validation cases and four v2 schema cases (**11 passed**); one optional private fixture test skipped because its decoded fixture paths were not supplied. The synthetic cases cover byte-preserving round trip, raw cursor clear, owner edits, wrong title, wrong map set before any base read, late malformed delta, missing base, invalid active references and rejected item templates. No private game file was read for this check.

See also: [Known Save Assembly](MM6-F4-Known-Save-Assembly.md), [Map Save Fields](MM6-F4-Map-Save-Fields.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
