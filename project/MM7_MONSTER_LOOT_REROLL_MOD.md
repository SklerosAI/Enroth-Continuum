# MM7 monster loot reroll mod

This is a separate, opt-in MM7 private mod. Turn on **Use private mods** and
**Reroll click loot · MM7** in Atlas and relaunch MM7. The dice checkbox on
the left edge of the game HUD, beside the first party portrait, switches the
popup on or off immediately. The checkbox choice is saved in the MM7 game
configuration; Atlas remains the master launch toggle.

Clicking an eligible dead monster pauses the game and opens a loot preview.
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

Only corpses whose random reward has not yet been generated are eligible.
Actors with carried items, stored items, stored gold or other pre-generated
contents use normal MM7 looting. The chest mod is independent; MM6 and MM8 are
unaffected. Direct pickup of an already generated ground item is unchanged.

For rapid manual testing, starting a new MM7 game with this Atlas mod active
arms a one-time setup: on the first visit to Emerald Isle's Dragon's Lair in
that same game session, the first live dragon there is already a lootable dead
corpse. Loading another saved game cancels the pending setup. The setup does
not alter the published ECv0.1.0 tag or existing saves. A newly started game
must reach the lair before changing sessions for this convenience to apply.

Headless tests cover the toggle boundary, deferred award, reroll, Escape,
one-time Take, fixed-item exclusion, clicked script rewards and new-game dragon setup. Visible
graphical placement and click acceptance remain for a normal play session.
