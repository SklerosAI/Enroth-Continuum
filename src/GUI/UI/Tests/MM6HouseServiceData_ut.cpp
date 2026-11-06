#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "GUI/UI/MM6HouseServiceData.h"

#include "Library/Json/Json.h"
#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormats.h"

#include "Library/Vid/VidReader.h"

#include "Media/MM6MovieLookup.h"

#include "Utility/Memory/Blob.h"

#include "gtest/gtest.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 private house test input is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string changeHouseRowField(std::string source, int houseId, int column, std::string_view replacement) {
    size_t start = source.find("\r\n" + std::to_string(houseId) + "\t");
    if (start == std::string::npos)
        throw std::runtime_error("MM6 retained first house row is missing");
    start += 2;
    const size_t end = source.find("\r\n", start);
    for (int current = 0; current < column; ++current) {
        start = source.find('\t', start);
        if (start == std::string::npos || start >= end)
            throw std::runtime_error("MM6 retained house row is too short");
        ++start;
    }
    size_t next = source.find('\t', start);
    if (next == std::string::npos || next > end)
        next = end;
    source.replace(start, next - start, replacement);
    return source;
}

} // namespace

TEST(MM6HouseServiceData, PreservesRetainedRowsWithoutMM7ServiceRules) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!manifest || !research)
        GTEST_SKIP() << "private MM6 house evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const std::filesystem::path root(research);
    const mm6::HouseServiceData data = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(root / "icons" / "2devents.txt")), rooms);
    const Json report = Json::parse(readFile(root / "house-route-probe.json"));
    const Json &reportedRows = report.at("house_rows");
    ASSERT_EQ(reportedRows.size(), 557);

    std::array<int, 4> families{};
    int numericRooms = 0;
    int blankRooms = 0;
    int valueRows = 0;
    int stockRows = 0;
    int auxiliaryRows = 0;
    int nonnumericAuxiliaryRows = 0;
    int scheduledRows = 0;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseServiceRow &row = data.at(houseId);
        EXPECT_EQ(row.houseId, houseId);
        ++families[static_cast<int>(row.family)];
        const Json &reported = reportedRows.at(std::to_string(houseId));
        if (reported.at("room_id").is_null()) {
            EXPECT_EQ(row.room.kind, mm6::HouseRoomKind::Blank);
            EXPECT_FALSE(row.room.room.has_value());
            ++blankRooms;
        } else {
            EXPECT_EQ(row.room.kind, mm6::HouseRoomKind::Room);
            ASSERT_TRUE(row.room.room.has_value());
            EXPECT_EQ(row.room.room->videoName,
                      rooms.at(reported.at("room_id").get<int>()).videoName);
            ++numericRooms;
        }
        valueRows += row.value.has_value();
        stockRows += !row.stockOrSkillText.empty();
        auxiliaryRows += !row.auxiliaryText.empty();
        nonnumericAuxiliaryRows += !row.auxiliaryText.empty() &&
            !std::all_of(row.auxiliaryText.begin(), row.auxiliaryText.end(),
                         [](char value) { return value >= '0' && value <= '9'; });
        EXPECT_EQ(row.openHour.has_value(), row.closeHour.has_value());
        scheduledRows += row.openHour.has_value();
    }
    EXPECT_EQ(families[static_cast<int>(mm6::HouseServiceFamily::Shop)], 47);
    EXPECT_EQ(families[static_cast<int>(mm6::HouseServiceFamily::MagicGuild)], 22);
    EXPECT_EQ(families[static_cast<int>(mm6::HouseServiceFamily::Transport)], 21);
    EXPECT_EQ(families[static_cast<int>(mm6::HouseServiceFamily::Other)], 467);
    EXPECT_EQ(numericRooms, 526);
    EXPECT_EQ(blankRooms, 31);
    EXPECT_EQ(valueRows, 153);
    EXPECT_EQ(stockRows, 123);
    EXPECT_EQ(auxiliaryRows, 89);
    EXPECT_EQ(nonnumericAuxiliaryRows, 20);
    EXPECT_EQ(scheduledRows, 153);
}

TEST(MM6HouseServiceData, RejectsMalformedHouseTableBeforeSelection) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!manifest || !research)
        GTEST_SKIP() << "private MM6 house evidence is unavailable";
    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const std::string source = readFile(std::filesystem::path(research) / "icons" / "2devents.txt");
    const mm6::HouseServiceData valid = mm6::decodeHouseServiceData(Blob::fromString(source), rooms);
    EXPECT_THROW(valid.at(0), std::exception);
    EXPECT_THROW(valid.at(558), std::exception);
    EXPECT_THROW(mm6::decodeHouseServiceData(Blob::fromString(source.substr(0, source.size() / 2)), rooms),
                 std::exception);
    for (const auto &[column, replacement] : {
             std::pair{0, "0"}, std::pair{2, "Unknown MM6 House"},
             std::pair{4, "119"}, std::pair{12, "not-a-value"},
             std::pair{18, "25"}, std::pair{19, ""}}) {
        const std::string invalid = changeHouseRowField(source, 1, column, replacement);
        EXPECT_THROW(mm6::decodeHouseServiceData(Blob::fromString(invalid), rooms), std::exception);
    }
}

