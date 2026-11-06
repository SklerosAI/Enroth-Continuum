#include <array>
#include <cstring>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/GameTitle.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/TableSerialization.h"
#include "Engine/Tables/TileTable.h"

#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6TileTable, KeepsOriginalWaterRoadNamesWithoutChangingMM7) {
    std::array<TileData_MM7, 2> records{};
    std::memcpy(records[0].textureName.data(), "wtrdrn", 7);
    std::memcpy(records[1].textureName.data(), "wtrtyl", 7);
    for (TileData_MM7 &record : records) {
        record.tileset = TILESET_MM7_WATER;
        record.variant = TILE_VARIANT_MM7_BASE1_NSEW;
    }
    std::string data(4 + sizeof(records), '\0');
    data[0] = static_cast<char>(records.size());
    std::memcpy(data.data() + 4, records.data(), sizeof(records));

    TileTable mm6;
    TileTable mm7;
    deserialize(Blob::fromString(data), &mm6, GameTitle::MM6);
    deserialize(Blob::fromString(std::move(data)), &mm7, GameTitle::MM7);
    EXPECT_EQ(mm6.tile(0).textureName, "wtrdrn");
    EXPECT_EQ(mm6.tile(1).textureName, "wtrtyl");
    EXPECT_EQ(mm7.tile(0).textureName, "hwtrdrn");
    EXPECT_EQ(mm7.tile(1).textureName, "wtrtyl");
}
