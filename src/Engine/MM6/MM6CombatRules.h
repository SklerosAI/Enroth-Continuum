#pragma once

#include <algorithm>
#include <span>
#include <stdexcept>

#include "Engine/MM6/MM6MonsterDamage.h"
#include "Engine/MM6/MM6MonsterSpecialAttack.h"
#include "Engine/MM6/MM6SpellEffectGuard.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/SpriteObject.h"

#include "Library/Random/RandomEngine.h"

#include "Media/Audio/AudioPlayer.h"

namespace mm6 {

/**
 * Refuses an MM6 party attack that could reach an unverified spell or damage channel anywhere in
 * `Actor::DamageMonsterFromParty`, before that function touches the monster, the party or the RNG.
 *
 * @param character                     Attacking character.
 * @param projectile                    Projectile that hit the monster, or `nullptr` for a melee attack.
 */
inline void requireVerifiedPartyAttack(const Character &character, const SpriteObject *projectile) {
    auto requireVerifiedAdditionalDamage = [](const Item &item) {
        if (item.specialEnchantment != ITEM_ENCHANTMENT_NULL && item.specialEnchantment != ITEM_ENCHANTMENT_ANTIQUE)
            throw std::runtime_error("MM6 weapon enchantment damage is not verified");
    };

    if (projectile) {
        if (projectile->uSpellID == SPELL_BOW_ARROW) {
            requireVerifiedAdditionalDamage(projectile->containing_item);
        } else if (projectile->uSpellID != SPELL_BLASTER_PROJECTILE && projectile->uSpellID != SPELL_101) {
            requireVerifiedSpellEffect(GameTitle::MM6, projectile->uSpellID, SpellEffectSource::Damage);
        }
        return;
    }

    if (character.IsUnarmed() && character.pCharacterBuffs[CHARACTER_BUFF_HAMMERHANDS].Active())
        requireVerifiedMonsterDamage(GameTitle::MM6, DAMAGE_BODY);
    for (ItemSlot slot : {ITEM_SLOT_OFF_HAND, ITEM_SLOT_MAIN_HAND})
        if (InventoryConstEntry item = character.inventory.functionalEntry(slot))
            requireVerifiedAdditionalDamage(*item);

    bool canStun = character.GetSpecialItemBonus(ITEM_ENCHANTMENT_OF_FORCE);
    if (InventoryConstEntry mainHand = character.inventory.functionalEntry(ITEM_SLOT_MAIN_HAND)) {
        const Skill skill = mainHand->skill();
        canStun |= (skill == SKILL_STAFF || skill == SKILL_MACE) && character.getSkillValue(skill).mastery() >= MASTERY_MASTER;
    }
    if (canStun)
        requireVerifiedMonsterDamage(GameTitle::MM6, DAMAGE_EARTH); // Stun and paralysis check the target's Earth immunity.
}

/**
 * Refuses an MM6 monster spell whose effect is not verified.
 *
 * @param monster                       Attacking monster.
 * @param ability                       Which of its abilities hit.
 */
inline void requireVerifiedMonsterSpellAbility(const Actor &monster, ActorAbility ability) {
    if (ability == ABILITY_SPELL1)
        requireVerifiedSpellEffect(GameTitle::MM6, monster.monsterInfo.spell1Id, SpellEffectSource::Monster);
    if (ability == ABILITY_SPELL2)
        requireVerifiedSpellEffect(GameTitle::MM6, monster.monsterInfo.spell2Id, SpellEffectSource::Monster);
}

/**
 * Refuses an MM6 monster hit on a character that could reach an unverified spell, damage channel or special attack
 * anywhere in `DamageCharacterFromMonster`, before that function touches the party, the monster or the RNG.
 *
 * @param monster                       Attacking monster.
 * @param ability                       Which of its abilities hit.
 * @param canApplySpecialAttack         Whether this hit rolls for the monster's special attack.
 * @param party                         Party characters, one of them takes the hit.
 */
inline void requireVerifiedMonsterHit(const Actor &monster, ActorAbility ability, bool canApplySpecialAttack,
                                      std::span<const Character> party) {
    requireVerifiedMonsterSpellAbility(monster, ability);
    const DamageType type = monster.abilityDamageType(ability);
    requireVerifiedCharacterDamage(GameTitle::MM6, type);
    if (std::ranges::any_of(party, [](const Character &character) {
            return character.pCharacterBuffs[CHARACTER_BUFF_PAIN_REFLECTION].Active();
        }))
        requireVerifiedMonsterDamage(GameTitle::MM6, type); // Pain reflection sends the damage back to the monster.
    if (canApplySpecialAttack)
        requireVerifiedMonsterSpecialAttack(GameTitle::MM6, monster.monsterInfo.specialAttackType);
}

/**
 * Refuses an MM6 monster hit on another monster that could reach an unverified spell or damage channel, before
 * `Actor::ActorDamageFromMonster` touches either monster or the RNG.
 *
 * @param attacker                      Attacking monster.
 * @param ability                       Which of its abilities hit.
 */
inline void requireVerifiedMonsterOnMonsterHit(const Actor &attacker, ActorAbility ability) {
    requireVerifiedMonsterSpellAbility(attacker, ability);
    requireVerifiedMonsterDamage(GameTitle::MM6, attacker.abilityDamageType(ability));
}

/**
 * MM6 rule for damage that reaches a character, from the original character damage routine. Physical damage passes
 * unchanged. An elemental hit is halved up to four times while a roll against luck bonus plus resistance plus 30
 * reaches 30. Unlike MM7, the roll runs even when luck and resistance add up to nothing.
 *
 * @param character                     Character taking the hit.
 * @param type                          Damage channel.
 * @param damage                        Incoming damage.
 * @param rng                           Game RNG.
 * @return                              Damage after resistance.
 */
inline int characterIncomingDamage(const Character &character, DamageType type, int damage, RandomEngine &rng) {
    requireVerifiedCharacterDamage(GameTitle::MM6, type);
    if (type == DAMAGE_PHYSICAL)
        return damage;

    const int divider = character.GetParameterBonus(character.GetActualLuck()) +
                        character.GetActualResistance(characterResistanceForDamage(type)) + 30;
    for (int i = 0; i < 4 && divider > 0; i++) {
        if (rng.random(divider) < 30)
            break;
        damage /= 2;
    }
    return damage;
}

/**
 * Breaks an item for a successful MM6 break-item special attack. MM6 has no hardened items. The original portrait
 * animation for this effect is not mapped, the shared one has no MM6 icon frames.
 *
 * @param character                     Character whose item breaks.
 * @param item                          Item to break.
 */
inline void breakItem(Character *character, Item *item) {
    character->playReaction(SPEECH_ITEM_BROKEN);
    item->SetBroken();
    pAudioPlayer->playUISound(SOUND_metal_vs_metal03h);
}

}  // namespace mm6
