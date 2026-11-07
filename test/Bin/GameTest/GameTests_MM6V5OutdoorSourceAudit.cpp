#include <algorithm>
#include <memory>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6V5OutdoorMapIdentity.h"
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
            ASSERT_NO_THROW(delta = decodeMM6OutdoorDelta(maps.readDefaultDelta(deltaName)));
            EXPECT_EQ(base->decorations.size(), base->decorationNames.size());
            EXPECT_EQ(delta.chests.size(), 20);
            ++admitted;
        }
    EXPECT_EQ(admitted, 15);
}
