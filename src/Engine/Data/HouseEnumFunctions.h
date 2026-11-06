#pragma once

#include <utility>

#include "AwardEnums.h"
#include "HouseEnums.h"

#include "Utility/Segment.h"

inline Segment<HouseId> allTownhallHouses() {
    return {HOUSE_FIRST_TOWN_HALL, HOUSE_LAST_TOWN_HALL};
}

inline Segment<HouseId> allHouses() {
    return {HOUSE_FIRST, HOUSE_LAST};
}

inline Segment<HouseId> allArcomageTaverns() {
    return {HOUSE_FIRST_ARCOMAGE_TAVERN, HOUSE_LAST_ARCOMAGE_TAVERN};
}

inline bool isShop(HouseId houseId) {
    return houseId >= HOUSE_FIRST_SHOP && houseId <= HOUSE_LAST_SHOP;
}

inline bool isWeaponShop(HouseId houseId) {
    return houseId >= HOUSE_FIRST_WEAPON_SHOP && houseId <= HOUSE_LAST_WEAPON_SHOP;
}

inline bool isArmorShop(HouseId houseId) {
    return houseId >= HOUSE_FIRST_ARMOR_SHOP && houseId <= HOUSE_LAST_ARMOR_SHOP;
}

inline bool isMagicShop(HouseId houseId) {
    return houseId >= HOUSE_FIRST_MAGIC_SHOP && houseId <= HOUSE_LAST_MAGIC_SHOP;
}

inline bool isAlchemyShop(HouseId houseId) {
    return houseId >= HOUSE_FIRST_ALCHEMY_SHOP && houseId <= HOUSE_LAST_ALCHEMY_SHOP;
}

inline bool isMagicGuild(HouseId houseId) {
    return houseId >= HOUSE_FIRST_MAGIC_GUILD && houseId <= HOUSE_LAST_MAGIC_GUILD;
}

namespace mm6 {

// This classifies a parsed MM6 house type without assuming MM7 house-ID ranges
// or implying that the corresponding service window and stock are ready.
enum class HouseServiceFamily { Shop, MagicGuild, Transport, Other };

inline HouseServiceFamily houseServiceFamily(HouseType type) {
    switch (type) {
        case HOUSE_TYPE_WEAPON_SHOP:
        case HOUSE_TYPE_ARMOR_SHOP:
        case HOUSE_TYPE_MAGIC_SHOP:
        case HOUSE_TYPE_ALCHEMY_SHOP:
        case HOUSE_TYPE_GENERAL_STORE:
            return HouseServiceFamily::Shop;
        case HOUSE_TYPE_FIRE_GUILD:
        case HOUSE_TYPE_AIR_GUILD:
        case HOUSE_TYPE_WATER_GUILD:
        case HOUSE_TYPE_EARTH_GUILD:
        case HOUSE_TYPE_SPIRIT_GUILD:
        case HOUSE_TYPE_MIND_GUILD:
        case HOUSE_TYPE_BODY_GUILD:
        case HOUSE_TYPE_LIGHT_GUILD:
        case HOUSE_TYPE_DARK_GUILD:
        case HOUSE_TYPE_ELEMENTAL_GUILD:
        case HOUSE_TYPE_SELF_GUILD:
        case HOUSE_TYPE_MIRRORED_PATH_GUILD:
            return HouseServiceFamily::MagicGuild;
        case HOUSE_TYPE_STABLE:
        case HOUSE_TYPE_BOAT:
            return HouseServiceFamily::Transport;
        default:
            return HouseServiceFamily::Other;
    }
}

} // namespace mm6

inline bool isStable(HouseId houseId) {
    return houseId >= HOUSE_FIRST_STABLE && houseId <= HOUSE_LAST_STABLE;
}

inline bool isBoat(HouseId houseId) {
    return houseId >= HOUSE_FIRST_BOAT && houseId <= HOUSE_LAST_BOAT;
}

inline bool isTavern(HouseId houseId) {
    return houseId >= HOUSE_FIRST_TAVERN && houseId <= HOUSE_LAST_TAVERN;
}

inline bool isArcomageTavern(HouseId houseId) {
    return houseId >= HOUSE_FIRST_ARCOMAGE_TAVERN && houseId <= HOUSE_LAST_ARCOMAGE_TAVERN;
}

inline int arcomageTopicForTavern(HouseId houseId) {
    assert(isArcomageTavern(houseId));
    return std::to_underlying(houseId) - std::to_underlying(HOUSE_FIRST_ARCOMAGE_TAVERN) + 355;
}

AwardId membershipAwardForGuild(HouseId houseId);

AwardId membershipAwardForGuild(GuildId guildId);
