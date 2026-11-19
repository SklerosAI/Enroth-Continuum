#include <memory>
#include <optional>
#include <set>
#include <string>

#include "Engine/MM6/MM6Executable.h"

#include "GUI/UI/MM6RoomDescriptors.h"

#include "Testing/Unit/MM6TestData.h"

#include "Utility/String/Ascii.h"

#include "gtest/gtest.h"

TEST(MM6RoomDescriptors, ExecutableTableAndRoomSelection) {
    // Room 0 has no movie and every other room has its own.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::RoomDescriptors valid = mm6::decodeRoomDescriptors(mm6::Executable(data->read("mm6.exe")));
    EXPECT_EQ(valid.at(0).videoName, "");
    std::set<std::string> names;
    for (int roomId = 1; roomId <= 118; ++roomId) {
        EXPECT_NE(valid.at(roomId).videoName, "");
        names.insert(ascii::toLower(valid.at(roomId).videoName));
    }
    EXPECT_EQ(names.size(), 118);

    EXPECT_THROW(valid.at(-1), std::exception);
    EXPECT_THROW(valid.at(119), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 0, 1), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 558, 1), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 1, 0), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 1, 119), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 600, 1), std::exception);
    EXPECT_EQ(mm6::selectHouseRoom(valid, 1, std::nullopt).kind, mm6::HouseRoomKind::Blank);
    EXPECT_EQ(mm6::selectHouseRoom(valid, 601, std::nullopt).kind, mm6::HouseRoomKind::Terminal);
}
