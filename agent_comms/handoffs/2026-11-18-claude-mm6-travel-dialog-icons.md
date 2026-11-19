# Claude MM6 travel dialog icons lane, 2026-11-18

Branch `claude/mm6-travel-dialog-icons`, off `claude/mm6` at `a927bd004` (GrayFace travel lane merged).

## Commits

- `7c677f70c` Draw the MM6 map edge travel dialog with MM6 art. `GUIWindow_Travel` gets `initializeMM6`/`updateMM6` in
  `MM6TransitionDialog.cpp`, the MM7 path is untouched. Game test `MM6MapTravel.EdgeDialogArt`.

## Verified against MM6.exe and the data

Static trace of the owner's MM6.exe (llvm-objdump, image base 0x400000):

- Init 0x43A4E0: background `sprintf("evpan%03u", 4)` (0x4BF5BC), destination picture `outside` (0x4BF01C), window
  type 17 at 640x480, hint "Leave %s" (global 410).
- Buttons, window type 17 in 0x419320 (0x419931..0x419B2E): Cancel (566,313,75,33) msg 91 hotkey 'N', name global 156
  "Stay in this Area", pressed BUTTESC2. Yes (486,313,75,33) msg 90 hotkey 'Y', pressed BUTTYES2. Portrait
  (525,34,63,73) msg 90 param 1 hotkey space. Room view (8,8,460,344) msg 90 param 1, no hotkey.
- Draw 0x43A630: background at (481,0) opaque, `outside` at (525,34) (pixel offset 22285 at 0x4BE7F8), BUTTESC1 at
  (566,313) and BUTTYES1 at (486,313) colour-keyed. Destination name centred in x 493 w 126 at y 2. Body text
  global 128 "It will take %d days to travel to %s." + "\n \n" + global 126 "Do you wish to leave %s?", centred in
  x 483 w 148 at (212 - h) / 2 + 101, white. MM6's global.txt has no id 663 (599 lines), so a one-day trip reads
  "1 days" in MM6, ported as is.
- Room descriptor 0 (table 0x4BE88C) has evpan 4, the same panel.
- MM6 icons.lod has EVPAN004, Outside, BUTTESC1/2, BUTTYES1/2, and no `x_ok_u`, `x_x_u`, `evt02`.

## Other MM7-only icon names

A temporary probe in `NullRenderer` logged every named 2D quad without an icons.lod entry while the full MM6 game
suite ran (299 tests, 297 passed, 2 skipped). Besides the travel dialog (fixed here) it found only the `i1_`..`i6_` buff and spell
animation frames, which are sprites in MM6's SPRITES.LOD, not missing art. Static grep of `getIcon("...")` literals
against MM6's icons.lod lists 100+ names MM6 lacks, almost all in MM7-only branches. Not fixed, possibly reachable:

- `GUIWindow_Inventory_CastSpell` (UIInventory.cpp) draws `x_x_u` as its cancel art. It opens for item-targeted spells
  (Fire Aura, Recharge Item, Enchant Item, Vampiric Weapon by MM7 id). No MM6 test reaches it.
- The MM6 indoor entry/exit dialog (`GUIWindow_IndoorEntryExit::updateMM6`) uses MM6 names but not MM6's layout. MM6.exe
  (draw 0x43A2xx, buttons window type 18 at 0x419A25) draws `evpan%03d` of the house's room at (481,0), BUTTESC1 at
  (566,313) and BUTTYES1 at (486,313), with hit boxes at y 313. Ours draws `backevt` at (477,0) and the pressed
  BUTTESC2/BUTTYES2 art at (556,445)/(476,445) with hit boxes at y 445, over the HUD buttons.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/travel-dialog-icons/before.png` and `after.png`, oute3 west edge at
(-22450, -10000). Before: three red missing-texture signs, one at the top left of the minimap
area (the `evt02` background at (477,0)) and two over the HUD menu buttons at (476,451) and (556,451). The text sits
on the HUD's stained glass panes and the `outside` picture over the minimap. After: the dark marble `evpan004` panel
from x 481, "Castle Ironfist" on top, the `outside` picture, the text centred on the panel, the thumbs-up Yes and the
door No buttons at y 313 on the panel, and the HUD menu buttons, food 7, gold 200, portraits and the "Leave New
Sorpigal" status line untouched. Nothing else wrong in either frame.

## Test results

- `MM6MapTravel.EdgeDialogArt` fails with the fix reverted (the No click on the art leaves the dialog open,
  `screenTape.back()` stays `SCREEN_CHANGE_LOCATION`), passes with it.
- Full MM6 game suite `MM6*` with the fix (and the logging-only probe): 297/297 passed, 2 skipped (the private-research
  save cases), 299 total.
- After removing the probe: `MM6MapTravel.*:MM6Transport.*:MM6Houses.*` 54/54.
- `OpenEnroth_UnitTest` with `OPENENROTH_MM6_PATH`: 1166/1166.
- MM7 controls (`UITransition.cpp/h` are shared, the MM7 path only gained an MM6 early return):
  `Prs.Pr2354:Prs.Pr2956:Issues.Issue840:Issues.Issue1197:Issues.Issue1532:Issues.Issue1547` 6/6, and
  `Issues.Issue691:Issues.Issue7*` 21/21 (Issue691 is the MM7 test that opens the travel dialog).
- `check_style` clean. `check_tidy` not run, clang-tidy 22 is not installed locally.