TEST(MM6HouseServiceData, PlansOnlyVerifiedTransportMenuSlots) {
    const char *roomManifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *transportManifest = std::getenv("ENROTH_MM6_TRANSPORT_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!roomManifest || !transportManifest || !research)
        GTEST_SKIP() << "private MM6 transport evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(roomManifest)));
    const std::filesystem::path root(research);
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(root / "icons" / "2devents.txt")), rooms);
    const mm6::TransportRules rules = mm6::decodeTransportRules(Blob::fromString(readFile(transportManifest)));
    ASSERT_EQ(rules.houses.size(), 21);

    int transportRows = 0;
    int nonTransport = 0;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseServiceRow &house = houses.at(houseId);
        if (house.family != mm6::HouseServiceFamily::Transport) {
            ++nonTransport;
            EXPECT_EQ(rules.findHouse(houseId), nullptr);
            continue;
        }
        ++transportRows;
        ASSERT_NE(rules.findHouse(houseId), nullptr);
        ASSERT_EQ(house.room.kind, mm6::HouseRoomKind::Room);
        ASSERT_TRUE(house.room.room.has_value());
        for (int weekday = 0; weekday < 7; ++weekday) {
            const mm6::TransportMenuData menu = mm6::selectTransportMenu(houses, rules, houseId, weekday);
            bool any = false;
            for (size_t slot = 0; slot < menu.routes.size(); ++slot) {
                const mm6::TransportRoute *expected = rules.scheduledRoute(houseId, slot, weekday, false);
                EXPECT_EQ(menu.routes[slot].has_value(), expected != nullptr);
                if (menu.routes[slot]) {
                    ASSERT_NE(expected, nullptr);
                    EXPECT_EQ(menu.routes[slot]->mapIndex, expected->mapIndex);
                    EXPECT_EQ(menu.routes[slot]->duration, expected->duration);
                    EXPECT_EQ(menu.routes[slot]->questBit, 0);
                    any = true;
                }
            }
            EXPECT_EQ(menu.hasAvailableRoute(), any);
        }
    }
    EXPECT_EQ(transportRows, 21);
    EXPECT_EQ(nonTransport, 536);
    EXPECT_THROW(mm6::selectTransportMenu(houses, rules, 0, 0), std::exception);
    EXPECT_THROW(mm6::selectTransportMenu(houses, rules, 558, 0), std::exception);
    EXPECT_THROW(mm6::selectTransportMenu(houses, rules, rules.houses[0].id, -1), std::exception);
    EXPECT_THROW(mm6::selectTransportMenu(houses, rules, rules.houses[0].id, 7), std::exception);
    for (int houseId = 1; houseId <= 557; ++houseId) {
        if (houses.at(houseId).family != mm6::HouseServiceFamily::Transport) {
            EXPECT_THROW(mm6::selectTransportMenu(houses, rules, houseId, 0), std::exception);
            break;
        }
    }

    const int transportId = rules.houses[0].id;
    mm6::TransportRules missingRouteHouse = rules;
    missingRouteHouse.houses.erase(missingRouteHouse.houses.begin());
    EXPECT_THROW(mm6::selectTransportMenu(houses, missingRouteHouse, transportId, 0), std::exception);

    const std::string source = readFile(root / "icons" / "2devents.txt");
    const std::string blankRoom = changeHouseRowField(source, transportId, 4, "");
    const mm6::HouseServiceData missingRoom = mm6::decodeHouseServiceData(Blob::fromString(blankRoom), rooms);
    EXPECT_THROW(mm6::selectTransportMenu(missingRoom, rules, transportId, 0), std::exception);
}

