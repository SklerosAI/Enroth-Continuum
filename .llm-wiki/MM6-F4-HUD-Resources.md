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

The omitted History pointer is null. A source audit found its only potential dereference through the Journal overlay; the overlay's MM6 update returns before reading the button, and the right-panel draw returns before the absent texture. The constructor merely stores the pointer. No other current source consumer dereferences it.

The MM6 hit boxes are image-measured candidates pending graphical cursor and click checks. The minimap frame, status art and other skin pieces remain open. The similarly named MM6 book item icons are spell-book inventory art. Frame seams, compass behavior, zoom semantics and action feedback need graphical acceptance when the gameplay guards permit a map.

The MM6 right frame also bakes four lower action controls, identified in the original manual as Cast Spell, Rest, Quick Reference and Game Options. `MM6HudActionControls.h` supplies 29×61 bounded hit boxes at x490/524/559/594 and y398. `UI_Create` uses those rectangles only for MM6, without the absent MM7 action textures; the existing messages and actions remain. `OnButtonClick` skips drawing a missing pressed texture only for these four MM6 buttons. Headless tests inspect the live rects, messages, actions, texture absence and nonoverlap. MM7 continues through its original asset and button branch. Image-measured targeting and press feedback await graphical acceptance.

`CalendarBook` now selects the retained 360×300 MM6 time background while preserving the MM7 `sbdate-time` lookup. Resource availability and title selection have tests; text alignment and Events semantics are unverified. The `arrowl*`/`arrowr*` sequence is already a party-creation resource and does not establish NPC-scroll art. Keep the inherited NPC-scroll icon requests visible as an unresolved presentation seam; do not alias a generic arrow without original role and placement evidence.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [Main Menu Assets](MM6-F4-Main-Menu-Assets.md), [Runtime Isolation](Runtime-Isolation.md).
