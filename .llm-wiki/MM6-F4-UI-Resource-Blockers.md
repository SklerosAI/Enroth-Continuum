---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Route]]"
tags: [mm6, f4, ui, hud, headless]
---

# MM6 F4 UI Resource Blockers

The [public packet](../project/MM6_F4_UI_RESOURCE_BLOCKERS.md) is the bounded first-frame matrix. Source authority: `SetUserInterface` and `UI_Create` in `src/GUI/GUIWindow.cpp`, `Engine::DrawGUI`, and its `UIGame.cpp` consumers. Private evidence was read locally only: the retained MM6 icon set and original manual main adventure screen on pages 30–31. Do not copy the icon inventory, artwork or manual image to this wiki.

Prior MM6 frame/footer/navigation/vital-bar and baked book/action helpers already handle known native pieces. This turn adds only `hudNpcArrowResources`: MM6 returns retained 14×25 unpressed `npc_lup`/`npc_rup` for all alignments; MM7 returns each caller's original alignment strings. A null-renderer 2/2 check validates both MM6 resources and MM7 passthrough. It does not prove follower hit boxes or pressed-state art.

First-frame gate order: `SetUserInterface` still leaves some inherited MM7 requests null, but the MM6 minimap panel now uses the original four-state hour selector. The accepted original executable chooses `tap4` for hours 0–4 and 21–23, `tap3` for hour 5, `tap2` for hours 6–19, and `tap1` for hour 20. It places the selected image at `(468,0)` after automap work and skips palette index zero. All four private 172×142 panels have index-zero aperture pixels although their LOD transparency flags are false. The MM6 resource manager forces zero masking only for those four icons; the GUI selects by `pParty->uCurrentHour`. MM7 keeps its alignment-specific panel. The manual supports the visible day/night context, while the executable supplied the exact thresholds.

Secondary right-panel, dialogue/exit and book-page requests remain separate. The first-map headless probe now establishes the `DoPrepareWorld` menu-window precondition by entering party creation, then calls `GameUI_LoadPlayerPortraitsAndVoices` before map preparation as `Game::gameLoop` does. The outer encounter resolver belongs to its separate workstream. No normal MM6 chest/new-game guard was opened; no foreground GUI was used. The passed **2/2** `MM6TopPanel*` resource checks and first outdoor `DrawGUI` null-renderer pass support a later rendered-frame inspection. The timing gems now use source-backed MM6 names, red/yellow precedence and four coordinates; the selected-portrait sprite is routed and awaits rendered alignment checking.

The accepted original executable loads the native 7×10 green/yellow/red timing gems and draws them using party alert bits 0x10 and 0x20 (red wins), with a fixed four-entry x table and y378 framebuffer destination. `hudPortraitAlertResources` and `hudPortraitAlertPosition` route only MM6; MM7 alignment names and coordinates pass through. A monitored 2/2 headless test checked names, dimensions, precedence, positions and title isolation, with no window/focus change. The original selected-portrait effect is an `aframe1` sprite-frame draw, not an `IB-selec` icon. Its four center/baseline anchors, 69×91 dimensions and palette **2** are now traced and title-locally routed. `ib-mb-A`, exit/dialogue and inactive-buff requests have separate consumers.

The full-world probe’s `aframe1` warning came from party creation’s icon lookup. The same sprite-frame effect is now decoded from `sprites.lod` through a title-local palette-2 2D cache and used by both selection and party creation. The original framebuffer path centers it at x50/163/276/388 with baseline y467; current 69×91 art yields top-left x16/129/242/354, y377. Original face positions are x22/135/248/360, y383, so MM6 portrait drawing is adjusted to those origins. The corrected headless palette and timing test passed 2/2 with the tree-palette case (78 monitor samples, no onscreen window or focus change). Final animation and alignment need rendered comparison.

