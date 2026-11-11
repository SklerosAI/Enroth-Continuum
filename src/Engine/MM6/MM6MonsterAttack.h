#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <initializer_list>
#include <utility>
#include <vector>

#include "Application/GameConfig.h"

#include "Engine/AttackList.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/MM6/MM6CombatRules.h"
#include "Engine/MM6/MM6MonsterProjectile.h"
#include "Engine/MM6/MM6MonsterSpellEffect.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/ObjectTable.h"
#include "Engine/mm7_data.h"

#include "Library/Random/RandomEngine.h"

#include "Media/Audio/AudioPlayer.h"

#include "Utility/Math/TrigLut.h"

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
 * @param character                     Character to check.
 * @param enchantment                   Special enchantment.
 * @return                              Whether an unbroken item the character wears has it.
 */
inline bool wearsUnbrokenEnchantment(const Character &character, ItemEnchantment enchantment) {
    return std::ranges::any_of(character.inventory.equipment(), [&](InventoryConstEntry entry) {
        return entry->specialEnchantment == enchantment && !entry->IsBroken();
    });
}

/**
 * @param character                     Character to check.
 * @param slot                          Equipment slot.
 * @param item                          MM6 item ID.
 * @return                              Whether the slot holds that item unbroken.
 */
inline bool wearsUnbrokenItem(const Character &character, ItemSlot slot, int item) {
    InventoryConstEntry entry = character.inventory.entry(slot);
    return entry && std::to_underlying(entry->itemId) == item && !entry->IsBroken();
}

/**
 * @param character                     Character an arrow hits.
 * @return                              How many times the original halves the arrow's damage for this character. The
 *                                      Shield buff, any Shielding item, the artifact 408 in the armor slot and the
 *                                      artifact 423 in either hand halve it once each.
 */
inline int arrowHalvings(const Character &character) {
    int result = character.pCharacterBuffs[static_cast<CharacterBuff>(3)].Active(); // MM6 buffs keep their original slots, 3 is Shield.
    result += wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_SHIELDING);
    result += wearsUnbrokenItem(character, ITEM_SLOT_ARMOUR, 408);
    result += wearsUnbrokenItem(character, ITEM_SLOT_MAIN_HAND, 423) || wearsUnbrokenItem(character, ITEM_SLOT_OFF_HAND, 423);
    return result;
}

/**
 * @param character                     Character a special attack lands on.
 * @param type                          Special attack other than breaking or stealing.
 * @param party                         Party whose protection buffs add to the resistances.
 * @param enchanterBonus                Poison resistance that a hired Enchanter gives.
 * @return                              Save value the original bad-thing routine rolls with: a stat's bonus for most
 *                                      attacks, the raw Poison resistance for poison and the raw Magic resistance for
 *                                      paralysis, death, stone, eradication, aging and draining.
 */
inline int specialAttackSave(const Character &character, MonsterSpecialAttack type, const Party &party, int enchanterBonus) {
    switch (type) {
        case SPECIAL_ATTACK_CURSE:
            return character.GetParameterBonus(character.GetActualPersonality());
        case SPECIAL_ATTACK_WEAK:
        case SPECIAL_ATTACK_SLEEP:
        case SPECIAL_ATTACK_FEAR:
        case SPECIAL_ATTACK_DRUNK:
        case SPECIAL_ATTACK_DISEASE_WEAK:
        case SPECIAL_ATTACK_DISEASE_MEDIUM:
        case SPECIAL_ATTACK_DISEASE_SEVERE:
        case SPECIAL_ATTACK_UNCONSCIOUS:
            return character.GetParameterBonus(character.GetActualEndurance());
        case SPECIAL_ATTACK_INSANE:
            return character.GetParameterBonus(character.GetActualIntelligence());
        case SPECIAL_ATTACK_STEAL:
            return character.GetParameterBonus(character.GetActualAccuracy());
        case SPECIAL_ATTACK_POISON_WEAK:
        case SPECIAL_ATTACK_POISON_MEDIUM:
        case SPECIAL_ATTACK_POISON_SEVERE:
            return characterResistance(character, DAMAGE_EARTH, party, enchanterBonus);
        case SPECIAL_ATTACK_PARALYZED:
        case SPECIAL_ATTACK_DEAD:
        case SPECIAL_ATTACK_PETRIFIED:
        case SPECIAL_ATTACK_ERADICATED:
        case SPECIAL_ATTACK_AGING:
        case SPECIAL_ATTACK_MANA_DRAIN:
            return characterResistance(character, DAMAGE_MAGIC, party, 0);
        default:
            break;
    }
    throw std::runtime_error("MM6 special attack has no save of its own");
}

