#include <optional>
#include <string>

#include "GUI/UI/MM6RoomDescriptors.h"

#include "Library/Json/Json.h"

#include "Testing/Unit/MM6TestData.h"

#include "Utility/Memory/Blob.h"

#include "gtest/gtest.h"

TEST(MM6RoomDescriptors, RejectsMalformedManifestBeforeUse) {
    const std::optional<Blob> manifest = readMM6TestRules("house-rooms.json");
    if (!manifest)
        GTEST_SKIP() << mm6TestRulesMissing;
    const std::string source(manifest->str());
    const mm6::RoomDescriptors valid = mm6::decodeRoomDescriptors(Blob::fromString(source));
    EXPECT_THROW(valid.at(-1), std::exception);
    EXPECT_THROW(valid.at(119), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 0, 1), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 558, 1), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 1, 0), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 1, 119), std::exception);
    EXPECT_THROW(mm6::selectHouseRoom(valid, 600, 1), std::exception);
    EXPECT_EQ(mm6::selectHouseRoom(valid, 1, std::nullopt).kind, mm6::HouseRoomKind::Blank);
    EXPECT_EQ(mm6::selectHouseRoom(valid, 601, std::nullopt).kind, mm6::HouseRoomKind::Terminal);

    Json document = Json::parse(source);
    document.at("rooms")[1]["video_name"] = "../bad";
    EXPECT_THROW(mm6::decodeRoomDescriptors(Blob::fromString(document.dump())), std::exception);
    document = Json::parse(source);
    document.at("rooms")[2]["id"] = 1;
    EXPECT_THROW(mm6::decodeRoomDescriptors(Blob::fromString(document.dump())), std::exception);
    document = Json::parse(source);
    document.at("rooms").erase(document.at("rooms").end() - 1);
    EXPECT_THROW(mm6::decodeRoomDescriptors(Blob::fromString(document.dump())), std::exception);
}
