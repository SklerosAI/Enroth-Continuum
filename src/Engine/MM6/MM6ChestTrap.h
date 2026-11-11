#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/MM6/MM6ChestDisarmScore.h"
#include "Engine/MM6/MM6MonsterDamage.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/MapTable.h"

namespace mm6 {

/**
 * @offset 0x4853e0
 *
 * @param character                     Character trying to disarm a chest trap.
 * @return                              The character's MM6 chest disarm score.
 */
inline int chestDisarmScore(const Character &character) {
    std::array<ChestDisarmEquippedItem, 16> equipped{};
    for (ItemSlot slot : allItemSlots())
        if (InventoryConstEntry item = character.inventory.entry(slot))
            equipped[std::to_underlying(slot) - 1] = {std::to_underlying(item->itemId), static_cast<int>(item->specialEnchantment), item->IsBroken()};
    const std::optional<ChestDisarmEquipmentFlags> flags = scanChestDisarmEquipment(GameTitle::MM6, equipped);
    if (!flags)
        throw std::runtime_error("MM6 character has equipment outside the title range");

    ChestDisarmScoreInput input;
    input.title = GameTitle::MM6;
    input.encodedSkill = static_cast<std::uint8_t>(character.getSkillValue(SKILL_TRAP_DISARM).joined());
    input.unbrokenItem410 = flags->unbrokenItem410;
    input.unbrokenItem415 = flags->unbrokenItem415;
    input.unbrokenThieveryEnchantment = flags->unbrokenThieveryEnchantment;
    input.hasTinker = CheckHiredNPCSpeciality(NPC_PROFESSION_TINKER);
    input.hasLocksmith = CheckHiredNPCSpeciality(NPC_PROFESSION_LOCKSMITH);
    input.hasBurglar = CheckHiredNPCSpeciality(NPC_PROFESSION_BURGLAR);
    return *stageChestDisarmScore(input);
}

/**
 * @param sprite                        Sprite of a sprite object.
 * @return                              Damage channel of the MM6 chest trap that the sprite shows, or empty if it
 *                                      isn't one.
 */
inline std::optional<DamageType> chestTrapDamageType(SpriteId sprite) {
    // Event damage kinds from the jump table at 0x430b34. Sprite 812 is the cold trap and 813 the electric one, the
    // other way round from what their MM7 names say.
    switch (sprite) {
        case SPRITE_TRAP_FIRE: return damageTypeForEventKind(2);
        case SPRITE_TRAP_LIGHTNING: return damageTypeForEventKind(4);
        case SPRITE_TRAP_COLD: return damageTypeForEventKind(3);
        case SPRITE_TRAP_BODY: return damageTypeForEventKind(5);
        default: return std::nullopt;
    }
}

/**
 * @offset 0x445420
 *
 * @param x                             Absolute X coordinate of a vector.
 * @param y                             Absolute Y coordinate.
 * @param z                             Absolute Z coordinate.
 * @return                              The original integer estimate of the vector's length.
 */
inline int approximateLength(int x, int y, int z) {
    std::array<int, 3> sides = {x, y, z};
    std::ranges::sort(sides);
    return sides[2] + ((sides[1] * 11) >> 5) + (sides[0] >> 2);
}

/**
 * Hurts the party with an MM6 chest trap whose sprite has run out its lifetime, unless the party has walked away.
 *
 * @offset 0x4309d0
 *
 * @param trap                          Sprite object whose lifetime has run out. Anything that isn't a chest trap is
 *                                      ignored.
 */
inline void explodeChestTrap(const SpriteObject &trap) {
    const std::optional<DamageType> type = chestTrapDamageType(trap.spriteId);
    if (!type)
        return;
    const int dx = std::abs(static_cast<int>(pParty->pos.x) - static_cast<int>(trap.vPosition.x));
    const int dy = std::abs(static_cast<int>(pParty->pos.y) - static_cast<int>(trap.vPosition.y));
    const int dz = std::abs(static_cast<int>(pParty->pos.z) + pParty->eyeLevel - static_cast<int>(trap.vPosition.z));
    if (approximateLength(dx, dy, dz) > 1024)
        return;

    const int damage = 5 + grng->randomDice(pMapTable->pInfos[engine->_currentLoadedMapId].trapDamageD20DiceCount, 20);
    for (Character &character : pParty->pCharacters) {
        const int perception = character.getSkillValue(SKILL_PERCEPTION).joined(); // Raw skill with its mastery bits, no item bonus.
        if (perception && grng->random(perception + 20) > 20) {
            character.playReaction(SPEECH_AVOID_DAMAGE);
        } else {
            character.receiveDamage(damage, *type);
        }
    }
}

}  // namespace mm6