/**
 * @param character                     Character a breaking or stealing special attack lands on.
 * @param type                          Special attack.
 * @return                              Items the original picks from, in its order. Breaking anything takes every
 *                                      unbroken item, breaking armor the armor and any shield in hand, breaking a
 *                                      weapon the bow and any weapon in hand, and stealing every item in the backpack.
 *                                      The hands go unchecked for broken items.
 */
inline std::vector<InventoryEntry> specialAttackItems(Character &character, MonsterSpecialAttack type) {
    std::vector<InventoryEntry> result;
    auto regular = [](InventoryConstEntry entry) {
        return std::to_underlying(entry->itemId) >= 1 && std::to_underlying(entry->itemId) <= 134;
    };
    auto equipped = [&](ItemSlot slot, bool unbroken, std::initializer_list<ItemType> types) {
        InventoryEntry entry = character.inventory.entry(slot);
        if (entry && (!unbroken || !entry->IsBroken()) && std::ranges::contains(types, entry->type()))
            result.push_back(entry);
    };
    switch (type) {
        case SPECIAL_ATTACK_BREAK_ANY:
            for (InventoryEntry entry : character.inventory.entries())
                if (regular(entry) && !entry->IsBroken())
                    result.push_back(entry);
            break;
        case SPECIAL_ATTACK_BREAK_ARMOR:
            equipped(ITEM_SLOT_OFF_HAND, false, {ITEM_TYPE_SHIELD});
            equipped(ITEM_SLOT_MAIN_HAND, false, {ITEM_TYPE_SHIELD});
            equipped(ITEM_SLOT_ARMOUR, true, {ITEM_TYPE_ARMOUR});
            break;
        case SPECIAL_ATTACK_BREAK_WEAPON:
            equipped(ITEM_SLOT_OFF_HAND, false, {ITEM_TYPE_SINGLE_HANDED, ITEM_TYPE_TWO_HANDED});
            equipped(ITEM_SLOT_MAIN_HAND, false, {ITEM_TYPE_SINGLE_HANDED, ITEM_TYPE_TWO_HANDED});
            equipped(ITEM_SLOT_BOW, true, {ITEM_TYPE_BOW});
            break;
        case SPECIAL_ATTACK_STEAL:
            for (int y = 0; y < character.inventory.gridSize().h; ++y) {
                for (int x = 0; x < character.inventory.gridSize().w; ++x) {
                    InventoryEntry entry = character.inventory.entry(Pointi(x, y));
                    if (entry && entry.geometry().topLeft() == Pointi(x, y) && regular(entry))
                        result.push_back(entry);
                }
            }
            break;
        default:
            break;
    }
    return result;
}

/**
 * Applies an MM6 monster special attack to a character, as the original bad-thing routine does. Breaking and
 * stealing first pick an item and do nothing without one. Then the character escapes when a roll below the save
 * value plus the luck bonus plus 30 reaches 30. An unbroken item that wards off the effect still counts as a hit.
 *
 * @param character                     Character the attack lands on.
 * @param type                          Special attack.
 * @param rng                           Game RNG.
 * @return                              Whether the character failed to escape.
 */
