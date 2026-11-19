#pragma once

#include <algorithm>
#include <initializer_list>
#include <utility>

#include "Core/Time/Time.h"

#include "Engine/Objects/Character.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

/**
 * @param drinker                       Character that would drink.
 * @param item                          Held item.
 * @return                              Whether the item is an Essence (181-187) that the drinker already drank, so
 *                                      drinking it would do nothing.
 */
inline bool isRepeatedEssence(const Character &drinker, ItemId item) {
    const int id = std::to_underlying(item);
    if (id < 181 || id > 187)
        return false;
    return drinker._pureStatPotionUsed[static_cast<Attribute>(id - 181)]; // Essences of Might to Luck follow the attribute order.
}

/**
 * Effect of an MM6 herb or potion on the character that eats or drinks it. MM6 potions have no power, every amount is
 * fixed.
 *
 * @param drinker                       Character that eats or drinks.
 * @param item                          Held item, an MM6 herb (160-162) or filled bottle (164-188).
 * @param now                           Party time.
 * @return                              Whether the item can be drunk. The empty bottle 163 and the bottle templates
 *                                      past 188 can't, and are left in the hand. A drink that changes nothing, like a
 *                                      second Essence of Might, is still drunk.
 * @offset 0x459040
 */
inline bool drinkPotion(Character &drinker, ItemId item, Time now) {
    auto setStatBonuses = [&](int bonus) {
        for (Attribute stat : allStatAttributes())
            drinker._statBonuses[stat] = bonus; // Bonuses are assigned, a weaker potion after a stronger one lowers them.
    };
    auto setResistanceBonuses = [&](int bonus) { // MM6 Fire, Cold, Electricity, Poison and Magic.
        drinker.sResFireBonus = drinker.sResWaterBonus = drinker.sResAirBonus = drinker.sResBodyBonus = drinker.sResMagicBonus = bonus;
    };
    auto lowerStat = [&](Attribute stat, int amount) {
        drinker._stats[stat] = std::max(1, drinker._stats[stat] - amount);
    };
    auto drinkEssence = [&](Attribute stat, std::initializer_list<Attribute> lowered, int amount) {
        if (drinker._pureStatPotionUsed[stat])
            return;
        drinker._stats[stat] += 15;
        drinker._pureStatPotionUsed[stat] = true;
        for (Attribute other : lowered)
            lowerStat(other, amount);
    };
    auto applyBuff = [&](CharacterBuff buff) {
        drinker.pCharacterBuffs[buff].Apply(now + Duration::fromHours(6), MASTERY_MASTER, 5, 0, 0);
    };

    switch (std::to_underlying(item)) {
    case 160: // Poppysnaps.
        if (!drinker.conditions.has(CONDITION_POISON_WEAK))
            drinker.conditions.set(CONDITION_POISON_WEAK, now); // No resistance roll.
        return true;
    case 161: // Phirna Root.
        drinker.mana = std::min(drinker.mana + 2, drinker.GetMaxMana());
        break;
    case 162: // Widoweeps Berries.
        drinker.Heal(2);
        break;
    case 164: // Cure Wounds.
        drinker.Heal(10);
        break;
    case 165: // Magic Potion.
        drinker.mana = std::min(drinker.mana + 10, drinker.GetMaxMana());
        break;
    case 166: // Energy.
        setStatBonuses(10);
        break;
    case 167: // Protection.
        drinker.sACModifier = 10;
        break;
    case 168: // Resistance.
        setResistanceBonuses(10);
        break;
    case 169: // Cure Poison.
        for (Condition condition : {CONDITION_POISON_WEAK, CONDITION_POISON_MEDIUM, CONDITION_POISON_SEVERE})
            drinker.conditions.reset(condition);
        break;
    case 170: // Supreme Protection.
        drinker.sACModifier = 20;
        break;
    case 171: // Restoration.
        for (Condition condition : allConditions())
            if (condition != CONDITION_DEAD && condition != CONDITION_PETRIFIED && condition != CONDITION_ERADICATED)
                drinker.conditions.reset(condition); // Unconscious too, even with no hit points left.
        break;
    case 172: // Extreme Energy.
        setStatBonuses(20);
        break;
    case 173: // Super Resistance.
        setResistanceBonuses(20);
        break;
    case 174: // Heroism.
        applyBuff(CHARACTER_BUFF_HEROISM);
        break;
    case 175: // Haste, a weak character is hasted too.
        applyBuff(CHARACTER_BUFF_HASTE);
        break;
    case 176: // Stone Skin.
        applyBuff(CHARACTER_BUFF_STONESKIN);
        break;
    case 177: // Bless.
        applyBuff(CHARACTER_BUFF_BLESS);
        break;
    case 178: // Divine Power.
        drinker.sAgeModifier++;
        drinker.sLevelModifier = 20;
        break;
    case 179: // Divine Cure, uncapped and with no dead check.
        if (drinker.health <= drinker.GetMaxHealth()) {
            drinker.health += 100;
            if (drinker.health > 0)
                drinker.conditions.reset(CONDITION_UNCONSCIOUS);
            drinker.sAgeModifier++;
        }
        break;
    case 180: // Divine Magic, uncapped.
        if (drinker.mana <= drinker.GetMaxMana()) {
            drinker.mana += 100;
            drinker.sAgeModifier++;
        }
        break;
    case 181: // Essence of Might.
        drinkEssence(ATTRIBUTE_MIGHT, {ATTRIBUTE_INTELLIGENCE}, 5);
        break;
    case 182: // Essence of Intellect.
        drinkEssence(ATTRIBUTE_INTELLIGENCE, {ATTRIBUTE_MIGHT}, 5);
        break;
    case 183: // Essence of Personality.
        drinkEssence(ATTRIBUTE_PERSONALITY, {ATTRIBUTE_SPEED}, 5);
        break;
    case 184: // Essence of Endurance.
        drinkEssence(ATTRIBUTE_ENDURANCE, {ATTRIBUTE_MIGHT, ATTRIBUTE_INTELLIGENCE, ATTRIBUTE_PERSONALITY, ATTRIBUTE_ACCURACY,
                                           ATTRIBUTE_SPEED, ATTRIBUTE_LUCK}, 1);
        break;
    case 185: // Essence of Accuracy.
        drinkEssence(ATTRIBUTE_ACCURACY, {ATTRIBUTE_LUCK}, 5);
        break;
    case 186: // Essence of Speed.
        drinkEssence(ATTRIBUTE_SPEED, {ATTRIBUTE_PERSONALITY}, 5);
        break;
    case 187: // Essence of Luck.
        drinkEssence(ATTRIBUTE_LUCK, {ATTRIBUTE_ACCURACY}, 5);
        break;
    case 188: // Rejuvenation.
        for (Attribute stat : allStatAttributes())
            lowerStat(stat, 1);
        drinker.sAgeModifier = 0;
        break;
    default:
        return false;
    }
    drinker.playReaction(SPEECH_DRINK_POTION);
    return true;
}

} // namespace mm6
