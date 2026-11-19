#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/MM6/MM6Executable.h"
#include "Engine/Spells/MM6CombatBuffRules.h"
#include "Engine/Spells/MM6FirstAidRules.h"
#include "Engine/Spells/MM6PowerCureRules.h"
#include "Engine/Spells/MM6SpellRules.h"
#include "Engine/Spells/MM6TimedCureRules.h"

#include "GUI/UI/Houses/MM6ShopStock.h"
#include "GUI/UI/Houses/MM6TransportRules.h"
#include "GUI/UI/MM6RoomDescriptors.h"

#include "Library/Json/Json.h"

#include "Testing/Unit/MM6TestData.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Format.h"

#include "gtest/gtest.h"

static Json profileRules(std::string_view name) {
    const std::optional<Blob> file = readMM6TestRules(name);
    return file ? Json::parse(file->str()) : Json();
}

static bool cures(SpellId spell, Mastery mastery, int skill, int ageSeconds, mm6::TimedCureWindow window) {
    const Time now = Time::fromDays(1000);
    CharacterConditions conditions;
    const Condition condition = std::to_underlying(spell) == 40 ? CONDITION_PETRIFIED :
                                std::to_underlying(spell) == 49 ? CONDITION_CURSED :
                                std::to_underlying(spell) == 57 ? CONDITION_FEAR :
                                std::to_underlying(spell) == 60 ? CONDITION_PARALYZED :
                                std::to_underlying(spell) == 64 ? CONDITION_INSANE : CONDITION_WEAK;
    conditions.set(condition, now - Duration::fromSeconds(ageSeconds));
    return mm6::applyTimedCure(spell, mastery, skill, now, conditions, window);
}