inline bool receiveSpecialAttack(Character &character, MonsterSpecialAttack type, RandomEngine &rng) {
    const bool takesItem = type == SPECIAL_ATTACK_BREAK_ANY || type == SPECIAL_ATTACK_BREAK_ARMOR ||
                           type == SPECIAL_ATTACK_BREAK_WEAPON || type == SPECIAL_ATTACK_STEAL;
    InventoryEntry item;
    int save = 0;
    if (takesItem) {
        std::vector<InventoryEntry> items = specialAttackItems(character, type);
        if (items.empty())
            return false;
        item = rng.randomSample(items);
        save = type == SPECIAL_ATTACK_STEAL ? specialAttackSave(character, type, *pParty, 0)
                                            : 3 * (std::to_underlying(pItemTable->items[item->itemId].rarity) + item->GetDamageMod());
    } else if (type != SPECIAL_ATTACK_NONE) {
        save = specialAttackSave(character, type, *pParty, CheckHiredNPCSpeciality(NPC_PROFESSION_ENCHANTER) ? 20 : 0);
    }
    if (rng.random(save + character.GetParameterBonus(character.GetActualLuck()) + 30) >= 30)
        return false;

    const Time now = pParty->GetPlayingTime();
    auto inflict = [&](Condition condition) {
        if (!character.conditions.has(condition))
            character.conditions.set(condition, now);
    };
    switch (type) {
        case SPECIAL_ATTACK_CURSE: inflict(CONDITION_CURSED); break;
        case SPECIAL_ATTACK_WEAK: inflict(CONDITION_WEAK); break;
        case SPECIAL_ATTACK_FEAR: inflict(CONDITION_FEAR); break;
        case SPECIAL_ATTACK_DRUNK: inflict(CONDITION_DRUNK); break;
        case SPECIAL_ATTACK_SLEEP:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_ALARMS))
                inflict(CONDITION_SLEEP);
            break;
        case SPECIAL_ATTACK_INSANE:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_SANITY))
                inflict(CONDITION_INSANE);
            break;
        case SPECIAL_ATTACK_POISON_WEAK:
        case SPECIAL_ATTACK_POISON_MEDIUM:
        case SPECIAL_ATTACK_POISON_SEVERE:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_ANTIDOTES) && !wearsUnbrokenItem(character, ITEM_SLOT_CLOAK, 410))
                inflict(type == SPECIAL_ATTACK_POISON_WEAK ? CONDITION_POISON_WEAK :
                        type == SPECIAL_ATTACK_POISON_MEDIUM ? CONDITION_POISON_MEDIUM : CONDITION_POISON_SEVERE);
            break;
        case SPECIAL_ATTACK_DISEASE_WEAK:
        case SPECIAL_ATTACK_DISEASE_MEDIUM:
        case SPECIAL_ATTACK_DISEASE_SEVERE:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_IMMUNITY))
                inflict(type == SPECIAL_ATTACK_DISEASE_WEAK ? CONDITION_DISEASE_WEAK :
                        type == SPECIAL_ATTACK_DISEASE_MEDIUM ? CONDITION_DISEASE_MEDIUM : CONDITION_DISEASE_SEVERE);
            break;
        case SPECIAL_ATTACK_PARALYZED:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_FREEDOM))
                inflict(CONDITION_PARALYZED);
            break;
        case SPECIAL_ATTACK_UNCONSCIOUS:
            inflict(CONDITION_UNCONSCIOUS);
            character.health = std::min(character.health, 0);
            break;
        case SPECIAL_ATTACK_DEAD:
            inflict(CONDITION_DEAD);
            character.health = std::min(character.health, 0);
            character.mana = std::min(character.mana, 0);
            break;
        case SPECIAL_ATTACK_PETRIFIED:
            if (!wearsUnbrokenEnchantment(character, ITEM_ENCHANTMENT_OF_MEDUSA) && !wearsUnbrokenItem(character, ITEM_SLOT_MAIN_HAND, 423) &&
                !wearsUnbrokenItem(character, ITEM_SLOT_OFF_HAND, 423))
                character.conditions.set(CONDITION_PETRIFIED, now);
            break;
        case SPECIAL_ATTACK_ERADICATED:
            if (!character.conditions.has(CONDITION_UNCONSCIOUS)) // The original tests Unconscious, not Eradicated, here.
                character.conditions.set(CONDITION_ERADICATED, now);
            character.health = std::min(character.health, 0);
            character.mana = std::min(character.mana, 0);
            break;
        case SPECIAL_ATTACK_BREAK_ANY:
        case SPECIAL_ATTACK_BREAK_ARMOR:
        case SPECIAL_ATTACK_BREAK_WEAPON:
            breakItem(&character, item.get());
            break;
        case SPECIAL_ATTACK_STEAL:
            character.inventory.take(item); // The thief keeps nothing.
            break;
        case SPECIAL_ATTACK_AGING:
            character.sAgeModifier++;
            break;
        case SPECIAL_ATTACK_MANA_DRAIN:
            character.mana = 0;
            break;
        default:
            break;
    }
    return true;
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
    requireVerifiedMonsterHit(actor, ability, pParty->pCharacters);
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

    if (impact == MonsterObjectImpact::Missile) {
        applySpellSpriteDamage(spriteIndex, target);
    } else if (impact == MonsterObjectImpact::FingerOfDeath) {
        if (target.type() == OBJECT_Character && grng->random(100) < object.spell_level) {
            Actor &actor = pActors[object.spell_caster_pid.id()];
            receiveSpecialAttack(pParty->pCharacters[stru_50C198.which_player_to_attack(&actor)], SPECIAL_ATTACK_DEAD, *grng);
        }
    } else if (impact != MonsterObjectImpact::Explosion && impact != MonsterObjectImpact::Meteor) {
        throw std::runtime_error("MM6 monster object impact is not verified");
    }
    object.spriteId = monsterObjectExplosion(object.spriteId);
    object.uObjectDescID = pObjectTable->ObjectIDByItemID(object.spriteId);
    if (object.uObjectDescID == 0)
        SpriteObject::Remove(spriteIndex);
    object.spellSpriteStop();
    if (impact == MonsterObjectImpact::Explosion || impact == MonsterObjectImpact::Meteor)
        pushAoeAttack(Pid(OBJECT_Sprite, spriteIndex), engine->config->gameplay.AoeDamageDistance.value(), object.vPosition,
                      object.spellCasterAbility);
    if (object.uSpellID != SPELL_NONE)
        pAudioPlayer->playSpellSound(object.uSpellID, true, SOUND_MODE_PID, Pid(OBJECT_Sprite, spriteIndex));
    return false;
}

