---
type: synthesis
up: "[[MM6-F4-Held-Item]]"
related: ["[[MM6-F4-Raw-Item-Authority]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, save, held-item, raw-item]
---

# MM6 F4 Raw Held Item Section

The detached `HELD` v2 candidate retains all 28 original MM6 cursor bytes for both present and absent records.

Its required payload is always 32 bytes: version `2`, presence byte `0` or `1`, two zero reserved bytes, then one exact `ItemRecord28`. Present requires a structural item ID in 1–580. Absent requires bytes 0–24 zero and retains opaque bytes 25–27, matching the original cursor-clear trace. An empty v2 packet is therefore not equivalent to the four-byte v1 empty marker. The full wire and mutation contract is in [the implementation packet](../project/MM6_F4_RAW_HELD_ITEM_SECTION.md).

Current `Party` cursor operations copy shared `Item` values, so the new `mm6RawRecord` carrier can cross those runtime moves. Current `HELD` v1 capture/copy-stage still drops it, and an absent shared `Item` has no separate native tail owner. `KnownSaveFields` remains fixed to v1 and provisional `ITEM`; the detached v2 codec is not called from the live save path. Present v1 records cannot be migrated losslessly. The final `.ec6` manifest must revise inventory and cursor ownership together, or reject old partial saves before mutation.

A detached [raw cursor owner](MM6-F4-Raw-Cursor-Owner.md) now retains the absent tail and stages the proved full set/partial clear writes. It is not yet attached to live `Party` operations.

Three synthetic linked tests passed: exact present/absent-tail round trip; malformed envelope or section rejection before destination assignment; foreign title and old-version rejection. No private assets, live save, cursor image or GUI were involved. The existing post-commit policy resets the transient click offset; image publication and loaded-icon checks must occur after full staged-world validation and commit.

See also: [Held Item](MM6-F4-Held-Item.md), [Raw Cursor Owner](MM6-F4-Raw-Cursor-Owner.md), [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
