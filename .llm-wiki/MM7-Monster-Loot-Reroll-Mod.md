---
type: synthesis
up: "[[MM7-Chest-Reroll-Mod]]"
related: ["[[MM7-Disposable-Save-Reload]]"]
tags: [mm7, mods, monster, loot]
---

# MM7 Click Loot Reroll Mod

The separate [source contract](../project/MM7_MONSTER_LOOT_REROLL_MOD.md)
defines an Atlas launch toggle and an in-game dice checkbox beside the first
portrait. Eligible random monster rewards and random scripted rewards from
clicked trash heaps, decorations and faces use a paused Reroll/Take popup.
The current local build uses a stone popup over the visible world, with brass
and dark-wood buttons and bright gold text. The HUD checkbox and dice have the
same palette.
The latest popup shows the roll's identified name, base combat or item stats,
magic bonus, value and item description directly on screen. It does not
change the rolled item's identified flag, so Take preserves its original
inventory state. The focused display and loot tests passed 4/4; an offscreen
capture showed the text fitting around item art.
Preview generation restores the artifact ledger and does not award loot; Take
awards the selected result once. Clicked events run once and their other effects
are not repeated on reroll. Fixed rewards, gold and pre-generated contents stay
on the inherited loot path. A new game with this mod active also arms a
same-session dead dragon in Emerald Isle's Dragon's Lair for quick manual
acceptance. Existing saves are not changed.

Hidden OpenGL captures of the popup and HUD were inspected. Mouse acceptance
in the installed app still requires graphical play.
See [MM7 Chest Reroll Mod](MM7-Chest-Reroll-Mod.md) for the independent chest
option.
