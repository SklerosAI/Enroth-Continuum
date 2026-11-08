#include "MM6Temple.h"

#include <algorithm>
#include <vector>

#include "Engine/MM6/MM6ActualStatCondition.h"
#include "Engine/Objects/Character.h"

#include "Utility/Segment.h"

static constexpr int HOUSE_TEMPLE_STONE = 70;
static constexpr int HOUSE_TEMPLE_BAA = 78;

static Condition worstCondition(const Character &character) {
    return mm6::actualStatCondition(GameTitle::MM6, [&](Condition condition) { return character.conditions.has(condition); });
}

static bool hasFullHealthAndMana(const Character &character) {
    return character.health >= character.GetMaxHealth() && character.mana >= character.GetMaxMana();
}

/**
 * @return                              The game day on which the condition started, mod 7, plus 1. MM6 multiplies the
 *                                      heal price by this, not by how long the condition has lasted.
 */
static int conditionDayFactor(const Character &character, Condition condition) {
    return character.conditions.get(condition).toDays() % 7 + 1;
}

bool mm6::templeOffersHeal(int houseId, const Character &character) {
    if (houseId == HOUSE_TEMPLE_BAA) {
        for (Condition condition : Segment(CONDITION_CURSED, CONDITION_PARALYZED))
            if (character.conditions.has(condition))
                return true;
        if (character.conditions.has(CONDITION_UNCONSCIOUS) && !character.conditions.has(CONDITION_DEAD))
            return true;
        return !hasFullHealthAndMana(character) && character.conditions.hasNone({CONDITION_DEAD, CONDITION_PETRIFIED, CONDITION_ERADICATED});
    }

    if (worstCondition(character) == CONDITION_GOOD && hasFullHealthAndMana(character))
        return false;
    return houseId == HOUSE_TEMPLE_STONE || !character.conditions.has(CONDITION_ERADICATED);
}

int mm6::templeHealPrice(const Character &character, float value) {
    const Condition worst = worstCondition(character);
    int multiplier = 1;
    int days = 1;
    if (worst == CONDITION_DEAD || worst == CONDITION_PETRIFIED || worst == CONDITION_ERADICATED) {
        multiplier = worst == CONDITION_ERADICATED ? 10 : 5;
        days = conditionDayFactor(character, worst);
    } else {
        for (Condition condition : Segment(CONDITION_CURSED, CONDITION_UNCONSCIOUS))
            days = std::max(days, conditionDayFactor(character, condition)); // A condition that isn't set reads as day 0.
    }
    return std::max(1, static_cast<int>(days * multiplier * static_cast<double>(value)));
}

void mm6::templeHeal(int houseId, Character *character) {
    const CharacterConditions oldConditions = character->conditions;
    const int oldHealth = character->health;
    const int oldMana = character->mana;

    character->conditions.resetAll();
    character->health = character->GetMaxHealth();
    character->mana = character->GetMaxMana();

    std::vector<Condition> kept;
    if (houseId == HOUSE_TEMPLE_BAA) {
        kept = {CONDITION_DEAD, CONDITION_PETRIFIED, CONDITION_ERADICATED};
    } else if (houseId >= 69 && houseId <= 77 && houseId != HOUSE_TEMPLE_STONE) {
        kept = {CONDITION_ERADICATED};
    }
    const bool keptAny = std::any_of(kept.begin(), kept.end(), [&](Condition condition) { return oldConditions.has(condition); });
    if (!keptAny)
        return;
    for (Condition condition : kept)
        character->conditions.set(condition, oldConditions.get(condition));
    character->health = oldHealth;
    character->mana = oldMana;
}
