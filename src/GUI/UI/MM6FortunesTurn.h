#pragma once

#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Item.h"
#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

/**
 * @return                              Whether MM6 Fortune's Turn is on, both its launch flag and the in-game dice
 *                                      checkbox.
 */
bool fortunesTurnOn();

/**
 * Rolls a dead monster's reward the way MM6 rolls it when the corpse is clicked, without giving it: gold dice and the
 * item drop chance on the gameplay RNG, the item through MM6's item picker. The picker's LCG moves on, so the next roll
 * differs, but the generated artifact flags don't change.
 *
 * @param actor                         Dead monster.
 * @return                              Gold and item the corpse would give.
 */
MonsterLootOffer rollCorpseFortune(const Actor &actor);

/**
 * Rolls a random event item the way MM6 GiveItem rolls it, without giving it. The picker's LCG moves on, the generated
 * artifact flags don't change.
 *
 * @param level                         Treasure level 1..6.
 * @param type                          Requested item kind.
 * @return                              Rolled item.
 */
Item rollFortuneItem(ItemTreasureLevel level, RandomItemType type);

/**
 * Marks a taken random artifact generated, as MM6's item picker does when it rolls one.
 *
 * @param itemId                        Item the party takes.
 */
void claimFortuneItem(ItemId itemId);

/**
 * Puts an item from `rollFortuneItem` on the cursor, where MM6 GiveItem puts it, and claims it.
 *
 * @param item                          Item the party takes.
 */
void takeFortuneItem(const Item &item);

/**
 * Keeps the Fortune's Turn checkbox on the MM6 HUD, beside the fourth portrait's bars, while the launch flag is set,
 * and draws it and its dice on the game screen. The button comes and goes with the flag.
 */
void updateFortunesTurnToggle();

} // namespace mm6
