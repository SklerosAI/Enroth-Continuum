#pragma once

#include <array>
#include <utility>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/ItemEnums.h"

namespace mm6 {

/**
 * One attribute that an MM6 artifact or relic raises or lowers while it's equipped.
 */
struct ArtifactAttributeBonus {
    int item = 0;
    Attribute attribute = ATTRIBUTE_MIGHT;
    int bonus = 0;
};

/**
 * Attribute bonuses of MM6's artifacts and relics, items 400 to 429, as MM6.exe's item bonus routine has them in its
 * switch over items 401 to 429. Mordred and Percival have no entry there. Items raise four resistances, Fire,
 * Electricity, Cold and Poison, which the shared runtime keeps in its fire, air, water and body fields.
 */
inline constexpr std::array kArtifactAttributeBonuses = std::to_array<ArtifactAttributeBonus>({
    {401, ATTRIBUTE_MIGHT, 20}, // Thor.
    {402, ATTRIBUTE_MIGHT, 20}, // Conan.
    {403, ATTRIBUTE_MIGHT, 30}, // Excalibur.
    {404, ATTRIBUTE_MANA, 40}, // Merlin.
    {406, ATTRIBUTE_HEALTH, 25}, // Galahad.
    {406, ATTRIBUTE_RESIST_FIRE, 10},
    {406, ATTRIBUTE_RESIST_AIR, 10},
    {406, ATTRIBUTE_RESIST_WATER, 10},
    {406, ATTRIBUTE_RESIST_BODY, 10},
    {407, ATTRIBUTE_ENDURANCE, 30}, // Pellinore.
    {408, ATTRIBUTE_ACCURACY, 30}, // Valeria.
    {409, ATTRIBUTE_MIGHT, 10}, // Arthur.
    {409, ATTRIBUTE_INTELLIGENCE, 10},
    {409, ATTRIBUTE_PERSONALITY, 10},
    {409, ATTRIBUTE_ENDURANCE, 10},
    {409, ATTRIBUTE_ACCURACY, 10},
    {409, ATTRIBUTE_SPEED, 10},
    {409, ATTRIBUTE_LUCK, 10},
    {409, ATTRIBUTE_MANA, 25},
    {410, ATTRIBUTE_LUCK, 30}, // Pendragon.
    {411, ATTRIBUTE_SPEED, 30}, // Lucius.
    {412, ATTRIBUTE_MANA, 30}, // Guinevere.
    {413, ATTRIBUTE_MANA, 25}, // Igraine.
    {414, ATTRIBUTE_MANA, 20}, // Morgan.
    {415, ATTRIBUTE_LUCK, 20}, // Hades.
    {416, ATTRIBUTE_RESIST_FIRE, 25}, // Ares.
    {417, ATTRIBUTE_MIGHT, 20}, // Poseidon.
    {417, ATTRIBUTE_ENDURANCE, 20},
    {417, ATTRIBUTE_ACCURACY, 20},
    {417, ATTRIBUTE_SPEED, -10},
    {417, ATTRIBUTE_AC_BONUS, -10},
    {418, ATTRIBUTE_HEALTH, 100}, // Cronos.
    {418, ATTRIBUTE_LUCK, -50},
    {419, ATTRIBUTE_MIGHT, 50}, // Hercules.
    {419, ATTRIBUTE_ENDURANCE, 20},
    {419, ATTRIBUTE_INTELLIGENCE, -30},
    {420, ATTRIBUTE_RESIST_FIRE, -10}, // Artemis.
    {420, ATTRIBUTE_RESIST_AIR, -10},
    {420, ATTRIBUTE_RESIST_WATER, -10},
    {420, ATTRIBUTE_RESIST_BODY, -10},
    {421, ATTRIBUTE_ENDURANCE, -30}, // Apollo.
    {421, ATTRIBUTE_LUCK, 20},
    {421, ATTRIBUTE_RESIST_FIRE, 20},
    {421, ATTRIBUTE_RESIST_AIR, 20},
    {421, ATTRIBUTE_RESIST_WATER, 20},
    {421, ATTRIBUTE_RESIST_BODY, 20},
    {422, ATTRIBUTE_HEALTH, 50}, // Zeus.
    {422, ATTRIBUTE_MANA, 50},
    {422, ATTRIBUTE_LUCK, 50},
    {422, ATTRIBUTE_INTELLIGENCE, -50},
    {423, ATTRIBUTE_LUCK, 20}, // Aegis.
    {423, ATTRIBUTE_SPEED, -20},
    {424, ATTRIBUTE_SPEED, -40}, // Odin.
    {424, ATTRIBUTE_RESIST_FIRE, 50},
    {424, ATTRIBUTE_RESIST_AIR, 50},
    {424, ATTRIBUTE_RESIST_WATER, 50},
    {424, ATTRIBUTE_RESIST_BODY, 50},
    {425, ATTRIBUTE_MIGHT, 100}, // Atlas.
    {425, ATTRIBUTE_SPEED, -40},
    {426, ATTRIBUTE_SPEED, 100}, // Hermes.
    {426, ATTRIBUTE_ACCURACY, -40},
    {427, ATTRIBUTE_PERSONALITY, 100}, // Aphrodite.
    {427, ATTRIBUTE_LUCK, -40},
    {428, ATTRIBUTE_INTELLIGENCE, 100}, // Athena.
    {428, ATTRIBUTE_MIGHT, -40},
    {429, ATTRIBUTE_HEALTH, 50}, // Hera.
    {429, ATTRIBUTE_MANA, 50},
    {429, ATTRIBUTE_LUCK, 50},
    {429, ATTRIBUTE_PERSONALITY, -50},
});

/**
 * @param item                          Item ID.
 * @return                              Whether it's one of MM6's artifacts and relics, items 400 to 429.
 */
constexpr bool isArtifactOrRelic(ItemId item) {
    return std::to_underlying(item) >= 400 && std::to_underlying(item) <= 429;
}

/**
 * @offset 0x482E80
 *
 * @param item                          Equipped MM6 artifact or relic.
 * @param attribute                     Attribute to look up.
 * @return                              What the item adds to the attribute, negative for a relic's drawback.
 */
constexpr int artifactAttributeBonus(ItemId item, Attribute attribute) {
    int result = 0;
    for (const ArtifactAttributeBonus &entry : kArtifactAttributeBonuses)
        if (entry.item == std::to_underlying(item) && entry.attribute == attribute)
            result += entry.bonus;
    return result;
}

/**
 * @offset 0x481E61
 *
 * @param item                          Weapon an MM6 character attacks with.
 * @return                              Whether it's Merlin or Percival, which shorten the attack recovery as much as the
 *                                      Swift enchantment does.
 */
constexpr bool isSwiftArtifact(ItemId item) {
    return std::to_underlying(item) == 404 || std::to_underlying(item) == 405;
}

/**
 * @offset 0x47E4B0
 *
 * @param weapon                        MM6 weapon that hits.
 * @param enchantment                   Its special enchantment.
 * @param monster                       MM6 monster ID of the target, its row in `monsters.txt`.
 * @return                              Whether the weapon's damage roll doubles against the monster. Conan doubles
 *                                      against the devils, 25 to 30, and the lizards, drakes, wyrms and dragons, 31
 *                                      to 42, and ignores its enchantment. Demon and Dragon Slaying each take one of
 *                                      the two ranges.
 */
constexpr bool weaponSlays(ItemId weapon, ItemEnchantment enchantment, int monster) {
    if (std::to_underlying(weapon) == 402) // Conan.
        return monster >= 25 && monster <= 42;
    if (enchantment == ITEM_ENCHANTMENT_DEMON_SLAYING)
        return monster >= 25 && monster <= 30;
    if (enchantment == ITEM_ENCHANTMENT_DRAGON_SLAYING)
        return monster >= 31 && monster <= 42;
    return false;
}

/**
 * Highest skill level an MM6 spellbook cast can use: a skill of 63, half again for a school enchantment and half of
 * that again for an artifact. MM6.exe casts with the boosted level as it is.
 */
inline constexpr int kMaxCastSkillLevel = 141;

/**
 * @offset 0x422829
 *
 * @param item                          Item an MM6 caster wears.
 * @param slot                          Slot it's worn in.
 * @param school                        Magic skill of the spell being cast.
 * @return                              Whether the item boosts that school's spells by half the skill. Morgan does it
 *                                      for Fire, Air, Water and Earth as the amulet, Igraine for Spirit, Mind and Body
 *                                      and Guinevere for Light and Dark on any ring finger.
 */
constexpr bool boostsSpellSchool(ItemId item, ItemSlot slot, Skill school) {
    const bool ring = slot >= ITEM_SLOT_RING1 && slot <= ITEM_SLOT_RING6;
    switch (school) {
        case SKILL_FIRE:
        case SKILL_AIR:
        case SKILL_WATER:
        case SKILL_EARTH:
            return slot == ITEM_SLOT_AMULET && std::to_underlying(item) == 414;
        case SKILL_SPIRIT:
        case SKILL_MIND:
        case SKILL_BODY:
            return ring && std::to_underlying(item) == 413;
        case SKILL_LIGHT:
        case SKILL_DARK:
            return ring && std::to_underlying(item) == 412;
        default:
            return false;
    }
}

} // namespace mm6
