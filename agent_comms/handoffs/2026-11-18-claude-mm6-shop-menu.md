# Claude MM6 shop-menu lane, 2026-11-18

Branch `claude/mm6-shop-menu`, off `claude/mm6` `9eb81732f` (W28 merged). Follows
`2026-11-18-claude-mm6-shop-unconscious.md`.

## Commits

- `cede6983e` Hide MM6 service menus from a character who can't act. `GUIWindow_MM6House::menuNeedsActingCharacter`
  (tavern, training) and its `GUIWindow_MM6Shop` override (weapon, armor, magic, general store, only while no shop
  screen is open) feed `turnsAwayActiveCharacter`. While it holds, `Update` gives every option an empty line, so its
  button gets height 0, and draws "%s is in no condition to %s" (427, 562) at (212 - h) / 2 + 101 in pale yellow.
  `selectService` refuses in both classes. No config flag, it is vanilla. The two W28 tests now open the screen
  before selecting Zoltan. The W28 row of the reference doc mentions the menu gate.

## Verified against MM6.exe

Every per-type house draw (table 0x4983F4) was read for a CanAct (0x4876E0) call on state 1:
- Gated, failure branch sets the dialogue's button count [+40] to 0 and draws 427/562 at x 483 w 148,
  y (212 - h) / 2 + 101: weapon 0x4A0031, armor 0x4A1A61, magic 0x49A591, general store 0x4A1714, guilds 0x49B994,
  mercenary and thieves' guilds 0x49C7CE, stable and boat 0x49D042 (jump table 0x49DA68), tavern 0x49EDD1 (jump
  table 0x49FAC4), training 0x49A202. The success branch restores the count from [10345980] every frame, so the
  menu comes back on the first frame with a character who can act.
- Not gated: temple 0x49DCC0 and bank 0x49E460 have no CanAct call. Town hall, throne room, seer, dungeon, house,
  jail and circus have no per-type draw.
- Already gated in our port: `MM6Guild`, `MM6SkillGuild`, `MM6Transport`. Fixed here: the four shop kinds, tavern,
  training.

## Fixtures

Zoltan is knocked out (health 0, unconscious) before taping. Zoltan and the second character get 1000 experience in
the training test. Everything else is input: Space at the door, digit keys, a left click where the option is drawn
for a conscious character.

## Test results

- New `MM6Houses.WeaponShopMenuUnconscious` and `MM6Houses.TrainingUnconscious`, two-iteration loops (unconscious,
  conscious). The menu test also selects the second character again and checks the five lines come back at once.
  With the fix reverted both fail in the unconscious iteration only (menu listed, Identify Items opens, Zoltan trains
  to level 2, his HP changes, 10 gold taken).
- The reordered `WeaponShopIdentifyUnconscious` and `WeaponShopRepairUnconscious` still fail with the W28 click
  refusal removed, and pass with it.
- MM6 `MM6Houses.*`, `MM6HouseExit.*`, `MM6Transport.*`, `MM6Hirelings.*`, `MM6PortraitSelection.*`: 52/52 passed.
- `OpenEnroth_UnitTest`: 1125 passed, 45 skipped (private-fixture env inputs not supplied), 0 failed.
- MM7 controls `Prs.Pr2354:Prs.Pr2956:Issues.Issue840`: 3/3 passed. Only MM6 house files changed.
- `check_style` clean. `check_tidy` not run, clang-tidy 22 isn't installed locally.

## Captures

`local-game-data/mm6/task-captures-2026-11-18/shop-menu/before.png` and `after.png`, from a temporary
`MM6OffscreenFrame.DISABLED_ShopMenuCapture` (removed).

## Not covered

- No tavern test. The tavern goes through the same `GUIWindow_MM6House` path the training test covers.
