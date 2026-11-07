#include <array>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Tables/MapTable.h"
#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6MapStatsEncounterColumns, UsesInternalStemsRatherThanDisplayLabels) {
    std::array<std::string, 30> cells{};
    cells[0] = "1";
    cells[1] = "Later Outdoor";
    cells[2] = "OutE2.Odm";
    cells[5] = "672";
    cells[8] = "2";
    cells[13] = "PeasantM3";
    cells[14] = "Hermit";
    cells[15] = "2";
    cells[16] = "4-6";
    cells[17] = "PeasantF3";
    cells[18] = "Cutpurse";
    cells[19] = "2";
    cells[20] = "4-6";
    cells[21] = "Goblin";
    cells[22] = "Village Guard";
    cells[23] = "1";
    cells[24] = "1-4";
    cells[25] = "2";
    std::string row;
    for (std::size_t index = 0; index < cells.size(); ++index) {
        if (index) row += '\t';
        row += cells[index];
    }
    const std::string table = "heading\r\ncolumns\r\nunits\r\n" + row + "\r\n";
    MapTable maps;
    maps.Initialize(Blob::fromString(table), GameTitle::MM6);
    const MapData &entry = maps.pInfos[static_cast<MapId>(1)];
    EXPECT_EQ(entry.fileName, "oute2.odm");
    EXPECT_EQ(entry.encounter1MonsterInternalName, "PeasantM3");
    EXPECT_EQ(entry.encounter2MonsterInternalName, "PeasantF3");
    EXPECT_EQ(entry.encounter3MonsterInternalName, "Goblin");
}
