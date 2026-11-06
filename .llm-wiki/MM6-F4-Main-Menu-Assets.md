---
type: synthesis
up: "[[MM6-F4-Headless-Harness]]"
related: ["[[Runtime-Isolation]]", "[[MM6-Headless-Integration-Policy]]"]
tags: [mm6, f4, ui, assets]
---

# MM6 F4 Main Menu Assets

The [public packet](../project/MM6_F4_MAIN_MENU_ASSETS.md) records a title-specific main-menu icon and geometry mapping, pending visual acceptance.

The retained ignored MM6 icon evidence contains a title background with four visible actions and four matching overlay entries. The upstream menu asks the MM6 archive for four MM7-only button names and positions its hit boxes lower on the screen. `mainMenuResources(GameTitle)` now chooses MM6 overlays and their bounded image-matched positions for MM6 while preserving the exact MM7 lookups and coordinates. `GUIWindow_MainMenu` uses the choice for loading, hit testing, hover redraw and click feedback. A pure test checks both title branches; a headless test checks the MM6 entries can be read through `ResourceManager`. No proprietary images or detailed inventory are tracked.

This only addresses title-menu requests. Headless startup also logs absent MM7-style in-game HUD icons; those need a separate mapping and visual review. The null renderer cannot establish alignment or interaction quality, so a later graphical pass must inspect all four buttons, cursor hit boxes and action transitions before this menu is accepted.

See also: [Headless Harness](MM6-F4-Headless-Harness.md), [Runtime Isolation](Runtime-Isolation.md), [Headless Integration Policy](MM6-Headless-Integration-Policy.md).