/**
 * Casts a monster spell as the original monster cast does. Projectile spells launch their object from half the
 * monster's height. Meteor Shower drops its meteors from above the party, and only outdoors. Dispel Magic ends every
 * party buff and the buffs of each character who fails a save. Mass Curse tries to curse each character who can act.
 *
 * @param actorId                       Casting monster.
 * @param direction                     Direction to its target.
 * @param spell                         Verified monster spell.
 * @param skill                         Skill from the monster's spell cell.
 */
inline void castMonsterSpell(int actorId, const AIDirection &direction, SpellId spell, CombinedSkillValue skill) {
    Actor &actor = pActors[actorId];
    SpriteObject sprite;
    sprite.containing_item.Reset();
    sprite.uSpellID = spell;
    sprite.spell_level = monsterSkillByte(skill);
    sprite.spell_skill = MASTERY_NONE;
    sprite.spell_caster_pid = Pid(OBJECT_Actor, actorId);
    sprite.spellCasterAbility = ABILITY_SPELL1;
    sprite.field_60_distance_related_prolly_lod = direction.uDistance < 1024 ? 1 : direction.uDistance < 2560 ? 2 : 3;
    if (direction.uDistance < meleeRange)
        sprite.field_60_distance_related_prolly_lod = 0;

    switch (static_cast<int>(spell)) {
        case 9: {
            if (uCurrentlyLoadedLevelType == LEVEL_INDOOR)
                return;
            sprite.spriteId = monsterSpellObject(spell);
            sprite.uObjectDescID = pObjectTable->ObjectIDByItemID(sprite.spriteId);
            int dx = 0;
            int dy = 0;
            for (int i = 0; i < monsterMeteorCount(skill); i++) {
                const int height = grng->random(1000);
                const float dz = height - 2500.0f;
                int yaw = 0;
                int pitch = 0;
                if (std::sqrt(dx * dx + dy * dy + dz * dz) > 1.0f) {
                    yaw = TrigLUT.atan2(dx, dy);
                    pitch = TrigLUT.atan2(static_cast<int>(std::sqrt(static_cast<float>(dx * dx + dy * dy))), static_cast<int>(dz));
                }
                sprite.vPosition = Vec3f(pParty->pos.x, pParty->pos.y, pParty->pos.z + 2500 + height);
                sprite.uFacing = yaw;
                const int spriteId = sprite.Create(yaw, pitch, pObjectTable->pObjects[sprite.uObjectDescID].uSpeed, 0);
                if (spriteId != -1)
                    pAudioPlayer->playSpellSound(spell, false, SOUND_MODE_PID, Pid(OBJECT_Sprite, spriteId));
                dx = grng->random(1024) - 512;
                dy = grng->random(1024) - 512;
            }
            return;
        }
        case 80:
            for (int i = 0; i < 13; i++)
                pParty->pPartyBuffs[static_cast<PartyBuff>(i)].Reset(); // MM6 buffs keep their original slots.
            for (Character &character : pParty->pCharacters) {
                const int luck = character.GetParameterBonus(character.GetActualLuck());
                if (grng->random(30 + luck + characterResistance(character, DAMAGE_MAGIC, *pParty, 0)) >= 30)
                    continue;
                for (int i = 0; i < 12; i++)
                    character.pCharacterBuffs[static_cast<CharacterBuff>(i)].Reset();
            }
            pAudioPlayer->playSpellSound(spell, false, SOUND_MODE_PID, Pid(OBJECT_Actor, actorId));
            return;
        case 91:
            for (Character &character : pParty->pCharacters)
                if (character.conditions.hasNone({CONDITION_SLEEP, CONDITION_PARALYZED, CONDITION_UNCONSCIOUS, CONDITION_DEAD,
                                                  CONDITION_PETRIFIED, CONDITION_ERADICATED}))
                    receiveSpecialAttack(character, SPECIAL_ATTACK_CURSE, *grng);
            pAudioPlayer->playSpellSound(spell, false, SOUND_MODE_PID, Pid(OBJECT_Actor, actorId));
            return;
        default:
            break;
    }

    sprite.spriteId = monsterSpellObject(spell);
    sprite.uObjectDescID = pObjectTable->ObjectIDByItemID(sprite.spriteId);
    sprite.vPosition = actor.pos + Vec3f(0, 0, actor.height / 2);
    sprite.uFacing = direction.uYawAngle;
    sprite.uSectorID = pIndoor->GetSector(sprite.vPosition);
    const int spriteId = sprite.Create(direction.uYawAngle, direction.uPitchAngle, pObjectTable->pObjects[sprite.uObjectDescID].uSpeed, 0);
    if (spriteId != -1)
        pAudioPlayer->playSpellSound(spell, false, SOUND_MODE_PID, Pid(OBJECT_Sprite, spriteId));
}

}  // namespace mm6