TEST(MM6HouseServiceData, PreflightsEveryActiveHouseInstruction) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!manifest || !research)
        GTEST_SKIP() << "private MM6 house evidence is unavailable";

    const std::filesystem::path root(research);
    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(root / "icons" / "2devents.txt")), rooms);
    const Json report = Json::parse(readFile(root / "house-route-probe.json"));
    std::istringstream iconIndex(readFile(root / "icons.entries"));
    std::set<std::string> iconNames;
    for (std::string name; std::getline(iconIndex, name);) {
        if (!name.empty() && name.back() == '\r')
            name.pop_back();
        iconNames.insert(name);
    }
    int tableBacked = 0;
    int blankOrUnused = 0;
    int terminal = 0;
    int proprietorIcons = 0;
    for (const Json &site : report.at("enter_house_refs")) {
        const int houseId = site.at("house_id").get<int>();
        const mm6::HouseEntryPlan entry = mm6::planHouseEntry(houses, houseId);
        EXPECT_EQ(entry.houseId, houseId);
        switch (entry.kind) {
            case mm6::HouseEntryKind::TableBacked:
                ASSERT_TRUE(entry.room.has_value());
                EXPECT_FALSE(entry.room->videoName.empty());
                if (entry.proprietorIconName) {
                    EXPECT_GT(entry.room->npcPicture, 0);
                    EXPECT_TRUE(iconNames.contains(*entry.proprietorIconName));
                    ++proprietorIcons;
                } else {
                    EXPECT_EQ(entry.room->npcPicture, 0);
                }
                ++tableBacked;
                break;
            case mm6::HouseEntryKind::BlankOrUnused:
                EXPECT_FALSE(entry.room.has_value());
                EXPECT_FALSE(entry.proprietorIconName.has_value());
                ++blankOrUnused;
                break;
            case mm6::HouseEntryKind::Terminal:
                EXPECT_FALSE(entry.room.has_value());
                EXPECT_FALSE(entry.proprietorIconName.has_value());
                ++terminal;
                break;
        }
    }
    EXPECT_EQ(tableBacked, 671);
    EXPECT_EQ(blankOrUnused, 1);
    EXPECT_EQ(terminal, 2);
    EXPECT_EQ(proprietorIcons, 282);
    EXPECT_THROW(mm6::planHouseEntry(houses, 0), std::exception);
    EXPECT_THROW(mm6::planHouseEntry(houses, 558), std::exception);

    int taverns = 0;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseServiceRow &row = houses.at(houseId);
        if (row.type != HOUSE_TYPE_TAVERN)
            continue;
        const mm6::HouseEntryPlan entry = mm6::planHouseEntry(houses, houseId);
        EXPECT_EQ(entry.kind, mm6::HouseEntryKind::TableBacked);
        EXPECT_TRUE(entry.room.has_value());
        EXPECT_TRUE(row.value.has_value());
        EXPECT_TRUE(entry.openHour.has_value());
        EXPECT_TRUE(entry.closeHour.has_value());
        ++taverns;
    }
    EXPECT_EQ(taverns, 21);
}

TEST(MM6HouseServiceData, DecodesEveryReferencedProprietorImageFromStagedArchive) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    const char *archive = std::getenv("ENROTH_MM6_STAGED_ICONS_LOD");
    if (!manifest || !research || !archive)
        GTEST_SKIP() << "private MM6 room and staged icon evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(std::filesystem::path(research) / "icons" / "2devents.txt")), rooms);
    const LodReader icons{PathView(archive)};
    std::set<std::string> names;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseEntryPlan entry = mm6::planHouseEntry(houses, houseId);
        if (entry.proprietorIconName)
            names.insert(*entry.proprietorIconName);
    }
    EXPECT_EQ(names.size(), 45);
    for (const std::string &name : names) {
        ASSERT_TRUE(icons.exists(name));
        const Blob imageData = icons.read(name);
        const Sizei size = lod::decodeImageSize(imageData);
        ASSERT_GT(size.w, 0);
        ASSERT_GT(size.h, 0);
        ASSERT_LE(size.w, 1024);
        ASSERT_LE(size.h, 1024);
        const LodImage image = lod::decodeImage(imageData);
        EXPECT_EQ(image.image.size(), size);
        EXPECT_EQ(image.image.pixels().size(), static_cast<size_t>(size.w * size.h));
    }
}

TEST(MM6HouseServiceData, ResolvesEveryReferencedRoomMovieFromStagedArchives) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    const char *firstPath = std::getenv("ENROTH_MM6_STAGED_ANIMS1_VID");
    const char *secondPath = std::getenv("ENROTH_MM6_STAGED_ANIMS2_VID");
    if (!manifest || !research || !firstPath || !secondPath)
        GTEST_SKIP() << "private MM6 room and staged movie evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(std::filesystem::path(research) / "icons" / "2devents.txt")), rooms);
    const VidReader first{PathView(firstPath)};
    const VidReader second{PathView(secondPath)};
    std::set<std::string> names;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseEntryPlan entry = mm6::planHouseEntry(houses, houseId);
        if (entry.kind == mm6::HouseEntryKind::TableBacked) {
            ASSERT_TRUE(entry.room.has_value());
            names.insert(entry.room->videoName);
        }
    }
    EXPECT_EQ(names.size(), 116);
    for (const std::string &name : names) {
        const Blob movie = lookupMovieBlob(GameTitle::MM6, first, second, name);
        EXPECT_TRUE(movie);
        EXPECT_GT(movie.size(), 16);
        EXPECT_TRUE(movie.str().starts_with("SMK"));
    }
}
