---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Save-Transaction-Checklist]]", "[[MM6-F4-Shop-Item-Record-Conversion-Gate]]"]
tags: [mm6, f4, inventory, save, item]
---

# MM6 F4 Raw Inventory Section

The [implementation packet](../project/MM6_F4_RAW_INVENTORY_SECTION.md) defines a detached byte-preserving character inventory candidate based on the pinned MMExtension MM6 physical layout.

`INVR` v1 stores four characters × 138 exact 28-byte item records, 126 raw 32-bit grid words and 16 raw equipment words per character. It retains unknown and inactive bytes instead of converting through `Item_MM7`. The generic `.ec6` container recognizes the required standalone section, but the provisional eighteen-section known-save decoder still rejects it before mutation. Focused linked tests passed 5/5 for full physical round trip, malformed/foreign rejection and the known-save guard; the private-backed MM6/LOD gate passed 347/347 and the general non-MM6 gate 437/437.

The accepted F3-matched original executable's free-item path also scans all 138 records and can assign the selected record to grid or equipment. MM6 `Party::Zero()` configures 138 runtime slots, including the `Reset()` path, while the MM7 default remains 126; a monitored headless test restores an item at the final MM6 index. The inherited MM7 party and character snapshot/reconstruct entry points now reject MM6 or 138-slot state before changing their destinations, as does the direct inventory reconstruction path. The full monitored MM6 headless suite passed 51/51 over 1,504 samples with no on-screen window or focus change; an MM7 save/load game control passed 1/1 and rebuilt non-MM6-prefix unit checks passed 438/438. `INVR` still has no live Party/raw-item owner, nonzero original save sample or normal SaveLoad route; item field meanings and complete mutation/save semantics remain open.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Shop Item Conversion Gate](MM6-F4-Shop-Item-Record-Conversion-Gate.md).
