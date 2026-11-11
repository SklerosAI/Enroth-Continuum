#pragma once

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "Application/GameConfig.h"

#include "Engine/AttackList.h"
#include "Engine/Engine.h"
#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/MM6/MM6MonsterProjectile.h"
#include "Engine/MM6/MM6MonsterSpellEffect.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ObjectTable.h"
#include "Engine/mm7_data.h"

#include "Library/Random/RandomEngine.h"

#include "Media/Audio/AudioPlayer.h"

namespace mm6 {

/**
 * Damage of one monster hit before the target's resistance, from the original monster damage routine.
 *
 * @param monster                       Hitting monster.
 * @param ability                       Which of its abilities hit.
 * @param rng                           Game RNG.
 * @return                              Damage. Every ability adds the first attack's bonus, the second attack too.
 */
inline int monsterAttackDamage(const MonsterInfo &monster, ActorAbility ability, RandomEngine &rng) {
    switch (ability) {
        case ABILITY_ATTACK1:
            return rng.randomDice(monster.attack1DamageDiceRolls, monster.attack1DamageDiceSides) + monster.attack1DamageBonus;
        case ABILITY_ATTACK2:
            return rng.randomDice(monster.attack2DamageDiceRolls, monster.attack2DamageDiceSides) + monster.attack1DamageBonus;
        case ABILITY_SPELL1:
            return monsterSpellDamage(monster, monster.spell1Id, monster.spell1SkillMastery, rng);
        default:
            return 0;
    }
}

/**
 * @param character                     Character an arrow hits.
 * @return                              How many times the original halves the arrow's damage for this character. The
 *                                      Shield buff, any Shielding item, the artifact 408 in the armor slot and the
 *                                      artifact 423 in either hand halve it once each.
 */
inline int arrowHalvings(const Character &character) {
    auto wears = [&](ItemSlot slot, int item) {
        InventoryConstEntry entry = character.inventory.entry(slot);
        return entry && std::to_underlying(entry->itemId) == item && !entry->IsBroken();
    };
    auto shielding = [](InventoryConstEntry entry) {
        return entry->specialEnchantment == ITEM_ENCHANTMENT_OF_SHIELDING && !entry->IsBroken();
    };
    int result = character.pCharacterBuffs[static_cast<CharacterBuff>(3)].Active(); // MM6 buffs keep their original slots, 3 is Shield.
    result += std::ranges::any_of(character.inventory.equipment(), shielding);
    result += wears(ITEM_SLOT_ARMOUR, 408);
    result += wears(ITEM_SLOT_MAIN_HAND, 423) || wears(ITEM_SLOT_OFF_HAND, 423);
    return result;
}

/**
 * Applies a hit by an object that a monster launched to a character, as the original character hit routine does.
 * Only arrows and the robot laser roll to hit and halve for shields. Only the first attack rolls for the monster's
 * special attack.
 *
 * @param object                        Monster object that hit.
 * @param targetCharacter               Character index, or -1 to let the monster pick one.
 */
inline void damageCharacterFromMonsterObject(const SpriteObject &object, int targetCharacter) {
    Actor &actor = pActors[object.spell_caster_pid.id()];
    const ActorAbility ability = object.spellCasterAbility;
    requireVerifiedMonsterHit(actor, ability, ability == ABILITY_ATTACK1, pParty->pCharacters);
    if (targetCharacter == -1)
        targetCharacter = stru_50C198.which_player_to_attack(&actor);
    Character &character = pParty->pCharacters[targetCharacter];

    int damage = monsterAttackDamage(actor.monsterInfo, ability, *grng);
    if (std::to_underlying(object.spriteId) == 500 || std::to_underlying(object.spriteId) == 590) {
        if (actor.buffs[static_cast<ActorBuff>(2)].Active() && grng->random(100) < 50)
            return; // MM6 monster buffs keep their original slots, 2 is Curse.
        if (!actor.ActorHitOrMiss(&character))
            return;
        damage >>= arrowHalvings(character);
    }
    if (actor.buffs[ACTOR_BUFF_SHRINK].Active() && actor.buffs[ACTOR_BUFF_SHRINK].power > 0)
        damage /= actor.buffs[ACTOR_BUFF_SHRINK].power;

    character.receiveDamage(damage, actor.abilityDamageType(ability));

    const MonsterInfo &monster = actor.monsterInfo;
    if (ability == ABILITY_ATTACK1 && !engine->config->debug.NoDamage.value() && monster.specialAttackType != SPECIAL_ATTACK_NONE &&
        grng->random(100) < monster.level * monster.specialAttackLevel)
        character.ReceiveSpecialAttackEffect(monster.specialAttackType, &actor);

    if (!pParty->bTurnBasedModeOn) {
        const int bonus = character.GetParameterBonus(character.GetActualEndurance());
        character.SetRecoveryTime(Duration::fromTicks((20 - bonus) * debug_non_combat_recovery_mul * flt_debugrecmod3));
    }
}

/**
 * Handles an object that a monster launched hitting something, as the original object collision routine does.
 *
 * @param spriteIndex                   Index of the object in `pSpriteObjects`.
 * @param target                        What it hit.
 * @return                              Whether the object flies on.
 */
inline bool processMonsterObjectImpact(int spriteIndex, Pid target) {
    if (target.type() == OBJECT_Actor)
        return true; // The original lets every monster object fly through monsters.

    SpriteObject &object = pSpriteObjects[spriteIndex];
    const MonsterObjectImpact impact = monsterObjectImpact(object.spriteId);
    if (impact == MonsterObjectImpact::Arrow) {
        object.spellSpriteStop();
        applySpellSpriteDamage(spriteIndex, target);
        SpriteObject::Remove(spriteIndex);
        return false;
    }

    if (impact == MonsterObjectImpact::Missile)
        applySpellSpriteDamage(spriteIndex, target);
    else if (impact != MonsterObjectImpact::Explosion && impact != MonsterObjectImpact::Meteor)
        throw std::runtime_error("MM6 monster object impact is not verified");
    object.spriteId = monsterObjectExplosion(object.spriteId);
    object.uObjectDescID = pObjectTable->ObjectIDByItemID(object.spriteId);
    if (object.uObjectDescID == 0)
        SpriteObject::Remove(spriteIndex);
    object.spellSpriteStop();
    if (impact != MonsterObjectImpact::Missile)
        pushAoeAttack(Pid(OBJECT_Sprite, spriteIndex), engine->config->gameplay.AoeDamageDistance.value(), object.vPosition,
                      object.spellCasterAbility);
    if (object.uSpellID != SPELL_NONE)
        pAudioPlayer->playSpellSound(object.uSpellID, true, SOUND_MODE_PID, Pid(OBJECT_Sprite, spriteIndex));
    return false;
}

}  // namespace mm6
