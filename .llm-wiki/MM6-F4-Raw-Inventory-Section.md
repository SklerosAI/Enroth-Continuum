---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Save-Transaction-Checklist]]", "[[MM6-F4-Shop-Item-Record-Conversion-Gate]]"]
tags: [mm6, f4, inventory, save, item]
---

# MM6 F4 Raw Inventory Section

The [implementation packet](../project/MM6_F4_RAW_INVENTORY_SECTION.md) defines a detached byte-preserving character inventory candidate based on the pinned MMExtension MM6 physical layout.

`INVR` v1 stores four characters × 138 exact 28-byte item records, 126 raw 32-bit grid words and 16 raw equipment words per character. It retains unknown and inactive bytes instead of converting through `Item_MM7`. The generic `.ec6` container recognizes the required standalone section, but the provisional eighteen-section known-save decoder still rejects it before mutation. Focused linked tests passed 5/5 for full physical round trip, malformed/foreign rejection and the known-save guard; the private-backed MM6/LOD gate passed 347/347 and the general non-MM6 gate 437/437.

This layout is supported by the pinned MMExtension declarations, not by a nonzero original save sample. Slot activity beyond the current runtime's 126-record character capacity, equipment reference semantics, runtime mutation paths and all-owner publication remain open. `INVR` is not wired to Party or normal SaveLoad.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Shop Item Conversion Gate](MM6-F4-Shop-Item-Record-Conversion-Gate.md).
