---
type: synthesis
up: "[[MM6-F4-Chest-Map-Load-Route]]"
tags: [mm6, f4, ui, hud, headless]
---

# MM6 F4 UI Resource Blockers

The [public packet](../project/MM6_F4_UI_RESOURCE_BLOCKERS.md) is the bounded first-frame matrix. Source authority: `SetUserInterface` and `UI_Create` in `src/GUI/GUIWindow.cpp`, `Engine::DrawGUI`, and its `UIGame.cpp` consumers. Private evidence was read locally only: the retained MM6 icon set and original manual main adventure screen on pages 30–31. Do not copy the icon inventory, artwork or manual image to this wiki.

Prior MM6 frame/footer/navigation/vital-bar and baked book/action helpers already handle known native pieces. This turn adds only `hudNpcArrowResources`: MM6 returns retained 14×25 unpressed `npc_lup`/`npc_rup` for all alignments; MM7 returns each caller's original alignment strings. A null-renderer 2/2 check validates both MM6 resources and MM7 passthrough. It does not prove follower hit boxes or pressed-state art.

First-frame gate order: `SetUserInterface` leaves several inherited MM7 requests null; `Engine::DrawGUI` draws known frame/status, then the dynamic minimap and missing inherited `ib-autmask-a` overlay; portraits then need unproved alert and selection layers. Retained `tap1`–`tap4` are plausible 172×142 top-panel variants and `mapback` is 151×116, but their selector, transparency, placement and day/night behavior are unproved. Do not map any one variant as a static substitute. The original manual shows the corresponding automap/day-night/follower layout, not the exact resource-state algorithm.

Secondary right-panel, dialogue/exit and book-page requests remain separate. The first-map `DoPrepareWorld` menu-window precondition and outer encounter resolver belong to their own workstreams. No normal MM6 chest/new-game guard was opened; no foreground GUI was used. The next agent should trace the original top-panel state selector, add bounded visual-resource tests, then validate a headless `DrawGUI` pass before any first rendered frame claim.
