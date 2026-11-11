#pragma once

#include <cassert>
#include <optional>
#include <stdexcept>
#include <utility>

#include "Engine/Graphics/Indoor.h"
#include "Engine/MM6/MM6SpellDamageTag.h"
#include "Engine/MM6/MM6SpellEffectGuard.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Spells/Spells.h"
#include "Engine/Tables/ObjectTable.h"

#include "Library/Random/RandomEngine.h"

#include "Media/Audio/AudioPlayer.h"

#include "Utility/Math/TrigLut.h"

namespace mm6 {

/**
 * Damage of one hit by an MM6 event spell on a character, from the original's event spell damage table. Only the low
 * six bits of the event's skill level count there, so a level of 100 hits as 36.
 *
 * @param spell                         Event spell other than Armageddon, see `isVerifiedEventSpell`.
 * @param skillLevel                    Skill level from the event.
 * @param rng                           Game RNG.
 * @return                              Damage before the character's resistance.
 */
inline int eventSpellDamage(SpellId spell, int skillLevel, RandomEngine &rng) {
    const int level = skillLevel & 0x3F;
    switch (static_cast<int>(spell)) {
        case 6: return rng.randomDice(level, 6);
        case 15: return level + 2;
        case 28: return rng.randomDice(level, 7);
        case 32: return 12 + rng.randomDice(level, 2);
        case 90: return 25 + rng.randomDice(level, 10);
        default: break;
    }
    throw std::runtime_error("MM6 event spell damage is not verified");
}

/**
 * @param tag                           Damage tag of the spell in MM6 `spells.txt`.
 * @return                              Damage channel that the original passes to the character damage routine for it.
 *                                      Labels other than the five elements, Phys and none among them, deal Physical.
 */
inline DamageType spellDamageType(SpellDamageTag tag) {
    switch (tag) {
        case SpellDamageTag::Fire: return DAMAGE_FIRE;
        case SpellDamageTag::Electricity: return DAMAGE_AIR;
        case SpellDamageTag::Cold: return DAMAGE_WATER;
        case SpellDamageTag::Poison: return DAMAGE_EARTH; // The shared runtime keeps MM6 Poison in the Earth channel.
        case SpellDamageTag::Magic: return DAMAGE_MAGIC;
        case SpellDamageTag::None:
        case SpellDamageTag::Physical: return DAMAGE_PHYSICAL;
    }
    throw std::runtime_error("Invalid MM6 spell damage tag");
}

/**
 * Launches an MM6 event spell the way the original event cast does. Sparks spreads three, five or seven sparks over 60
 * degrees by mastery, the other projectiles fly alone, and Armageddon does nothing.
 *
 * @param spell                         Verified event spell, see `isVerifiedEventSpell`.
 * @param mastery                       Mastery from the event.
 * @param skillLevel                    Skill level from the event.
 * @param from                          Launch point.
 * @param yaw                           Launch yaw toward the target.
 * @param pitch                         Launch pitch toward the target.
 * @param distance                      Distance to the target.
 */
inline void launchEventSpell(SpellId spell, Mastery mastery, int skillLevel, Vec3f from, int yaw, int pitch, float distance) {
    assert(isVerifiedEventSpell(spell));
    if (static_cast<int>(spell) == 98)
        return;

    SpriteObject sprite;
    sprite.spriteId = SpellSpriteMapping[spell]; // Sprite IDs follow the spell's school and slot, as the spell IDs do.
    sprite.containing_item.Reset();
    sprite.uSpellID = spell;
    sprite.spell_level = skillLevel;
    sprite.spell_skill = mastery;
    sprite.uObjectDescID = pObjectTable->ObjectIDByItemID(sprite.spriteId);
    sprite.vPosition = from;
    sprite.uAttributes = SPRITE_IGNORE_RANGE;
    sprite.uSectorID = pIndoor->GetSector(from);
    sprite.field_60_distance_related_prolly_lod = distance;
    sprite.timeSinceCreated = 0_ticks;
    sprite.spell_caster_pid = Pid(OBJECT_Sprite, 1000);
    sprite.uFacing = yaw;
    const int speed = pObjectTable->pObjects[sprite.uObjectDescID].uSpeed;

    int spriteId = -1;
    if (static_cast<int>(spell) == 15) {
        const int count = std::to_underlying(mastery) * 2 + 1;
        const int arc = 60 * TrigLUT.uIntegerDoublePi / 360;
        sprite.spell_target_pid = Pid::character(0);
        for (int angle = -arc / 2; angle <= arc / 2; angle += arc / (count - 1)) {
            sprite.uFacing = yaw + angle;
            spriteId = sprite.Create(yaw + angle, pitch, speed, 0);
        }
    } else {
        spriteId = sprite.Create(yaw, pitch, speed, 0);
    }
    pAudioPlayer->playSpellSound(spell, false, SOUND_MODE_PID, Pid(OBJECT_Sprite, spriteId));
}

/**
 * Applies a hit by an MM6 event spell to a character, as the original character hit routine does for a projectile
 * that an event launched. The damage is rolled first. Without a given target it lands on a random character who can
 * act, and on nobody when none can.
 *
 * @param sprite                        Event spell sprite that hit.
 * @param targetCharacter               Character index, or -1 for a random one.
 */
inline void damageCharacterFromEventSpell(const SpriteObject &sprite, int targetCharacter) {
    const int damage = eventSpellDamage(sprite.uSpellID, sprite.spell_level, *grng);
    if (targetCharacter == -1)
        targetCharacter = pParty->getRandomActiveCharacterId(grng);
    if (targetCharacter == -1)
        return;

    const std::optional<SpellDamageTag> &tag = pSpellStats->pInfos[sprite.uSpellID].mm6DamageTag;
    if (!tag)
        throw std::runtime_error("MM6 event spell has no damage tag");
    pParty->pCharacters[targetCharacter].receiveDamage(damage, spellDamageType(*tag));
}

}  // namespace mm6
