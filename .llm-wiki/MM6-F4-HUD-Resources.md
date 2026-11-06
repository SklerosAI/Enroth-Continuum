---
type: synthesis
up: "[[MM6-F4-Headless-Harness]]"
related: ["[[MM6-F4-Main-Menu-Assets]]", "[[Runtime-Isolation]]"]
tags: [mm6, f4, ui, assets]
---

# MM6 F4 HUD Resources

The [public packet](../project/MM6_F4_HUD_RESOURCES.md) records title-specific HUD compass/zoom and frame candidates, with interaction and visual gates.

The current UI skin and `UI_Create` ask the MM6 archive for many MM7-only entries. The ignored retained evidence supplies a compass strip and two zoom-labelled icons as provisional candidates, so `hudNavigationResources(GameTitle, ...)` selects those three identifiers only for MM6; MM7 neutral, good and evil names pass through unchanged. A pure title-isolation test and a headless resource-presence test cover those lookups, without asserting visual or zoom semantics. No image data or detailed private inventory is committed.

The owner's original manual page 30 and the retained images establish four distinct MM6 structural HUD frames: right panel, bottom portrait strip, top edge and left edge. Their image dimensions and the 640×480 composition fix the right and bottom draw origins; top and left edges meet at the existing viewport boundary. `hudFrameAssets` and `hudFramePositions` select these only for MM6. The MM7 alignment names and frame positions remain unchanged. Headless tests check MM6 dimensions, while pure tests check title isolation.

The MM6 right panel bakes four book images into its frame, and the manual names Quests, Notes, Map and Events. The inherited UI creates five tome controls with separate MM7 images. The MM6 hit boxes and Events route remain unresolved; do not create or hide a fifth MM6 control by inference. The minimap frame, status art and other skin pieces also remain open. The similarly named MM6 book item icons are spell-book inventory art. Frame seams, compass behavior, zoom semantics and action feedback need graphical acceptance when the gameplay guards permit a map.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [Main Menu Assets](MM6-F4-Main-Menu-Assets.md), [Runtime Isolation](Runtime-Isolation.md).
