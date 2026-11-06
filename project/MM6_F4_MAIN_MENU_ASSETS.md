# MM6 main-menu resource mapping

Status: **source candidate with headless resource checks; graphical acceptance pending** (2026-11-06).

The current main menu loads four MM7 button icons and uses MM7 coordinates for both hit boxes and click feedback. Those requests return missing-entry diagnostics against the owner's private MM6 icon archive. The MM6 title background itself is present. Its four visible button labels correspond to four separate MM6 overlay entries in the retained, ignored asset index. A bounded pixel comparison against the private title image located those overlays at four vertically spaced positions in the upper-right portion of the 640×480 composition.

`mainMenuResources(GameTitle)` now supplies the four icon lookup identifiers and coordinates. `GUIWindow_MainMenu` uses that map for icon loading, hit boxes, hover redraw and click feedback. The MM7 branch retains its exact existing names and positions. No archive entries or image bytes are copied to Git. A pure test checks both branches and a headless MM6 test checks that the selected background and four icons exist in the private resource manager. The separate missing MM7-style in-game HUD icons are outside this main-menu change.

The headless renderer cannot show alignment, hover animation, text legibility, or clickable feel. A later owner-available graphical check must inspect the title at native aspect, move the cursor across all four buttons, click through each action, and compare button rectangles with the displayed art. That check should also decide whether MM6's additional animation frames belong in the hover state. Until then, this mapping is an asset-availability and geometry candidate, not visual acceptance or game playability.

See the [headless harness packet](MM6_F4_HEADLESS_HARNESS.md), [runtime isolation policy](../.llm-wiki/Runtime-Isolation.md), and [MM6 headless integration policy](../.llm-wiki/MM6-Headless-Integration-Policy.md).
