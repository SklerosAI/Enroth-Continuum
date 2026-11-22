#pragma once

#include <stdexcept>
#include <utility>

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/Objects/Character.h"

namespace mm6 {

/**
 * Skill bonus to a character attribute, as MM6.exe's skill bonus routine (0x483930) computes it. Weapon terms take the
 * points of `weaponSkillHirelingBonus` as extra skill levels, armor class terms those of `armorSkillHirelingBonus`.
 * MM6.exe multiplies an Expert blaster's attack by 1 where `skilldes.txt` promises double, `gameplay.fix_expert_blaster`
 * makes it 2.
 *
 * @param character                     Character whose skills and equipment give the bonus.
 * @param attribute                     Attribute to get the bonus for.
 * @return                              The bonus, 0 for an attribute that no MM6 skill adds to.
 */
inline int skillBonus(const Character &character, Attribute attribute) {
    auto level = [&](Skill skill) { return character.getActualSkillValue(skill).level(); };
    auto mastery = [&](Skill skill) { return character.getActualSkillValue(skill).mastery(); };
    auto shieldFactor = [](Mastery mastery) { return mastery == MASTERY_MASTER ? 3 : mastery == MASTERY_EXPERT ? 2 : 1; };
    auto blasterFactor = [](Mastery mastery) {
        if (mastery == MASTERY_MASTER)
            return 3;
        if (mastery == MASTERY_EXPERT && engine->config->gameplay.FixExpertBlaster.value())
            return 2;
        return 1;
    };

    switch (attribute) {
        case ATTRIBUTE_HEALTH:
        case ATTRIBUTE_MANA: {
            const int classId = std::to_underlying(character.classType);
            if (!isValidClass(classId))
                throw std::runtime_error("Invalid MM6 class ID for a Bodybuilding or Meditation bonus");
            if (attribute == ATTRIBUTE_HEALTH)
                return creationRules().hpFactor[classId] * character.GetBodybuilding();
            return creationRules().spFactor[classId] * character.GetMeditation();
        }

        case ATTRIBUTE_AC_BONUS: {
            const int points = armorSkillHirelingBonus();
            int result = 0;
            for (InventoryConstEntry item : character.inventory.functionalEquipment()) {
                const Skill skill = item->skill();
                switch (skill) {
                    case SKILL_STAFF:
                    case SKILL_SPEAR:
                        if (mastery(skill) >= MASTERY_EXPERT)
                            result += level(skill) + points;
                        break;
                    case SKILL_SHIELD:
                        result += (level(skill) + points) * shieldFactor(mastery(skill));
                        break;
                    case SKILL_LEATHER:
                    case SKILL_CHAIN:
                    case SKILL_PLATE:
                        result += level(skill) + points;
                        break;
                    default:
                        break;
                }
            }
            return result;
        }

        case ATTRIBUTE_ATTACK: // The first melee weapon with a weapon skill counts, the off hand comes first.
            for (InventoryConstEntry item : character.inventory.functionalEquipment()) {
                const Skill skill = item->skill();
                if (!item->isMeleeWeapon())
                    continue;
                switch (skill) {
                    case SKILL_STAFF:
                    case SKILL_SWORD:
                    case SKILL_DAGGER:
                    case SKILL_AXE:
                    case SKILL_SPEAR:
                    case SKILL_MACE:
                        return level(skill) + weaponSkillHirelingBonus();
                    case SKILL_BLASTER:
                        return (level(skill) + weaponSkillHirelingBonus()) * blasterFactor(mastery(skill));
                    default:
                        break;
                }
            }
            return 0;

        case ATTRIBUTE_MELEE_DMG_BONUS: // The first axe, spear or mace counts, other weapons are passed over.
            for (InventoryConstEntry item : character.inventory.functionalEquipment()) {
                const Skill skill = item->skill();
                if (!item->isMeleeWeapon())
                    continue;
                if (skill == SKILL_AXE || skill == SKILL_SPEAR)
                    return mastery(skill) == MASTERY_MASTER ? level(skill) + weaponSkillHirelingBonus() : 0;
                if (skill == SKILL_MACE)
                    return mastery(skill) >= MASTERY_EXPERT ? level(skill) + weaponSkillHirelingBonus() : 0;
            }
            return 0;

        case ATTRIBUTE_RANGED_ATTACK:
            for (InventoryConstEntry item : character.inventory.functionalEquipment()) {
                const Skill skill = item->skill();
                if (skill == SKILL_BOW)
                    return level(skill) + weaponSkillHirelingBonus();
                if (skill == SKILL_BLASTER)
                    return level(skill) * blasterFactor(mastery(skill)) + weaponSkillHirelingBonus();
            }
            return 0;

        default:
            return 0;
    }
}

} // namespace mm6
