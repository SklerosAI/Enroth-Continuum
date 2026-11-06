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

The MM6 right panel bakes four book images into its frame, and the manual names Quests, Notes, Map and Events. The manual says Events manages time; a retained time-themed book background and the current Calendar view's time fields support a Calendar **input** route. `UI_Create` now makes four MM6 hit boxes over the baked art, routes them to Quest, Notes, Map and Calendar actions, and omits the separate MM7 History button. MM7 retains five image-backed controls. MM6 book overlays and flashing draws skip absent MM7 tome art. A headless test checks live rectangles, messages, actions and no fifth button, but actual book-screen delivery and page content await first-map readiness.

The MM6 hit boxes are image-measured candidates pending graphical cursor and click checks. The minimap frame, status art and other skin pieces remain open. The similarly named MM6 book item icons are spell-book inventory art. Frame seams, compass behavior, zoom semantics and action feedback need graphical acceptance when the gameplay guards permit a map.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [Main Menu Assets](MM6-F4-Main-Menu-Assets.md), [Runtime Isolation](Runtime-Isolation.md).
