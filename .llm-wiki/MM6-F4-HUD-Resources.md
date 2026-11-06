---
type: synthesis
up: "[[MM6-F4-Headless-Harness]]"
related: ["[[MM6-F4-Main-Menu-Assets]]", "[[Runtime-Isolation]]"]
tags: [mm6, f4, ui, assets]
---

# MM6 F4 HUD Resources

The [public packet](../project/MM6_F4_HUD_RESOURCES.md) records a narrow title-specific HUD compass and zoom lookup candidate.

The current UI skin and `UI_Create` ask the MM6 archive for many MM7-only entries. The ignored retained evidence supplies a compass strip and two zoom-labelled icons as provisional candidates, so `hudNavigationResources(GameTitle, ...)` selects those three identifiers only for MM6; MM7 neutral, good and evil names pass through unchanged. A pure title-isolation test and a headless resource-presence test cover those lookups, without asserting visual or zoom semantics. No image data or detailed private inventory is committed.

The rest of the skin, minimap frame, status art and five tome controls remain unresolved. The similarly named MM6 book item icons are spell-book inventory art, so they cannot establish a HUD tome mapping. Visual alignment, compass behavior, zoom semantics and action feedback need later graphical acceptance when the gameplay guards permit a map.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [Main Menu Assets](MM6-F4-Main-Menu-Assets.md), [Runtime Isolation](Runtime-Isolation.md).
