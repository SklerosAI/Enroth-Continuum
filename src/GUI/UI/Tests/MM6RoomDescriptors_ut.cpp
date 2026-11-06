#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>

#include "GUI/UI/MM6RoomDescriptors.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

#include "gtest/gtest.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 private room test input is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

const char *manifestPath() {
    return std::getenv("ENROTH_MM6_ROOM_MANIFEST");
}

} // namespace

TEST(MM6RoomDescriptors, RetainedRowsAndActiveHouseEntries) {
    const char *manifest = manifestPath();
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!manifest || !research)
        GTEST_SKIP() << "private MM6 room evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const std::filesystem::path root(research);
    const Json pe = Json::parse(readFile(root / "F3_PE_RECONCILIATION_2026-11-05_v2.json"));
    const Json house = Json::parse(readFile(root / "house-route-probe.json"));
    const Json &peRows = pe.at("house_movies").at("rows");
    ASSERT_EQ(peRows.size(), 119);
    for (int id = 0; id < 119; ++id) {
        const mm6::RoomDescriptor &room = rooms.at(id);
        const Json &expected = peRows[id];
        EXPECT_EQ(room.videoName, expected.at("file_name").value("value", ""));
        EXPECT_EQ(room.background, expected.at("background").get<int>());
        EXPECT_EQ(room.npcPicture, expected.at("npc_picture").get<int>());
        EXPECT_EQ(room.rawHouseType, expected.at("house_type").get<int>());
        EXPECT_EQ(room.soundGroup, expected.at("sound_group").get<int>());
    }

    const Json &houseRows = house.at("house_rows");
    ASSERT_EQ(houseRows.size(), 557);
    std::set<int> allReferencedRooms;
    for (auto it = houseRows.begin(); it != houseRows.end(); ++it) {
        const Json &value = it.value().at("room_id");
        if (value.is_null()) continue;
        const int roomId = value.get<int>();
        ASSERT_FALSE(rooms.at(roomId).videoName.empty());
        ASSERT_TRUE(peRows[roomId].at("video_name_match").get<bool>());
        allReferencedRooms.insert(roomId);
    }
    EXPECT_EQ(allReferencedRooms.size(), 116);

    int numericEvents = 0;
    int blankEvents = 0;
    int terminalEvents = 0;
    std::set<int> activeRoomIds;
    for (const Json &event : house.at("enter_house_refs")) {
        const int houseId = event.at("house_id").get<int>();
        const std::string key = std::to_string(houseId);
        if (!houseRows.contains(key)) {
            EXPECT_TRUE(houseId == 600 || houseId == 601);
            ++terminalEvents;
            continue;
        }
        const Json &value = houseRows.at(key).at("room_id");
        if (value.is_null()) {
            ++blankEvents;
            continue;
        }
        const int roomId = value.get<int>();
        ASSERT_FALSE(rooms.at(roomId).videoName.empty());
        activeRoomIds.insert(roomId);
        ++numericEvents;
    }
    EXPECT_EQ(numericEvents, 671);
    EXPECT_EQ(blankEvents, 1);
    EXPECT_EQ(terminalEvents, 2);
    EXPECT_EQ(activeRoomIds.size(), 114);
}

TEST(MM6RoomDescriptors, RejectsMalformedManifestBeforeUse) {
    const char *manifest = manifestPath();
    if (!manifest)
        GTEST_SKIP() << "private MM6 room manifest is unavailable";
    const std::string source = readFile(manifest);
    const mm6::RoomDescriptors valid = mm6::decodeRoomDescriptors(Blob::fromString(source));
    EXPECT_THROW(valid.at(-1), std::exception);
    EXPECT_THROW(valid.at(119), std::exception);

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
