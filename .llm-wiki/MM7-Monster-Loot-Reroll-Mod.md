---
type: synthesis
up: "[[MM7-Chest-Reroll-Mod]]"
related: ["[[MM7-Disposable-Save-Reload]]"]
tags: [mm7, mods, monster, loot, bounty, enchanting]
---

# MM7 Fortune's Turn Mod

The [source contract](../project/MM7_MONSTER_LOOT_REROLL_MOD.md) defines one
Atlas launch toggle and one in-game dice checkbox for random monster and click
loot, monthly town-hall bounties, original ore smith rewards, and successful
Enchant Item casts on regular weapons. The checkbox now shows a gold X when
enabled. Since 2026-11-22, the same Atlas choice also enables the chest’s
Reroll button. The chest button works while X is off; X controls only the
non-chest rewards. This supersedes the earlier separate chest launcher option.

All eligible results use the same paused stone Reroll/Take frame, brass and
dark-wood buttons, and bright gold text. Item results show art, name, combat
or item stats, magic bonus, value and description without changing the item's
identified flag. The bounty result shows its monster, level and gold reward.

Monster previews defer the one-time corpse award. Clicked events run once and
retain their other effects. Original global ore smith events 501-503 consume
one held ore before the random output is intercepted; Take or Escape awards
only the selected item. Bounty rerolls keep the hall's eligible monster pool
and monthly deadline, and stop after a kill or reward. Weapon rerolls keep the
successful spell's mana and recovery charge, original item, and its weighted
special-enchantment pool. Fixed gifts, authored gold, failed casts and
pre-generated contents stay on their existing paths.

The focused headless mod gate passed 7/7. The full MM7 GameTest gate ran 741
registered cases in six isolated workers with no failures; opt-in tests skip
when their launch flags are off. `Issue2817` passed in that isolated run. A
hidden OpenGL capture showed the gold X on the live HUD. Installed-app click
acceptance for the three added flows remains for graphical play.

On 2026-11-22, the merged choice passed fresh headless coexistence checks:
39 passed and one expected disabled-mode skip with Dragoon, All Artifacts,
Fortune’s Turn and vendor refresh active. The legacy chest flag was off,
proving Fortune’s Turn alone provides chest reroll. The chest test keeps the
HUD X off while clicking Reroll. Swift environment contracts, Atlas page
controls, full AtlasApp typecheck and changed C++ lint passed.

See [MM7 Chest Reroll Mod](MM7-Chest-Reroll-Mod.md) for the chest button’s
provenance and claim rules. Atlas retains one isolated Fortune’s Turn choice
per title, and a separate MM6 All Artifacts choice.
