# Claude MM6 shop-unconscious lane, 2026-11-18

Branch `claude/mm6-shop-unconscious`, off master `23f6eb0c9`. GrayFace backlog item W28 in
`project/MM6_GRAYFACE_PATCH_REFERENCE.md`.

## Commits

- `7d0bc4977` Refuse MM6 shop services to a character who can't act. `GUIWindow_MM6Shop::houseScreenClick` refuses
  with `SOUND_error` when the active character can't act, and `GUIWindow_MM6Shop::Update` draws "%s is in no condition
  to %s" (427 with 562 "do anything") in the right panel on every shop screen, at (212 - h) / 2 + 101 in pale yellow.
  No config flag, because MM6.exe already refused. The W28 rows of the reference doc are updated.

## Verified against MM6.exe and the data

- Shop click handler 0x4A4CA0 calls CanAct 0x4876E0 before its jump on the screen state (2 to 66), and on failure
  draws 427/562 at x 483 w 148, y (212 - h) / 2 + 101, then plays sound 27 (0x4A4E3F) and returns.
- Shop screen draws call CanAct and draw the same line: weapon shop states 1 to 6 (jump table 0x4A1020, CanAct at
  0x4A0033, 0x4A02F6, 0x4A0644, 0x4A0878, 0x4A0B28, 0x4A0D7A), the sell/identify/repair screen at 0x4A11A2, the guild
  buy screen at 0x49BF6D.
- GrayFace `FixDeadIdentifyItemProc` (`MMHooks.pas:3062`, hook at MM6 0x41C58E) sits in the item info box 0x41C440,
  where the selected member's Identify Item skill (char byte +117) and Repair skill set the identified and repaired
  bits on a right-clicked item. It returns "no member" when the member can't act.
- MM6 `global.txt` 427 and 562 render as "Zoltan is in no condition to do anything" (pinned by the new tests).
- Only MM6 data and MM6.exe were in scope. MM7 already refuses paid shop services in `GUIWindow_Shop::houseScreenClick`
  through `checkIfPlayerCanInteract`, and the change is in an MM6-only file.

Throwaway game test (deleted), MM6: a character with Grandmaster Identify Item, conscious or unconscious, right-clicks
the unidentified sword in the weapon shop identify screen and it stays unidentified (MM6 shops have no item info box
yet). In the character screen the conscious one identifies it, the unconscious one doesn't (the `UIPopup.cpp` CanAct
check). Leaving the character screen with the unconscious one selected moves focus to character 3 in the same tick.

## Fixtures

The sword (item 1) in Zoltan's backpack is made unidentified or broken, and Zoltan is knocked out (health 0,
unconscious) before taping. Everything else is input: Space at the shop door, digit 1, the Identify or Repair option,
a left click on the sword.

## Test results

- New `MM6Houses.WeaponShopIdentifyUnconscious` and `MM6Houses.WeaponShopRepairUnconscious`, each a two-iteration
  loop (unconscious, conscious). With the fix reverted both fail in the unconscious iteration only (gold 200 to 125 or
  189, item identified or repaired, "Done!" or "Good as New!", no error sound, no refusal line).
- MM6 `MM6Houses.*`, `MM6HouseExit.*`, `MM6Transport.*`, `MM6Hirelings.*`, `MM6PortraitSelection.*`: 50/50 passed.
- `OpenEnroth_UnitTest`: 1125 passed, 45 skipped (private-fixture env inputs not supplied), 0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3 passed.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/shop-unconscious/before.png` and `after.png`, from a temporary
`MM6OffscreenFrame.DISABLED_ShopUnconsciousCapture` (removed).

## Found, not fixed

- With a character who can't act, MM6.exe hides the shop proprietor's menu (button count 0) and shows the no
  condition line instead (weapon shop state 1 at 0x4A0031). Our menu still lists the services. The screens behind them
  now refuse.
- MM6 shops have no right-click item info box on the shelf or backpack. When it's ported, GrayFace's W28 check (skip
  the skill identify and repair for a member who can't act) has to come with it.
- The shop footer is drawn over the status bar, so "Done!" and "Good as New!" never show on these screens. Not checked
  against MM6.exe.
