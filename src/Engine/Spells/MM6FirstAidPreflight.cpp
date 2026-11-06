#include "Engine/Spells/MM6FirstAidPreflight.h"

#include <stdexcept>

#include "Engine/Spells/MM6FirstAidRules.h"
#include "Engine/Spells/MM6SpellRules.h"

namespace mm6 {

FirstAidPreflightPlan planFirstAidSpellbookCast(const FirstAidPreflightInput &input) {
    if (input.title != GameTitle::MM6 || input.spell != SPELL_BODY_FIRST_AID)
        throw std::runtime_error("First Aid preflight is MM6-only");
    if (input.route != FirstAidCastRoute::Spellbook)
        throw std::runtime_error("MM6 First Aid cast route is unverified");
    if (input.casterIndex < 0 || input.casterIndex >= 4 ||
        input.targetCharacterIndex < -1 || input.targetCharacterIndex >= 4 ||
        input.availableMana < 0 || input.skillLevel <= 0)
        throw std::runtime_error("MM6 First Aid preflight has invalid cast input");
    if (!input.spellKnown || !input.casterCanAct || input.casterInRecovery ||
        input.casterCursed || input.partyInvisible || input.hasHeldItem ||
        input.turnBasedMode || input.underwater)
        throw std::runtime_error("MM6 First Aid cast context has unverified side effects");

    const SpellManaRecovery rule = spellManaRecovery(input.spell, input.mastery);
    const int healAmount = firstAidHealAmount(input.spell, input.mastery);

    FirstAidPreflightPlan plan{FirstAidPreflightState::NeedsCharacterTarget,
                               input.casterIndex, input.targetCharacterIndex,
                               rule.mana, rule.recoveryTicks, healAmount};
    if (input.availableMana < rule.mana) {
        plan.state = FirstAidPreflightState::InsufficientMana;
    } else if (input.targetCharacterIndex != -1) {
        if (!input.targetCanReceiveHealing)
            throw std::runtime_error("MM6 First Aid target condition is unverified");
        plan.state = FirstAidPreflightState::InputsReady;
    }
    return plan;
}

} // namespace mm6
