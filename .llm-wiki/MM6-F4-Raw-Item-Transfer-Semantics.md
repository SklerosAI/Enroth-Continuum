---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Item-Value-Provenance]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, item, inventory, cursor, source]
---

# MM6 F4 Raw Item Transfer Semantics

The [source-audit packet](../project/MM6_F4_RAW_ITEM_TRANSFER_SEMANTICS.md) records bounded original-MM6 physical item writes without publishing private executable material.

The item-event backpack removal path clears the first 25 bytes of a selected 28-byte record and its grid footprint while retaining the final three bytes. A free physical slot is selected by zero ID across all 138 records, so zero-ID slots can retain opaque tails. The cursor setter copies a full incoming 28-byte record, relocates an occupied cursor with a full copy and then clears its first 25 bytes. Equipment placement writes a reference, ID and body-location byte; equipment removal and ordinary UI pickup are not yet proved. Existing [shop purchase evidence](MM6-F4-Shop-Purchase-Write-Gate.md) independently has a full-copy and first-25-byte source clear.

The [item value carrier](MM6-F4-Item-Value-Provenance.md) is necessary but insufficient. A complete transaction must stage character physical records, index tables and held record together, preserve zero-ID tails, prove every writer and commit only after all late failures have been ruled out. The current [four-character stage](MM6-F4-Bounded-Party-Inventory.md) still rejects item movement; live SaveLoad remains guarded.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Item Value Provenance](MM6-F4-Item-Value-Provenance.md), [Bounded Party Inventory](MM6-F4-Bounded-Party-Inventory.md), [Shop Purchase-Write Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
