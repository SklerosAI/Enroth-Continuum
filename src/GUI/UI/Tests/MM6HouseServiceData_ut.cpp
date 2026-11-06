#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "GUI/UI/MM6HouseServiceData.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

#include "gtest/gtest.h"

namespace {

std::string readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 private house test input is missing");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string changeFirstRowField(std::string source, int column, std::string_view replacement) {
    size_t start = source.find("\r\n1\t");
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
        const std::string invalid = changeFirstRowField(source, column, replacement);
        EXPECT_THROW(mm6::decodeHouseServiceData(Blob::fromString(invalid), rooms), std::exception);
    }
}
