#include "Engine/Spells/MM6PowerCureTransaction.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "Engine/Spells/MM6PowerCureRules.h"
#include "Engine/Spells/MM6SpellRules.h"

namespace mm6 {
namespace {

bool hasUnprovedCondition(const Character &character) {
    for (int value = std::to_underlying(CONDITION_FIRST); value < std::to_underlying(CONDITION_GOOD); ++value)
        if (character.conditions.has(static_cast<Condition>(value)))
            return true;
    return false;
}

}  // namespace

PowerCureStateChange stagePowerCureOnCharacters(const PowerCureTransactionInput &input,
                                                std::array<Character, 4> *characters) {
    if (input.title != GameTitle::MM6 || input.route != PowerCureCastRoute::Spellbook ||
        input.spell != SPELL_BODY_POWER_CURE || !characters || input.casterIndex < 0 ||
        input.casterIndex >= 4 || input.expectedCasterMana < 0 || !input.spellKnown ||
        input.partyInvisible || input.hasHeldItem || input.turnBasedMode ||
        input.underwater || input.hasActorTarget)
        throw std::runtime_error("Invalid MM6 Power Cure transaction context");

    const SpellManaRecovery rule = spellManaRecovery(input.spell, input.mastery);
    const int healAmount = powerCureHealAmount(input.spell, input.mastery, input.skillLevel);
    if (rule.mana <= 0 || rule.recoveryTicks <= 0 || healAmount <= 0)
        throw std::runtime_error("Invalid MM6 Power Cure spell rule");

    const Character &caster = (*characters)[input.casterIndex];
    if (caster.mana != input.expectedCasterMana || caster.mana < rule.mana ||
        caster.timeToRecovery != 0_ticks || !caster.CanAct() || caster.IsCursed())
        throw std::runtime_error("MM6 Power Cure caster changed after input capture");

    std::array<int, 4> nextHealth;
    for (int i = 0; i < 4; ++i) {
        const Character &target = (*characters)[i];
        if (input.maxHealth[i] <= 0 || target.health <= 0 ||
            target.health > input.maxHealth[i] || hasUnprovedCondition(target))
            throw std::runtime_error("MM6 Power Cure target state is unverified");
        nextHealth[i] = static_cast<int>(std::min<std::int64_t>(
            input.maxHealth[i], static_cast<std::int64_t>(target.health) + healAmount));
    }

    const int nextMana = caster.mana - rule.mana;
    const Duration nextRecovery = Duration::fromTicks(rule.recoveryTicks);
    (*characters)[input.casterIndex].mana = nextMana;
    (*characters)[input.casterIndex].timeToRecovery = nextRecovery;
    for (int i = 0; i < 4; ++i)
        (*characters)[i].health = nextHealth[i];
    return {nextMana, nextHealth, nextRecovery};
}

}  // namespace mm6
