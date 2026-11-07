---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Bounded-Party-Inventory]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, item, provenance, transaction]
---

# MM6 F4 Item Value Provenance

The [implementation packet](../project/MM6_F4_ITEM_VALUE_PROVENANCE.md) records an optional 28-byte MM6 source record carried by the shared `Item` value and checked by the bounded common-rarity plain and standard item adapters.

The field survives ordinary value copies in inventory and cursor paths, while `Item::Reset()` and MM7 wire reconstruction clear it. The adapters reject capture if the carrier is absent or changed. This establishes a source-byte transport primitive, not a complete live raw owner: physical slot clearing, item identity, creation, cross-context transfers, final `INVR`/`HELD` schema and normal SaveLoad remain open. The private local source check found no authentic nonzero MM6 saved-item sample.

Linked focused checks passed 13/13; the rebuilt non-MM6 unit suite passed 438/438. Monitored headless controls passed MM7 save/load 1/1 and MM6 loaded-table item checks 2/2, without an on-screen window or focus change.

The [raw transfer audit](MM6-F4-Raw-Item-Transfer-Semantics.md) now identifies source paths where vacated physical records keep three tail bytes; the carrier alone does not publish those slot mutations.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Raw Item Transfer Semantics](MM6-F4-Raw-Item-Transfer-Semantics.md), [Bounded Party Inventory](MM6-F4-Bounded-Party-Inventory.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
