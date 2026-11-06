# MM6 HUD navigation resources

Status: **three title-specific lookup candidates; headless resource test pending; visual acceptance pending** (2026-11-06).

The inherited `SetUserInterface` skin requests many MM7 alignment-specific icons that are absent from the private MM6 icon archive. `UI_Create` likewise requests MM7 tome and status art. The retained archive contains a compass strip and two zoom-labelled controls whose names and dimensions make them plausible HUD candidates. The exact plus/minus meaning remains provisional. `hudNavigationResources(GameTitle, ...)` selects those three MM6 identifiers for every alignment while passing through the exact existing MM7 identifiers. `SetUserInterface` uses the result at its three alignment branches. A headless test checks that all three chosen MM6 entries can be read, and a pure test checks the title selection and exact MM7 neutral, good and evil preservation; neither test establishes zoom semantics.

The remaining skin, minimap frame, tome and status icon requests stay explicit presentation blockers. In particular, the similarly named MM6 `book` item art belongs to spell-book inventory entries and is not evidence for the five HUD tome buttons. The current code still uses inherited MM7 HUD coordinates and draw paths, so resource presence alone does not establish a usable MM6 in-game interface. No private image bytes or detailed icon inventory are tracked.

After the save/chest map gates open, a graphical pass must inspect the compass crop and heading, zoom button meaning and feedback, and the surrounding HUD layout at the actual render resolution. Resolve the other icon families from source evidence and then verify them visually before claiming an MM6 HUD.

See the [main-menu resource packet](MM6_F4_MAIN_MENU_ASSETS.md), [headless harness packet](MM6_F4_HEADLESS_HARNESS.md), and [MM6 screen audit](MM6_UI_SCREEN_AUDIT_F2.md).
