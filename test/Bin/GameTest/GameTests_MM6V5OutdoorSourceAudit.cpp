#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6EncounterMonster.h"
#include "Engine/MM6/MM6V5OutdoorMapIdentity.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/MapTable.h"

// Read-only source admission for the exact 15-map outdoor grid. This test
// neither publishes a map nor copies game data into the repository.
GAME_TEST(MM6V5OutdoorSourceHeadless, AllShippedOutdoorPairsDecode) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto names = maps.expectedNames();
    ASSERT_EQ(names.size(), 67);
    int admitted = 0;
    for (char column = 'a'; column <= 'e'; ++column)
        for (char row = '1'; row <= '3'; ++row) {
            std::string name = "out";
            name += column;
            name += row;
            name += ".odm";
            SCOPED_TRACE(name);
            ASSERT_TRUE(mm6::isMM6OutdoorMapName(name));
            ASSERT_NE(pMapTable->GetMapInfo(name), MAP_INVALID);
            const std::string deltaName = name.substr(0, 5) + ".ddm";
            ASSERT_EQ(std::count(names.begin(), names.end(), deltaName), 1);
            const auto base = std::make_unique<OutdoorLocation_MM7>();
            ASSERT_NO_THROW(deserializeMM6Outdoor(maps.readBase(name), base.get()));
            MM6OutdoorDeltaSnapshot delta;
            ASSERT_NO_THROW(delta = decodeMM6OutdoorDelta(
                decodeMM6ArchiveEntry(maps.readDefaultDelta(deltaName))));
            EXPECT_EQ(base->decorations.size(), base->decorationNames.size());
            EXPECT_EQ(delta.chests.size(), 20);
            ++admitted;
        }
    EXPECT_EQ(admitted, 15);
}

// Check the authored encounter stems in every loaded map table row against
// the title's monster and descriptor tables. No source rows are published.
GAME_TEST(MM6V5OutdoorSourceHeadless, AllMapEncounterStemsResolve) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    int checked = 0;
    for (MapId mapId : pMapTable->pInfos.indices()) {
        const MapData &map = pMapTable->pInfos[mapId];
        if (map.fileName.empty())
            continue;
        SCOPED_TRACE(map.fileName);
        const std::array<std::pair<const std::string *, int>, 3> encounters{{
            {&map.encounter1MonsterInternalName, map.Dif_M1},
            {&map.encounter2MonsterInternalName, map.Dif_M2},
            {&map.encounter3MonsterInternalName, map.Dif_M3},
        }};
        for (const auto &[stem, difficulty] : encounters) {
            if (stem->empty() || *stem == "0")
                continue;
            for (int rank = 0; rank < (difficulty ? 3 : 1); ++rank) {
                std::string name = *stem;
                if (difficulty) {
                    name += ' ';
                    name += static_cast<char>('A' + rank);
                }
                const MonsterId id = mm6::resolveEncounterMonster(
                    GameTitle::MM6, *pMonsterStats, name);
                ASSERT_NE(id, MONSTER_INVALID);
                ASSERT_FALSE(pMonsterList->monsters[id].internalMonsterName.empty());
                ++checked;
            }
        }
    }
    EXPECT_GT(checked, 0);
}
