#include "Engine/Spells/MM6FirstAidTransaction.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace mm6 {

FirstAidStateChange stageFirstAidOnCharacters(GameTitle title,
                                               const FirstAidPreflightPlan &plan,
                                               std::array<Character, 4> *characters,
                                               int targetMaxHealth) {
    if (title != GameTitle::MM6 || !characters || plan.state != FirstAidPreflightState::InputsReady ||
        plan.casterIndex < 0 || plan.casterIndex >= 4 ||
        plan.targetCharacterIndex < 0 || plan.targetCharacterIndex >= 4 ||
        plan.manaCost < 0 || plan.healAmount <= 0 || plan.recoveryTicks <= 0 ||
        targetMaxHealth <= 0)
        throw std::runtime_error("Invalid MM6 First Aid transaction");

    const Character &caster = (*characters)[plan.casterIndex];
    const Character &target = (*characters)[plan.targetCharacterIndex];
    if (!caster.CanAct() || caster.IsCursed() || caster.timeToRecovery > 0_ticks ||
        caster.mana < plan.manaCost || target.health <= 0 ||
        target.health > targetMaxHealth || target.IsDead() ||
        target.IsEradicated() || target.IsUnconcious() || target.IsZombie())
        throw std::runtime_error("MM6 First Aid state changed after preflight");

    const int newMana = caster.mana - plan.manaCost;
    const int newHealth = static_cast<int>(std::min<int64_t>(
        targetMaxHealth, static_cast<int64_t>(target.health) + plan.healAmount));
    const Duration recovery = Duration::fromTicks(plan.recoveryTicks);
    (*characters)[plan.casterIndex].mana = newMana;
    (*characters)[plan.targetCharacterIndex].health = newHealth;
    (*characters)[plan.casterIndex].timeToRecovery = recovery;
    return {newMana, newHealth, recovery};
}

}  // namespace mm6
