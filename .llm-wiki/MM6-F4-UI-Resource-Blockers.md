---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Route]]"
tags: [mm6, f4, ui, hud, headless]
---

# MM6 F4 UI Resource Blockers

The [public packet](../project/MM6_F4_UI_RESOURCE_BLOCKERS.md) is the bounded first-frame matrix. Source authority: `SetUserInterface` and `UI_Create` in `src/GUI/GUIWindow.cpp`, `Engine::DrawGUI`, and its `UIGame.cpp` consumers. Private evidence was read locally only: the retained MM6 icon set and original manual main adventure screen on pages 30–31. Do not copy the icon inventory, artwork or manual image to this wiki.

Prior MM6 frame/footer/navigation/vital-bar and baked book/action helpers already handle known native pieces. This turn adds only `hudNpcArrowResources`: MM6 returns retained 14×25 unpressed `npc_lup`/`npc_rup` for all alignments; MM7 returns each caller's original alignment strings. A null-renderer 2/2 check validates both MM6 resources and MM7 passthrough. It does not prove follower hit boxes or pressed-state art.

First-frame gate order: `SetUserInterface` still leaves some inherited MM7 requests null, but the MM6 minimap panel now uses the original four-state hour selector. The accepted original executable chooses `tap4` for hours 0–4 and 21–23, `tap3` for hour 5, `tap2` for hours 6–19, and `tap1` for hour 20. It places the selected image at `(468,0)` after automap work and skips palette index zero. All four private 172×142 panels have index-zero aperture pixels although their LOD transparency flags are false. The MM6 resource manager forces zero masking only for those four icons; the GUI selects by `pParty->uCurrentHour`. MM7 keeps its alignment-specific panel. The manual supports the visible day/night context, while the executable supplied the exact thresholds.

Secondary right-panel, dialogue/exit and book-page requests remain separate. The first-map `DoPrepareWorld` menu-window precondition and outer encounter resolver belong to their own workstreams. No normal MM6 chest/new-game guard was opened; no foreground GUI was used. The next agent should use the passed **2/2** `MM6TopPanel*` headless resource checks, validate a first-map `DrawGUI` pass and inspect a rendered frame. Portrait alert/selection resources remain the next distinct first-frame blocker.
