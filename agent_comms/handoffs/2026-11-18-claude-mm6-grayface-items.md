# Claude MM6 grayface-items lane, 2026-11-18

Branch `claude/mm6-grayface-items`, off `claude/mm6` at `eca9f3509`. GrayFace backlog items W22 and W26 from
`project/MM6_GRAYFACE_PATCH_REFERENCE.md`, both rows now marked fixed there.

## Commits

- `1a1b4319a` Keep an MM6 Essence the drinker already drank (W22). `Character::useItem` checks
  `mm6::isRepeatedEssence` (`MM6Potion.h`) before the MM6 drink. A repeated Essence (181 to 187) plays the error sound
  and stays in the hand, with no status text, speech or recovery, as GrayFace's `DontWasteBlackPotion` does by jumping
  to 0x459EE5. Flag `gameplay.fix_wasted_black_potion`, default on. Test `MM6Potion.RepeatedEssenceIsNotWasted`.
- `721c34c3e` Refuse to buy blasters in MM6 shops (W26). `GUIWindow_MM6Shop::sell` refuses an item whose items.txt
  Value is 0, as GrayFace's `CanSellItemHook2` does at the sell click 0x4A53C9. Identify and repair are untouched, the
  patch hooks only the sell call site. Flag `gameplay.fix_shops_buying_blasters`, default on. Test
  `MM6Houses.WeaponShopRefusesBlaster`.
- The docs commit with this handoff and the two reference rows.

## Verified against MM6.exe and the data

- MM6.exe (owner-local, `llvm-objdump`): use-item 0x459040, bottle dispatch 0x459103, refusal 0x459EB8 to 0x459F06
  (0x459EE5 is sound 27 then return). Essence flags at character +296 (+0x128), one dword per Essence in item order,
  matching `FIRST_COMBAT_PE_TRACE_2026-11-12.md`.
- MM6.exe shop code: trade check 0x4A4C30, its three callers 0x4A53C9 (sell), 0x4A55D2 and 0x4A5784. The sell path
  pays through 0x487630 with sound 479 and speech 77, a refused sale plays speech 79 and sound 27 (0x4A54E3, 0x4A4E3F).
  The hint routine 0x485120 takes the shop category as arg 2 and the action as arg 4, action 3 is sell.
- MM6 `icons/items.txt` (research copy and the GrayFace 2.5.7 decoded copy are identical): every Value 0 row listed.
  The ones a shop would otherwise take are 64 Blaster, 65 Blaster Rifle and gold 197 to 199. The rest are 430 to 435
  and 446 and up, which the trade check already refuses. Only MM6 was in scope, MM7 and MM8 data were not read.

## Fixtures

Two Essences of Might added to the first character's backpack. A Blaster (64) added to the first character's backpack.
Everything else is input: drinking by backpack click and paperdoll drop, the shop by Space at the door, the sell
screen by its option, and clicks on the backpack cells.

## Tests

- Revert and fail: with each fix disabled, its test failed (Essence count 2, 1, 0 with two drink sounds and no error
  sound, and the blaster sold with gold 200, 201, 215). Both pass with the fixes.
- MM6 game tests `MM6Potion.*:MM6Houses.*:MM6Melee.*:MM6TurnBased.*:MM6Rest.*:MM6Conditions.*`: 57/57 passed.
- `OpenEnroth_UnitTest`: 1168 tests, 1125 passed, 43 skipped (private-fixture and profile cases whose environment
  inputs were not supplied), 0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840` plus the MM7 potion tests `Issues.Issue563` and
  `Issues.Issue587`: 5/5 passed. `Character.cpp` and `GameConfig.h` are shared, the new branch is MM6 only.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 is not installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/grayface-items/before.png` and `after.png`, the New Sorpigal weapon
shop's sell screen right after a click on a blaster. Before, the blaster is gone and gold reads 201. After, the
blaster is still in the backpack and gold reads 200. The temporary capture test is removed.

## Open

- Our MM6 sell screen gives no response at all. MM6.exe plays sound 479 and speech 77 on a sale, and speech 79 with
  sound 27 on a refused item. Ours is silent for both, so a refused blaster shows only as nothing happening.
- Our MM6 shops show no sell hint, so GrayFace's hint-routine half of W26 (0x485120) has nothing to attach to yet.
