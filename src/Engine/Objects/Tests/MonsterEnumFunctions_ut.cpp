#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/GameTitle.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Objects/Monsters.h"

#include "Utility/Memory/Blob.h"

GAME_TEST(MonsterEnumFunctions, MonsterTierForMonsterId) {
    // Angel A/B/C map to tiers A/B/C.
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_ANGEL_A), MONSTER_TIER_A);
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_ANGEL_B), MONSTER_TIER_B);
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_ANGEL_C), MONSTER_TIER_C);

    // Spot check some other monsters.
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_GHOST_A), MONSTER_TIER_A);
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_GHOST_B), MONSTER_TIER_B);
    EXPECT_EQ(monsterTierForMonsterId(MONSTER_GHOST_C), MONSTER_TIER_C);
}

GAME_TEST(MonsterEnumFunctions, MonsterIdForMonsterTypeAndTier) {
    // Spot checks.
    EXPECT_EQ(monsterIdForMonsterTypeAndTier(MONSTER_TYPE_ANGEL, MONSTER_TIER_A), MONSTER_ANGEL_A);
    EXPECT_EQ(monsterIdForMonsterTypeAndTier(MONSTER_TYPE_ANGEL, MONSTER_TIER_C), MONSTER_ANGEL_C);
    EXPECT_EQ(monsterIdForMonsterTypeAndTier(MONSTER_TYPE_GHOST, MONSTER_TIER_B), MONSTER_GHOST_B);

    // Splitting any monster id into type and tier and reassembling must give the same id back.
    for (MonsterId id : allMonsters())
        EXPECT_EQ(monsterIdForMonsterTypeAndTier(monsterTypeForMonsterId(id), monsterTierForMonsterId(id)), id);
}

namespace {

std::string syntheticMonsterRow(std::string_view id, std::string_view attackType) {
    std::array<std::string_view, 39> cells;
    cells.fill("0");
    cells[0] = id;
    cells[1] = "Synthetic monster";
    cells[2] = "Synthetic monster";
    cells[3] = "1";
    cells[4] = "10";
    cells[9] = "N";
    cells[10] = "Short";
    cells[11] = "Normal";
    cells[17] = attackType;
    cells[18] = "1D1";
    cells[20] = "1";
    cells[21] = attackType;
    cells[22] = "1D1";

    std::string row;
    for (std::string_view cell : cells) {
        if (!row.empty())
            row += '\t';
        row += cell;
    }
    return row + "\r\n";
}

} // namespace

GAME_TEST(MonsterEnumFunctions, Mm7EnergyAttackTypes) {
    std::string table = "header\r\nheader\r\nheader\r\nheader\r\n";
    table += syntheticMonsterRow("1", "Ener");
    std::string mm6Table = table;
    table += syntheticMonsterRow("2", "Earth");
    Blob source = Blob::fromString(std::move(table));

    MonsterStats mm7;
    mm7.Initialize(source, GameTitle::MM7);
    EXPECT_EQ(mm7.infos[MONSTER_ANGEL_A].attack1Type, DAMAGE_ENERGY);
    EXPECT_EQ(mm7.infos[MONSTER_ANGEL_A].attack2Type, DAMAGE_ENERGY);
    EXPECT_EQ(mm7.infos[MONSTER_ANGEL_B].attack1Type, DAMAGE_EARTH);
    EXPECT_EQ(mm7.infos[MONSTER_ANGEL_B].attack2Type, DAMAGE_EARTH);

    MonsterStats mm6;
    mm6.Initialize(Blob::fromString(std::move(mm6Table)), GameTitle::MM6);
    EXPECT_EQ(mm6.infos[MONSTER_ANGEL_A].attack1Type, DAMAGE_ENERGY);
    EXPECT_EQ(mm6.infos[MONSTER_ANGEL_A].attack2Type, DAMAGE_ENERGY);
}
