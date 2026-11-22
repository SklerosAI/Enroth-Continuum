#pragma once

#include <algorithm>
#include <utility>

#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/Item.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

/**
 * Elemental extra damage of an MM6 relic hit. MM6.exe takes it off the monster's hit points as it is. The GrayFace
 * patch sends it through the monster's resistance roll first, and a stoned monster takes none.
 *
 * @param monster                       Monster that was hit.
 * @param type                          Damage channel of the extra damage.
 * @param damage                        Extra damage before resistance.
 * @param resisted                      Whether to apply the GrayFace rule.
 * @param rng                           Game RNG.
 * @return                              Hit points the monster loses on top of the hit.
 */
inline int artifactExtraDamage(const Actor &monster, DamageType type, int damage, bool resisted, RandomEngine &rng) {
    if (!resisted)
        return damage;
    if (monster.buffs[ACTOR_BUFF_STONED].Active())
        return 0;
    return monsterIncomingDamage(monster.monsterInfo, type, damage, rng);
}

/**
 * Applies what the MM6 artifacts and relics in a character's hands do when a melee hit lands, as MM6.exe's melee hit
 * routine does for each hand, off hand first. Mordred heals the wielder by a fifth of the hit while the monster
 * still stands. Hades adds 20 Poison and Ares 30 Fire damage.
 *
 * @offset 0x430FB4
 *
 * @param character                     Attacking character, Mordred heals it.
 * @param monster                       Monster that was hit, its hit points don't change here.
 * @param hitDamage                     Damage of the hit itself, after the monster's resistance.
 * @param resisted                      Whether relic damage goes through the monster's resistance, see
 *                                      `artifactExtraDamage`.
 * @param rng                           Game RNG.
 * @return                              Extra damage the monster takes on top of the hit.
 */
inline int artifactMeleeHit(Character &character, const Actor &monster, int hitDamage, bool resisted, RandomEngine &rng) {
    int extra = 0;
    for (ItemSlot slot : {ITEM_SLOT_OFF_HAND, ITEM_SLOT_MAIN_HAND}) {
        InventoryConstEntry weapon = character.inventory.functionalEntry(slot);
        if (!weapon)
            continue;
        switch (std::to_underlying(weapon->itemId)) {
            case 400: // Mordred.
                if (monster.hp - hitDamage - extra > 0)
                    character.health = std::min(character.health + hitDamage / 5, character.GetMaxHealth());
                break;
            case 415: // Hades.
                extra += artifactExtraDamage(monster, DAMAGE_EARTH, 20, resisted, rng);
                break;
            case 416: // Ares.
                extra += artifactExtraDamage(monster, DAMAGE_FIRE, 30, resisted, rng);
                break;
            default:
                break;
        }
    }
    return extra;
}

/**
 * @offset 0x431615
 *
 * @param bow                           Bow that shot the arrow.
 * @param monster                       Monster the arrow hit.
 * @param resisted                      Whether relic damage goes through the monster's resistance, see
 *                                      `artifactExtraDamage`.
 * @param rng                           Game RNG.
 * @return                              Extra damage the monster takes on top of the arrow, 20 Electricity for Artemis.
 */
inline int artifactArrowHit(const Item &bow, const Actor &monster, bool resisted, RandomEngine &rng) {
    if (std::to_underlying(bow.itemId) != 420) // Artemis.
        return 0;
    return artifactExtraDamage(monster, DAMAGE_AIR, 20, resisted, rng);
}

} // namespace mm6
