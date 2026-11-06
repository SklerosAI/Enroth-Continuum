---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Route]]"
tags: [mm6, f4, ui, hud, headless]
---

# MM6 F4 UI Resource Blockers

The [public packet](../project/MM6_F4_UI_RESOURCE_BLOCKERS.md) is the bounded first-frame matrix. Source authority: `SetUserInterface` and `UI_Create` in `src/GUI/GUIWindow.cpp`, `Engine::DrawGUI`, and its `UIGame.cpp` consumers. Private evidence was read locally only: the retained MM6 icon set and original manual main adventure screen on pages 30–31. Do not copy the icon inventory, artwork or manual image to this wiki.

Prior MM6 frame/footer/navigation/vital-bar and baked book/action helpers already handle known native pieces. This turn adds only `hudNpcArrowResources`: MM6 returns retained 14×25 unpressed `npc_lup`/`npc_rup` for all alignments; MM7 returns each caller's original alignment strings. A null-renderer 2/2 check validates both MM6 resources and MM7 passthrough. It does not prove follower hit boxes or pressed-state art.

First-frame gate order: `SetUserInterface` still leaves some inherited MM7 requests null, but the MM6 minimap panel now uses the original four-state hour selector. The accepted original executable chooses `tap4` for hours 0–4 and 21–23, `tap3` for hour 5, `tap2` for hours 6–19, and `tap1` for hour 20. It places the selected image at `(468,0)` after automap work and skips palette index zero. All four private 172×142 panels have index-zero aperture pixels although their LOD transparency flags are false. The MM6 resource manager forces zero masking only for those four icons; the GUI selects by `pParty->uCurrentHour`. MM7 keeps its alignment-specific panel. The manual supports the visible day/night context, while the executable supplied the exact thresholds.

Secondary right-panel, dialogue/exit and book-page requests remain separate. The first-map `DoPrepareWorld` menu-window precondition and outer encounter resolver belong to their own workstreams. No normal MM6 chest/new-game guard was opened; no foreground GUI was used. The next agent should use the passed **2/2** `MM6TopPanel*` headless resource checks, validate a first-map `DrawGUI` pass and inspect a rendered frame. The timing gems now use source-backed MM6 names, red/yellow precedence and four coordinates; the selection frame remains the next distinct first-frame blocker.

The accepted original executable loads the native 7×10 green/yellow/red timing gems and draws them using party alert bits 0x10 and 0x20 (red wins), with a fixed four-entry x table and y378 framebuffer destination. `hudPortraitAlertResources` and `hudPortraitAlertPosition` route only MM6; MM7 alignment names and coordinates pass through. A monitored 2/2 headless test checked names, dimensions, precedence, positions and title isolation, with no window/focus change. The remaining ordinary first-frame null request is the inherited `IB-selec-A`; `ib-mb-A`, exit/dialogue and inactive-buff requests have separate consumers. The accepted PE has no inspected `IB-selec` string; active-character selection behavior needs a targeted draw trace before mapping or skipping it.

A separate full-world probe logged missing `aframe1` during party creation. The current party-creation UI requests it as an icon, while retained F3 and original sprite-frame lookup place the 69×91 oval in `sprites.lod`. A naive `getSprite` substitution would use the generic red-channel sprite conversion; verify the original palette/transparency and rendered frame before changing this path.
