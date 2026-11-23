# Fortune's Turn: MM7 random reward rerolls

This opt-in MM7 private mod includes chest reroll. Turn on **Use private mods** and
**Fortune's Turn · MM7** in Atlas and relaunch MM7. The dice checkbox on
the left edge of the game HUD, beside the first party portrait, switches the
popup on or off immediately. The checkbox choice is saved in the MM7 game
configuration; Atlas remains the master launch toggle. The chest’s small Reroll button stays available when the in-game X is off; the X controls only non-chest rewards.
The HUD control and popup buttons use brass, dark wood and gold lettering to
match the MM7 interface; the loot preview keeps the game world visible behind
its stone frame.

Clicking an eligible dead monster pauses the game and opens a loot preview.
The preview shows the roll's full item name, visible item art, attack or armor
stats, any magic bonus, value, and item description. Previewing does not
identify the item in the party inventory; Take preserves the rolled item's
original identified state.
**Reroll** draws a fresh seed and repeats the monster's original gold dice,
item-drop chance, treasure level and treasure type. **Take** grants exactly the
displayed reward once and removes the corpse. Escape leaves the corpse intact
and awards nothing. Preview rolls restore the artifact-generation ledger, so
discarded offers do not consume artifact eligibility.

The same checkbox also covers random items awarded by directly clicking
trash heaps, decorations, and scripted faces. The click event runs once. If it
actually generates a random item, the item is held back and displayed in the
same paused popup. Reroll retains that event's treasure level and item type.
Take puts the selected item in hand; Escape leaves it behind. The event's
other one-time effects, such as disease or a used-up pile, have already run
and are not repeated by Reroll. A click that yields no random item proceeds
normally. Scripted fixed items and gold do not open the popup.

Monthly town-hall bounties use the same preview frame. It shows the chosen
monster, its level, and the gold reward. Reroll chooses only from that hall's
normal eligible monster pool and retains the monthly deadline. Keep or Escape
leaves the current choice in place. A killed or already claimed bounty cannot
be rerolled.

The three original ore smith services also use this frame. The original
script consumes one held ore once, then Fortune's Turn previews the random
item at that ore's original treasure level and item type. The preview shows
the item's art, stats, magic bonus, value, and description. Reroll changes
only that output. Take or Escape puts the selected item in hand; neither
repeats the ore consumption. Other scripted gifts keep their normal path.

Successful Enchant Item casts on regular weapons show the same frame after
the casting screen closes. The first enchantment has already succeeded and
the spell's normal mana and recovery are charged once. Reroll chooses another
special enchantment from the spell's original weapon and mastery pool on the
same item. Keep or Escape leaves the current enchantment. Failed casts,
broken items, armor, and deterministic weapon effects do not open this frame.

Only corpses whose random reward has not yet been generated are eligible.
Actors with carried items, stored items, stored gold or other pre-generated
contents use normal MM7 looting. MM6 has its own isolated Fortune’s Turn launch choice and helper; MM8 is unaffected. Direct pickup of an already generated ground item is unchanged.

For rapid manual testing, starting a new MM7 game with this Atlas mod active
arms a one-time setup: on the first visit to Emerald Isle's Dragon's Lair in
that same game session, the first live dragon there is already a lootable dead
corpse. Loading another saved game cancels the pending setup. The setup does
not alter the published ECv0.1.0 tag or existing saves. A newly started game
must reach the lair before changing sessions for this convenience to apply.

Headless tests cover the toggle boundary, deferred award, reroll, Escape,
one-time Take, fixed-item exclusion, clicked script rewards, new-game dragon setup,
the original ore smith transaction, bounty deadline and eligibility, and one-cast
weapon enchanting. Visible graphical placement and click acceptance remain for a normal play session.
