#include "GUI/UI/MM6HouseServiceData.h"

#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Tables/HouseTable.h"
#include "Engine/Engine.h"
#include "Engine/MM6/MM6Executable.h"
#include "Engine/Resources/ResourceManager.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Split.h"

namespace mm6 {
namespace {

std::optional<int> optionalNumber(std::string_view token, int minimum, int maximum) {
    if (token.empty())
        return std::nullopt;
    int value = 0;
    if (!tryDeserialize(token, &value) || value < minimum || value > maximum)
        throw std::runtime_error("MM6 house service field is outside retained bounds");
    return value;
}

std::string boundedText(std::string_view token) {
    if (token.size() > 256)
        throw std::runtime_error("MM6 house service text is too long");
    return std::string(token);
}

std::optional<float> optionalValue(std::string_view token) {
    if (token.empty())
        return std::nullopt;
    float value = 0;
    if (!tryDeserialize(token, &value) || !std::isfinite(value) || value < 0 || value > 1000)
        throw std::runtime_error("MM6 house service value is outside retained bounds");
    return value;
}

} // namespace

const HouseServiceRow &HouseServiceData::at(int houseId) const {
    if (houseId < 1 || houseId > 557)
        throw std::runtime_error("MM6 house service ID is outside retained table");
    return _rows[houseId];
}

HouseServiceData decodeHouseServiceData(const Blob &source, const RoomDescriptors &rooms) {
    if (source.size() > 131072)
        throw std::runtime_error("MM6 house service table is too large");

    const std::string_view text = source.str();
    HouseServiceData candidate;
    int expectedId = 1;
    for (std::string_view line : split(text).by("\r\n").drop(2).skip("")) {
        const std::array<std::string_view, 24> fields = split(line).by('\t').resize(24, "");
        const std::optional<int> houseId = optionalNumber(fields[0], 1, 557);
        if (!houseId || *houseId != expectedId)
            throw std::runtime_error("MM6 house service rows are missing or out of order");
        const std::string_view label = fields[2];
        const HouseType type = parseHouseType(label, GameTitle::MM6);
        if (type == HOUSE_TYPE_INVALID && !label.empty() && label != "Not Used")
            throw std::runtime_error("MM6 house service has unknown type label");

        HouseServiceRow &row = candidate._rows[expectedId];
        row.houseId = expectedId;
        row.type = type;
        row.family = houseServiceFamily(type);
        row.room = selectHouseRoom(rooms, expectedId, optionalNumber(fields[4], 1, 118));
        row.value = optionalValue(fields[12]);
        row.stockOrSkillText = boundedText(fields[13]);
        row.auxiliaryText = boundedText(fields[15]);
        row.openHour = optionalNumber(fields[18], 0, 24);
        row.closeHour = optionalNumber(fields[19], 0, 24);
        if (row.openHour.has_value() != row.closeHour.has_value())
            throw std::runtime_error("MM6 house service hours must form a pair");
        row.exit = {houseExitField(fields[20]), houseExitField(fields[21]), houseExitField(fields[22])};
        if (row.exit.picture < 0 || row.exit.picture > 8 || (row.exit.picture && row.exit.restriction < -6))
            throw std::runtime_error("MM6 house exit is outside retained bounds");
        ++expectedId;
    }
    if (expectedId != 558)
        throw std::runtime_error("MM6 house service table requires 557 rows");
    return candidate;
}

const HouseServiceData &houseServiceData() {
    if (!engine || engine->gameTitle() != GameTitle::MM6)
        throw std::runtime_error("MM6 house data requires the MM6 runtime");
    static const HouseServiceData data = [] {
        const RoomDescriptors rooms = decodeRoomDescriptors(executable());
        return decodeHouseServiceData(engine->resources()->eventsData("2dEvents.txt"), rooms);
    }();
    return data;
}

HouseEntryPlan planHouseEntry(const HouseServiceData &houses, int houseId) {
    if (houseId == 600 || houseId == 601)
        return {houseId, HouseEntryKind::Terminal, HouseServiceFamily::Other,
                std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    const HouseServiceRow &house = houses.at(houseId);
    if (house.type == HOUSE_TYPE_INVALID || house.room.kind != HouseRoomKind::Room)
        return {houseId, HouseEntryKind::BlankOrUnused, house.family,
                std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    std::optional<std::string> iconName;
    if (const unsigned picture = house.room.room->npcPicture) {
        std::string number = std::to_string(picture);
        iconName = "npc" + std::string(number.size() < 3 ? 3 - number.size() : 0, '0') + number;
    }
    return {houseId, HouseEntryKind::TableBacked, house.family, house.room.room,
            iconName, house.openHour, house.closeHour};
}

bool isHouseOpen(std::optional<int> openHour, std::optional<int> closeHour, int hour) {
    if (!openHour || !closeHour)
        return true;
    const int lastHour = *closeHour - 1;
    const unsigned now = hour; // The original compares unsigned, so a close hour of 0 keeps a house open all day.
    if (lastHour > *openHour)
        return now >= static_cast<unsigned>(*openHour) && now <= static_cast<unsigned>(lastHour);
    return now >= static_cast<unsigned>(*openHour) || now <= static_cast<unsigned>(lastHour);
}

HousePresentationState planHousePresentation(GameTitle title, const HouseServiceData &houses, int houseId) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 house presentation requires the MM6 title");
    const HouseEntryPlan entry = planHouseEntry(houses, houseId);
    if (entry.kind != HouseEntryKind::TableBacked || !entry.room)
        throw std::runtime_error("MM6 house presentation requires a file-backed room");
    // F3 matched this candidate for all table-referenced rooms with a sound
    // group. The original entry timing and cue policy remain unproved.
    const std::optional<SoundId> candidateGreeting = entry.room->soundGroup
        ? std::optional<SoundId>(static_cast<SoundId>(30000 + 100 * entry.room->soundGroup + 1))
        : std::nullopt;
    return {houseId, entry.room->videoName, entry.proprietorIconName, candidateGreeting};
}

std::vector<HouseServiceOption> houseServiceOptions(HouseType type) {
    using enum HouseServiceOption;
    switch (houseServiceFamily(type)) {
        case HouseServiceFamily::Shop:
            if (type == HOUSE_TYPE_GENERAL_STORE)
                return {BuyItems, SellItems};
            return {BuyItems, SellItems, IdentifyItems, RepairItems, BuySpecialItems};
        case HouseServiceFamily::MagicGuild: {
            const std::vector<Skill> skills = guildSkills(type);
            if (skills.empty())
                return {};
            std::vector<HouseServiceOption> result = {BuySpells};
            result.insert(result.end(), skills.size(), LearnSkill);
            return result;
        }
        case HouseServiceFamily::Transport:
            if (type == HOUSE_TYPE_STABLE)
                return {Travel, Travel, Travel, FixPrices};
            return {Travel, Travel, Travel};
        case HouseServiceFamily::Other:
            break;
    }
    switch (type) {
        case HOUSE_TYPE_TEMPLE:
            return {TempleHeal, TempleDonate};
        case HOUSE_TYPE_TRAINING_GROUND:
            return {Train};
        case HOUSE_TYPE_BANK:
            return {Deposit, Withdraw};
        case HOUSE_TYPE_TAVERN:
            return {RentRoom, FillPacks, HaveADrink, TipBarkeep};
        case HOUSE_TYPE_MERCENARY_GUILD:
        case HOUSE_TYPE_SHADOW_GUILD:
            return std::vector<HouseServiceOption>(5, LearnSkill);
        default:
            return {};
    }
}

std::vector<Skill> guildSkills(HouseType type) {
    switch (type) {
        case HOUSE_TYPE_FIRE_GUILD: return {SKILL_FIRE, SKILL_LEARNING};
        case HOUSE_TYPE_AIR_GUILD: return {SKILL_AIR, SKILL_LEARNING};
        case HOUSE_TYPE_WATER_GUILD: return {SKILL_WATER, SKILL_LEARNING};
        case HOUSE_TYPE_EARTH_GUILD: return {SKILL_EARTH, SKILL_LEARNING};
        case HOUSE_TYPE_SPIRIT_GUILD: return {SKILL_SPIRIT, SKILL_MEDITATION};
        case HOUSE_TYPE_MIND_GUILD: return {SKILL_MIND, SKILL_MEDITATION};
        case HOUSE_TYPE_BODY_GUILD: return {SKILL_BODY, SKILL_MEDITATION};
        case HOUSE_TYPE_LIGHT_GUILD: return {SKILL_LIGHT};
        case HOUSE_TYPE_DARK_GUILD: return {SKILL_DARK};
        case HOUSE_TYPE_ELEMENTAL_GUILD: return {SKILL_FIRE, SKILL_AIR, SKILL_WATER, SKILL_EARTH};
        case HOUSE_TYPE_SELF_GUILD: return {SKILL_SPIRIT, SKILL_MIND, SKILL_BODY};
        default: return {}; // The mirrored path guild lists nothing.
    }
}

std::array<Skill, 5> skillGuildSkills(HouseType type, int houseId) {
    assert(type == HOUSE_TYPE_MERCENARY_GUILD || type == HOUSE_TYPE_SHADOW_GUILD);
    if (type == HOUSE_TYPE_SHADOW_GUILD) {
        if (houseId >= 147 && houseId <= 151 && houseId % 2 == 1)
            return {SKILL_DAGGER, SKILL_MERCHANT, SKILL_ITEM_ID, SKILL_PERCEPTION, SKILL_TRAP_DISARM};
        return {SKILL_LEATHER, SKILL_DIPLOMACY, SKILL_ITEM_ID, SKILL_PERCEPTION, SKILL_TRAP_DISARM};
    }
    switch (houseId) {
        case 141: return {SKILL_SWORD, SKILL_AXE, SKILL_SPEAR, SKILL_STAFF, SKILL_LEATHER};
        case 142:
        case 144: return {SKILL_MACE, SKILL_BOW, SKILL_CHAIN, SKILL_SHIELD, SKILL_BODYBUILDING};
        case 145: return {SKILL_SWORD, SKILL_AXE, SKILL_SPEAR, SKILL_STAFF, SKILL_REPAIR};
        default: return {SKILL_CHAIN, SKILL_BOW, SKILL_SHIELD, SKILL_PLATE, SKILL_REPAIR};
    }
}

std::optional<AwardId> guildMembershipAward(int houseId) {
    static constexpr int kFirstGuild = 119;
    static constexpr std::array<int, 34> kAwards = {
        74, 74, 72, 72, 75, 75, 73, 73, 78, 78, 77, 77, 76, 76, 79, 79, 80, 80, 64, 64, 65, 65, // Spell guilds, Fire to Self.
        69, 70, 71, 70, 69, 71, // Mercenary guilds.
        66, 66, 67, 67, 68, 68, // Thieves' guilds.
    };
    if (houseId < kFirstGuild || houseId >= kFirstGuild + static_cast<int>(kAwards.size()))
        return std::nullopt;
    return static_cast<AwardId>(kAwards[houseId - kFirstGuild]);
}

LstrId houseServiceOptionLabel(HouseServiceOption option) {
    switch (option) {
        case HouseServiceOption::TempleHeal: return static_cast<LstrId>(104);    // "Heal"
        case HouseServiceOption::TempleDonate: return static_cast<LstrId>(68);   // "Donate"
        case HouseServiceOption::Train: return static_cast<LstrId>(228);         // "Train"
        case HouseServiceOption::BuyItems: return static_cast<LstrId>(539);      // "Buy Items"
        case HouseServiceOption::SellItems: return static_cast<LstrId>(540);     // "Sell Items"
        case HouseServiceOption::IdentifyItems: return static_cast<LstrId>(541); // "Identify Items"
        case HouseServiceOption::RepairItems: return static_cast<LstrId>(542);   // "Repair Items"
        case HouseServiceOption::BuySpecialItems: return static_cast<LstrId>(543); // "Special Items"
        case HouseServiceOption::BuySpells: return static_cast<LstrId>(400);     // "Buy Spells"
        case HouseServiceOption::Deposit: return static_cast<LstrId>(60);        // "Deposit"
        case HouseServiceOption::Withdraw: return static_cast<LstrId>(244);      // "Withdraw"
        case HouseServiceOption::RentRoom: return static_cast<LstrId>(178);      // "Rent Room for %d gold"
        case HouseServiceOption::FillPacks: return static_cast<LstrId>(86);      // "Fill Packs to %d days for %d gold"
        case HouseServiceOption::HaveADrink: return static_cast<LstrId>(398);    // "Have a Drink"
        case HouseServiceOption::TipBarkeep: return static_cast<LstrId>(399);    // "Tip Barkeep"
        case HouseServiceOption::Travel: return static_cast<LstrId>(404);        // "%d days to %s"
        case HouseServiceOption::LearnSkill:
        case HouseServiceOption::FixPrices: break;
    }
    throw std::runtime_error("MM6 house service option has no global.txt label");
}

} // namespace mm6
