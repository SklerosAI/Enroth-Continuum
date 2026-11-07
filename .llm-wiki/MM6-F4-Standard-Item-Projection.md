---
type: synthesis
up: "[[MM6-F4-Standard-Bonus-Map]]"
related: ["[[MM6-F4-Plain-Item-Projection]]", "[[MM6-F4-Bounded-Inventory-Projection]]"]
tags: [mm6, f4, item, enchantment, runtime]
---

# MM6 F4 Standard Item Projection

The [implementation packet](../project/MM6_F4_STANDARD_ITEM_PROJECTION.md) records a detached raw-preserving MM6 view for standard-enchanted equipment.

The accepted original item-picker trace writes the one-based bonus index and positive strength for equipment template types 3–11. The adapter uses the shared [14-index MM6 map](MM6-F4-Standard-Bonus-Map.md), requires the loaded item table to classify the source as common equipment and rejects a special-bonus word. Capture can change the mapped standard attribute, positive strength and identified/broken bits while retaining all other source bytes. It rejects changed IDs, types or unsupported logical fields. Artifact/relic templates are excluded because the inherited stat reader uses a separate artifact map for them. Strength is limited so the shared `Item::GetValue` expression can multiply by 100 and add the template base price without signed-`int` overflow. This is an arithmetic safety bound, not an original balance rule or proof of all derived effects.

Focused standard-item, bonus-map and bounded-inventory checks passed 6/6 after the guard on a separate arm64 build. The boundary test exercises a value calculation at the largest safe strength, one-above rejection on import/edit, and artifact/relic exclusion. An earlier monitored headless MM6 check with a loaded armor template passed 1/1 over 65 samples without an on-screen window or focus change. The later [bounded inventory stage](MM6-F4-Bounded-Inventory-Projection.md) uses this view but does not attach it to Party, live equipment effects or normal SaveLoad. Native nonzero item evidence, title-specific artifact behavior, modded armor effects and save/reload remain open.

The [value provenance carrier](MM6-F4-Item-Value-Provenance.md) now follows this raw record through shared `Item` copies and is required on capture; integrated linked focused checks passed 13/13. This remains detached from live equipment.

See also: [Standard Bonus Map](MM6-F4-Standard-Bonus-Map.md), [Plain Item Projection](MM6-F4-Plain-Item-Projection.md), [Item Value Provenance](MM6-F4-Item-Value-Provenance.md), [Bounded Inventory Projection](MM6-F4-Bounded-Inventory-Projection.md), [Raw Item Authority](MM6-F4-Raw-Item-Authority.md).
