---
type: synthesis
up: "[[MM6-F4-Raw-Item-Authority]]"
related: ["[[MM6-F4-Plain-Item-Projection]]", "[[MM6-F4-Bounded-Inventory-Projection]]"]
tags: [mm6, f4, item, enchantment, source]
---

# MM6 F4 Standard Bonus Map

The [source join packet](../project/MM6_F4_STANDARD_BONUS_MAP.md) centralizes the original MM6 item's 14 one-based standard-bonus indices and the fork's runtime attribute mapping.

The pinned MMExtension MM6 item structure labels `Bonus` as an MM6 Stats value plus one, and its MM6 Stats constants list core stats, HP/SP/armor, then fire/electricity/cold/poison resistance. `MM6StandardBonusMap.h` expresses this once; the MM6 item-table loader now uses that same array. Electricity, cold and poison map to the runtime's air, water and body attributes. Earth and mind are not accepted as MM6 standard-bonus indices.

Focused checks cover all 14 forward/reverse mappings. The rebuilt private-backed MM6/LOD aggregate passed 352/352, non-MM6-prefix unit checks 438/438 and a monitored loaded-table headless check 1/1 without window or focus change. The plain raw-item adapter still rejects nonzero bonus words. A later [standard-item projection](MM6-F4-Standard-Item-Projection.md) accepts bounded equipment bonuses in a detached view; effect behavior remains a runtime gate before normal SaveLoad.

See also: [Raw Item Authority](MM6-F4-Raw-Item-Authority.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Standard Item Projection](MM6-F4-Standard-Item-Projection.md), [Bounded Inventory Projection](MM6-F4-Bounded-Inventory-Projection.md), [Shop Item Conversion Gate](MM6-F4-Shop-Item-Record-Conversion-Gate.md).
