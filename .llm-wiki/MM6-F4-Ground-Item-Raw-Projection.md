---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Item-Value-Provenance]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, item, ground, save]
---

# MM6 F4 Ground Item Raw Projection

The detached [ground projection](../project/MM6_F4_GROUND_ITEM_RAW_PROJECTION.md) keeps the complete MM6 100-byte map sprite record and its 28-byte item region as source authority.

Map decoders currently copy that item into an MM7-shaped snapshot, then shared reconstruction clears native raw provenance and uses MM7 potion/gold ID tests. The map-arrangement pass can also apply inherited potion, wand or special-item generation. A source-typed view must therefore read raw map bytes before snapshot reconstruction. The new helper does so for bounded common plain/gold and standard items, retaining opaque bytes for empty or removed records. It rejects unsupported categories and mutations before returning a changed record.

The four synthetic cases passed C++23 syntax checking but await a linked integration run. This seam is detached: live ground pickup/drop, actor loot, map writer, complete save and all-owner commit remain guarded. Original ground removal and generation byte rules still need direct proof; no private asset data or native save was used.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Item Value Provenance](MM6-F4-Item-Value-Provenance.md), [Raw Item Transfer Semantics](MM6-F4-Raw-Item-Transfer-Semantics.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