TEST(MM6ExecutableRules, MatchesProfileJson) {
    // Every value the engine now takes from MM6.exe or from MM6.exe's code equals the profile's rules/*.json, except
    // the differences checked by name below.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data || !readMM6TestRules("creation.json"))
        GTEST_SKIP() << mm6TestDataMissing << ", " << mm6TestRulesMissing;
    const mm6::Executable exe(data->read("mm6.exe"));

    const Json creation = profileRules("creation.json");
    mm6::loadCreationRules(exe);
    const mm6::CreationRules &rules = mm6::creationRules();
    for (int family = 0; family < mm6::kClassFamilyCount; ++family) {
        EXPECT_EQ(rules.hpBase[family], creation["hp_base"][family].get<int>());
        EXPECT_EQ(rules.spBase[family], creation["sp_base"][family].get<int>());
        for (int stat = 0; stat < mm6::kAttributeCount; ++stat)
            EXPECT_EQ(rules.startingStats[family][stat], creation["starting_stats"][family][stat].get<int>());
        for (int skill = 0; skill < mm6::kSkillCount; ++skill)
            EXPECT_EQ(rules.startingSkills[family][skill], creation["starting_skills"][family][skill].get<int>());
    }
    for (int classId = 0; classId < mm6::kClassCount; ++classId) {
        EXPECT_EQ(rules.hpFactor[classId], creation["hp_factor"][classId].get<int>());
        EXPECT_EQ(rules.spFactor[classId], creation["sp_factor"][classId].get<int>());
    }

    const Json spells = profileRules("spells.json");
    const mm6::SpellRules spellRules = mm6::decodeSpellRules(exe);
    for (int id = 1; id <= 99; ++id) {
        for (int rank = 0; rank < 3; ++rank) {
            EXPECT_EQ(spellRules[id][rank].mana, spells["spells"][id - 1]["mana"][rank].get<int>()) << id;
            EXPECT_EQ(spellRules[id][rank].recoveryTicks, spells["spells"][id - 1]["recovery"][rank].get<int>()) << id;
        }
    }

    const Json firstAid = profileRules("first-aid.json");
    for (int rank = 1; rank <= 3; ++rank)
        EXPECT_EQ(mm6::firstAidHealAmount(SPELL_BODY_FIRST_AID, static_cast<Mastery>(rank)), firstAid["heal"][rank - 1].get<int>());

    const Json powerCure = profileRules("power-cure.json");
    for (int rank = 1; rank <= 3; ++rank)
        for (int skill = 1; skill <= 63; ++skill)
            EXPECT_EQ(mm6::powerCureHealAmount(SPELL_BODY_POWER_CURE, static_cast<Mastery>(rank), skill),
                      powerCure["base"].get<int>() + powerCure["per_skill"].get<int>() * skill);

    // The json holds the windows the spell descriptions print. MM6.exe gives Remove Fear, Cure Paralysis, Cure Insanity
    // and Cure Weakness three times those at Expert and Master, the bug GrayFace's FixConditionSpells fixes.
    const Json timedCures = profileRules("timed-cures.json");
    for (const Json &id : timedCures["ids"]) {
        const SpellId spell = static_cast<SpellId>(id.get<int>());
        for (int rank = 1; rank <= 3; ++rank) {
            SCOPED_TRACE(fmt::format("spell={} rank={}", id.get<int>(), rank));
            const Mastery mastery = static_cast<Mastery>(rank);
            const int described = timedCures["seconds_per_skill"][rank - 1].get<int>() * 2;
            const int original = described * (id.get<int>() >= 57 && rank >= 2 ? 3 : 1);
            EXPECT_TRUE(cures(spell, mastery, 2, described - 1, mm6::TIMED_CURE_WINDOW_DESCRIBED));
            EXPECT_FALSE(cures(spell, mastery, 2, described, mm6::TIMED_CURE_WINDOW_DESCRIBED));
            EXPECT_TRUE(cures(spell, mastery, 2, original - 1, mm6::TIMED_CURE_WINDOW_ORIGINAL));
            EXPECT_FALSE(cures(spell, mastery, 2, original, mm6::TIMED_CURE_WINDOW_ORIGINAL));
        }
    }

    // The json took the base duration from the printed "1 hour", MM6.exe 0x42472A and 0x426C21 add 3840 seconds.
    const Json combatBuffs = profileRules("combat-buffs.json");
    EXPECT_EQ(combatBuffs["base_seconds"].get<int>(), 3600);
    for (const Json &id : combatBuffs["ids"]) {
        const SpellId spell = static_cast<SpellId>(id.get<int>());
        for (int rank = 1; rank <= 3; ++rank) {
            const Time now = Time::fromDays(10);
            const mm6::CombatBuffPlan plan = mm6::planCombatBuff(spell, static_cast<Mastery>(rank), 7, now);
            EXPECT_EQ(plan.expires - now, Duration::fromSeconds(3840 + combatBuffs["per_skill_seconds"][rank - 1].get<int>() * 7));
            if (spell == SPELL_SPIRIT_HEROISM)
                EXPECT_EQ(plan.power, combatBuffs["heroism_power"][0].get<int>() + combatBuffs["heroism_power"][1].get<int>() * 7);
        }
    }

    const Json houseRooms = profileRules("house-rooms.json");
    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(exe);
    for (int roomId = 0; roomId < 119; ++roomId) {
        const Json &row = houseRooms["rooms"][roomId];
        EXPECT_EQ(rooms.at(roomId).videoName, row["video_name"].get<std::string>());
        EXPECT_EQ(rooms.at(roomId).background, row["background"].get<int>());
        EXPECT_EQ(rooms.at(roomId).npcPicture, row["npc_picture"].get<int>());
        EXPECT_EQ(rooms.at(roomId).rawHouseType, row["house_type"].get<int>());
        EXPECT_EQ(rooms.at(roomId).soundGroup, row["sound_group"].get<int>());
    }

    // The json's slot kinds describe the slot text in 2DEvents.txt. The game never reads them, so they aren't compared.
    const Json transport = profileRules("transport.json");
    const mm6::TransportRules transportRules = mm6::decodeTransportRules(exe);
    ASSERT_EQ(transportRules.routes.size(), transport["routes"].size());
    for (size_t i = 0; i < transportRules.routes.size(); ++i) {
        const Json &row = transport["routes"][i];
        const mm6::TransportRoute &route = transportRules.routes[i];
        EXPECT_EQ(route.mapIndex, row["map_index"].get<int>());
        for (int day = 0; day < 7; ++day)
            EXPECT_EQ(route.days[day], row["days"][day].get<int>());
        EXPECT_EQ(route.duration, row["duration"].get<int>());
        for (int axis = 0; axis < 3; ++axis)
            EXPECT_EQ(route.position[axis], row["position"][axis].get<int>());
        EXPECT_EQ(route.yaw, row["yaw"].get<int>());
        EXPECT_EQ(route.questBit, row["quest_bit"].get<int>());
    }
    ASSERT_EQ(transportRules.houses.size(), transport["houses"].size());
    for (size_t i = 0; i < transportRules.houses.size(); ++i) {
        const Json &row = transport["houses"][i];
        EXPECT_EQ(transportRules.houses[i].id, row["id"].get<int>());
        for (int slot = 0; slot < 3; ++slot)
            EXPECT_EQ(transportRules.houses[i].routes[slot], row["slots"][slot]["route"].get<size_t>());
    }

    const Json shopStock = profileRules("shop-stock.json");
    const mm6::ShopStockRules stock = mm6::decodeShopStockRules(exe);
    for (int type = 0; type < 4; ++type)
        EXPECT_EQ(stock.visibleSlots[type], shopStock["visible"][type].get<int>());
    for (int bank = 0; bank < 2; ++bank) {
        for (int house = 1; house <= 47; ++house) {
            const Json &rows = shopStock["banks"][bank][house - 1]["rows"];
            ASSERT_EQ(stock.banks[bank][house].rows.size(), rows.size());
            for (size_t i = 0; i < rows.size(); ++i) {
                EXPECT_EQ(stock.banks[bank][house].rows[i].treasureLevel, rows[i]["level"].get<int>());
                EXPECT_EQ(stock.banks[bank][house].rows[i].choices, rows[i]["choices"].get<std::vector<int>>());
            }
        }
    }
    for (int guild = 0; guild < 22; ++guild)
        EXPECT_EQ(stock.guildBooks[guild], shopStock["guilds"][guild]["books"].get<int>());
}