The MM6 sprite-frame row layout has 16-bit flags, light radius at offset 46, palette ID at 48, frame time at 52 and total time at 54; see the [pinned MMExtension `SFTItem` schema](https://github.com/GrayFace/MMExtension/blob/a2ab9b12705de7576aecf8111168666d6398f830/Scripts/Structs/01%20common%20structs.lua#L2405-L2446). The old MM7-style 32-bit flags shifted these fields: the prior claim that `aframe1` lasts the full 48-tick cycle is **superseded**. The six retained `aframe1`–`aframe6` frames last 8 ticks each and wrap after 48 ticks. A linked headless test checks each frame and wraparound. The current HUD cache still draws only the first frame; animate it in a later UI slice.

The separate `GameTests_MM6DrawGUI.cpp` fixture is disabled by default and must run alone in a fresh process. It stages first-map deltas and a deterministic MM6 chest owner in memory, enters the scoped first outdoor world, and calls `Engine::DrawGUI` through the null renderer. Its initial failure was a test-only missing outer-loop portrait-load call; adding `GameUI_LoadPlayerPortraitsAndVoices` in the same order as `Game::gameLoop` resolved the null portrait texture. The monitored isolated case passed **1/1** across 116 samples, with zero onscreen windows and no focus change. This validates completion of the active ordinary first-map draw route, not rendered pixels, input, secondary screens, or the normal MM6 new-game guard.

An explicit hidden OpenGL followup now captures pixels without taking focus or showing a window. Its [offscreen capture page](MM6-F4-Offscreen-Render-Capture.md) records the test guard and corrected 2D flush order. Four colored portraits now appear; the prior black ovals came from reading before queued quads were submitted. Corrected sprite-frame palettes render green and red trees as their original art specifies. The cyan/red ground or water region is still wrong. The frame is diagnostic; visual correctness and normal new-game entry remain open.

The MM6 character record now creates title-specific stats, skills, inventory,
awards and exit buttons. Fourteen normal, pressed and active icon names resolved
in the local MM6 archive. Focused headless key, button and adjacent-frame
navigation checks passed 3/3, and the full first-map gate passed 25/25 after
integration, including MM7 controls. The click test checks the right side of
each proposed button rectangle; exact art placement remains for F5a.

A hidden OpenGL diagnostic now renders the inventory character window and
checks nonblank pixels in all five proposed button rectangles; switching to
stats changes the strip. The integrated fresh-process run passed 1/1 with no
sampled visible window or focus change. This does not establish original
coordinates or complete visual correctness: inherited MM7 paperdoll and strip
requests still name art absent from MM6. See the [offscreen packet](../project/MM6_F4_OFFSCREEN_FRAME_CAPTURE.md).

The base paperdoll claim above is superseded by the 2026-11-09 title-specific
body and arm route. Its twelve face layouts follow the public
[MMExtension MM6 paperdoll table](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Misc/Paper%20Doll/mm6/PaperDol.txt).
The focused MM6 character checks passed 4/4 and the MM7 inventory control
passed 1/1. The hidden character capture passed 1/1 and changed only the
right-panel pixels compared with the earlier local capture. A separate
inherited panel lookup still fails; other equipped-item layers and original tab
placement have not been accepted. See the [updated offscreen packet](../project/MM6_F4_OFFSCREEN_FRAME_CAPTURE.md).

The next title-specific layer selects MM6 armor body and arm art by the active
item picture. All active MM6 armor templates matched thirteen retained layer
sets from the public MMExtension table; four extra public variant rows lacked
original archive art and were excluded. The integrated character suite passed
6/6, including a click-area check for equipped armor. An MM7 inventory control
and the monitored hidden armor capture each passed 1/1. The latter changed
pixels in the paperdoll region without showing a window or changing focus.
The next MM6-only route uses native item-table screen positions for main-hand
weapons, bows and shields, with public dual-wield offsets for swords and
daggers. Two daggers without public overrides use a translated native
position as an inferred fallback. The three inherited MM6 artifact-ID
assertions are gone. The integrated character suite passed 9/9, including
weapon and shield hit areas; the MM7 inventory control and hidden weapon
capture passed 1/1 each. The monitor saw no visible window or focus change.
Other equipment categories, panel art, exact placement and visible play remain
open; see the [capture packet](../project/MM6_F4_OFFSCREEN_FRAME_CAPTURE.md).

The subsequent MM6-only wearable route uses native item-table positions for
boots, helms and belts, with public PaperDol image substitutions for helms
and belts. All active retained items in these categories resolved. The
integrated character suite passed 11/11, including a combined clickable
wearable check; the MM7 inventory control and hidden wearable capture passed
1/1 each. The monitor saw no visible window or focus change. Cloaks, panel
art, original alignment and visible input/save remain open.
