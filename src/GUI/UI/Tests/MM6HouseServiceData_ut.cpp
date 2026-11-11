#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "GUI/UI/MM6HouseServiceData.h"
#include "GUI/UI/Houses/MM6TransportRules.h"
#include "GUI/UI/UIHouses.h"

#include "Engine/Snapshots/TableSerialization.h"

#include "Library/Json/Json.h"
#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormats.h"
#include "Library/Snd/SndReader.h"

#include "Library/Vid/VidReader.h"

#include "Media/MM6MovieLookup.h"
#include "Media/MediaPlayer.h"
#include "Media/Audio/SoundList.h"
#include "Media/AudioBufferDataSource.h"

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

TEST(MM6HouseServiceData, EveryTransportHouseHasRoutesAndARoom) {
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
        EXPECT_NE(rules.findHouse(houseId), nullptr);
        EXPECT_EQ(house.room.kind, mm6::HouseRoomKind::Room);
        EXPECT_TRUE(house.room.room.has_value());
    }
    EXPECT_EQ(transportRows, 21);
    EXPECT_EQ(nonTransport, 536);
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

TEST(MM6HouseServiceData, DecodesMM6PreviewPanelAndExitImages) {
    const char *archive = std::getenv("ENROTH_MM6_STAGED_ICONS_LOD");
    if (!archive)
        GTEST_SKIP() << "private staged MM6 icon evidence is unavailable";

    const LodReader icons{PathView(archive)};
    for (const auto &[name, maximumWidth, maximumHeight] : {
             std::tuple{"backevt", 163, 480}, std::tuple{"buttexi1", 169, 35}}) {
        ASSERT_TRUE(icons.exists(name));
        const Blob data = icons.read(name);
        const Sizei size = lod::decodeImageSize(data);
        EXPECT_GT(size.w, 0);
        EXPECT_GT(size.h, 0);
        EXPECT_LE(size.w, maximumWidth) << name;
        EXPECT_LE(size.h, maximumHeight) << name;
        EXPECT_EQ(lod::decodeImage(data).image.size(), size);
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

TEST(MM6HouseServiceData, RejectsMalformedAndTruncatedRoomMovieFrames) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    const char *firstPath = std::getenv("ENROTH_MM6_STAGED_ANIMS1_VID");
    const char *secondPath = std::getenv("ENROTH_MM6_STAGED_ANIMS2_VID");
    if (!manifest || !research || !firstPath || !secondPath)
        GTEST_SKIP() << "private MM6 room and staged movie evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(std::filesystem::path(research) / "icons" / "2devents.txt")), rooms);
    const mm6::HousePresentationState room = mm6::planHousePresentation(GameTitle::MM6, houses, 1);
    const VidReader first{PathView(firstPath)};
    const VidReader second{PathView(secondPath)};
    const Blob movie = lookupMovieBlob(GameTitle::MM6, first, second, room.videoName);
    ASSERT_TRUE(movie);

    EXPECT_FALSE(mm6::probeRoomMovieFirstFrame(Blob::fromString("BAD00000")));
    EXPECT_FALSE(mm6::probeRoomMovieFirstFrame(Blob::fromString("SMK00000")));
    EXPECT_FALSE(mm6::probeRoomMovieFirstFrame(movie.subBlob(0, std::min<size_t>(movie.size(), 64))));
}

TEST(MM6HouseServiceData, DecodesAllReferencedStagedRoomMovieFirstFramesWithoutAudioOrUI) {
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
        if (entry.kind == mm6::HouseEntryKind::TableBacked)
            names.insert(entry.room->videoName);
    }
    ASSERT_EQ(names.size(), 116);

    std::vector<std::pair<std::string, std::string>> failures;
    int decoded = 0;
    for (const std::string &name : names) {
        Blob movie;
        try {
            movie = lookupMovieBlob(GameTitle::MM6, first, second, name);
        } catch (const std::exception &) {
            failures.emplace_back(name, "lookup_error");
            continue;
        }
        if (!movie) {
            failures.emplace_back(name, "missing_payload");
        } else if (movie.size() > 32 * 1024 * 1024) {
            failures.emplace_back(name, "payload_over_bound");
        } else if (const auto frame = mm6::probeRoomMovieFirstFrame(movie)) {
            if (frame->size.w > 0 && frame->size.h > 0 && frame->size.w <= 1024 &&
                frame->size.h <= 1024 &&
                frame->decodedBytes >= static_cast<size_t>(frame->size.w * frame->size.h * 4)) {
                ++decoded;
            } else {
                failures.emplace_back(name, "decoded_frame_invalid");
            }
        } else {
            failures.emplace_back(name, "decoder_or_packet_bound");
        }
    }

    if (!failures.empty()) {
        const std::filesystem::path privateReport = std::filesystem::path(research) / "F4_ROOM_FRAME_FAILURES_PRIVATE.tsv";
        const int fd = ::open(privateReport.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC,
                              S_IRUSR | S_IWUSR);
        ASSERT_GE(fd, 0);
        const int chmodResult = ::fchmod(fd, S_IRUSR | S_IWUSR);
        ::close(fd);
        ASSERT_EQ(chmodResult, 0);
        std::ofstream output(privateReport, std::ios::app);
        ASSERT_TRUE(output.good());
        for (const auto &[name, reason] : failures)
            output << name << '\t' << reason << '\n';
    }
    EXPECT_EQ(decoded, 116);
    EXPECT_EQ(failures.size(), 0);
}

TEST(MM6HouseServiceData, ActivatesPresentationOnlyForTableBackedMM6Rooms) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!manifest || !research)
        GTEST_SKIP() << "private MM6 house evidence is unavailable";

    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(std::filesystem::path(research) / "icons" / "2devents.txt")), rooms);
    int tableBacked = 0;
    int unavailable = 0;
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const mm6::HouseEntryPlan entry = mm6::planHouseEntry(houses, houseId);
        if (entry.kind == mm6::HouseEntryKind::TableBacked) {
            const mm6::HousePresentationState state = mm6::planHousePresentation(GameTitle::MM6, houses, houseId);
            EXPECT_EQ(state.houseId, houseId);
            ASSERT_TRUE(entry.room.has_value());
            EXPECT_EQ(state.videoName, entry.room->videoName);
            EXPECT_EQ(state.proprietorIconName, entry.proprietorIconName);
            EXPECT_EQ(state.candidateGreetingSoundId.has_value(), entry.room->soundGroup != 0);
            ++tableBacked;
        } else {
            EXPECT_THROW(mm6::planHousePresentation(GameTitle::MM6, houses, houseId), std::exception);
            ++unavailable;
        }
        EXPECT_THROW(mm6::planHousePresentation(GameTitle::MM7, houses, houseId), std::exception);
    }
    EXPECT_EQ(tableBacked, 526);
    EXPECT_EQ(unavailable, 31);
    EXPECT_THROW(mm6::planHousePresentation(GameTitle::MM6, houses, 600), std::exception);
    EXPECT_THROW(mm6::planHousePresentation(GameTitle::MM6, houses, 601), std::exception);
}

TEST(MM6HouseServiceData, PreviewExitSkipsLegacyDialogueAndCharacterAdvanceOnlyForMM6House) {
    EXPECT_TRUE(isMM6HousePresentationScreen(GameTitle::MM6, SCREEN_HOUSE));
    EXPECT_FALSE(isMM6HousePresentationScreen(GameTitle::MM6, SCREEN_GAME));
    EXPECT_FALSE(isMM6HousePresentationScreen(GameTitle::MM7, SCREEN_HOUSE));
}

TEST(MM6HouseServiceData, ServiceOptionsFollowHouseFamilyAndType) {
    // A proprietor lists the services of their house's family, or of their house type for the other houses.
    using enum mm6::HouseServiceOption;
    using Options = std::vector<mm6::HouseServiceOption>;
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_WEAPON_SHOP), Options({BuyItems, SellItems, IdentifyItems, RepairItems, BuySpecialItems}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_MAGIC_SHOP), Options({BuyItems, SellItems, IdentifyItems, RepairItems, BuySpecialItems}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_GENERAL_STORE), Options({BuyItems, SellItems}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_ELEMENTAL_GUILD), Options({BuySpells, LearnSkills}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_STABLE), Options({Travel, Travel, Travel, FixPrices}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_BOAT), Options({Travel, Travel, Travel}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_TEMPLE), Options({TempleHeal, TempleDonate}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_TRAINING_GROUND), Options({Train}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_BANK), Options({Deposit, Withdraw}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_TAVERN), Options({RentRoom, FillPacks, HaveADrink, TipBarkeep}));
    EXPECT_EQ(mm6::houseServiceOptions(HOUSE_TYPE_HOUSE), Options());
    EXPECT_EQ(mm6::houseServiceOptionLabel(TempleHeal), LSTR_HEAL);
    EXPECT_EQ(mm6::houseServiceOptionLabel(Deposit), LSTR_DEPOSIT);
    EXPECT_THROW(mm6::houseServiceOptionLabel(FixPrices), std::exception);
}

TEST(MM6HouseServiceData, StagesOnlyF3MatchedMM6GreetingCandidatesWithoutPlayback) {
    const char *manifest = std::getenv("ENROTH_MM6_ROOM_MANIFEST");
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    const char *archive = std::getenv("ENROTH_MM6_STAGED_AUDIO_SND");
    if (!manifest || !research || !archive)
        GTEST_SKIP() << "private MM6 house cue evidence is unavailable";

    const std::filesystem::path root(research);
    const mm6::RoomDescriptors rooms = mm6::decodeRoomDescriptors(Blob::fromString(readFile(manifest)));
    const mm6::HouseServiceData houses = mm6::decodeHouseServiceData(
        Blob::fromString(readFile(root / "icons" / "2devents.txt")), rooms);
    const Json media = Json::parse(readFile(root / "F3_MEDIA_RECONCILIATION_2026-11-05.json"));
    const Json routes = Json::parse(readFile(root / "house-route-probe.json"));
    SoundList sounds;
    deserialize(Blob::fromString(readFile(root / "icons" / "dsounds.bin")), &sounds, GameTitle::MM6);
    const SndReader snd{PathView(archive)};

    std::array<int, 119> houseForRoom{};
    for (int houseId = 1; houseId <= 557; ++houseId) {
        const Json &row = routes.at("house_rows").at(std::to_string(houseId));
        if (row.at("room_id").is_number_integer()) {
            const int roomId = row.at("room_id").get<int>();
            ASSERT_GE(roomId, 1);
            ASSERT_LE(roomId, 118);
            if (!houseForRoom[roomId])
                houseForRoom[roomId] = houseId;
        }
    }

    int withGreeting = 0;
    int silent = 0;
    for (const Json &join : media.at("house_room_joins")) {
        const int roomId = join.at("room_id").get<int>();
        ASSERT_GE(roomId, 1);
        ASSERT_LE(roomId, 118);
        ASSERT_NE(houseForRoom[roomId], 0);
        const mm6::HousePresentationState state = mm6::planHousePresentation(
            GameTitle::MM6, houses, houseForRoom[roomId]);
        const mm6::RoomDescriptor &room = rooms.at(roomId);
        const Json &candidate = join.at("candidate_house_sounds").at("1");
        if (!room.soundGroup) {
            EXPECT_FALSE(state.candidateGreetingSoundId);
            EXPECT_FALSE(candidate.at("name_in_original_snd").get<bool>());
            ++silent;
            continue;
        }

        ASSERT_TRUE(state.candidateGreetingSoundId);
        const SoundId soundId = *state.candidateGreetingSoundId;
        EXPECT_EQ(std::to_underlying(soundId), candidate.at("id").get<int>());
        EXPECT_TRUE(candidate.at("name_in_original_snd").get<bool>());
        const SoundInfo *sound = sounds.soundInfo(soundId);
        ASSERT_NE(sound, nullptr);
        ASSERT_FALSE(sound->name.empty());
        EXPECT_TRUE(snd.exists(sound->name));
        ++withGreeting;
    }
    EXPECT_EQ(withGreeting, 63);
    EXPECT_EQ(silent, 53);
}

TEST(MM6HouseServiceData, DecodesOneActiveMM6EventSoundWithoutPlayback) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    const char *archive = std::getenv("ENROTH_MM6_STAGED_AUDIO_SND");
    if (!research || !archive)
        GTEST_SKIP() << "private MM6 event sound and staged SND evidence is unavailable";

    const std::filesystem::path root(research);
    const Json report = Json::parse(readFile(root / "F3_MEDIA_RECONCILIATION_2026-11-05.json"));
    const Json &events = report.at("event_sound_joins");
    ASSERT_EQ(events.size(), 8);
    const int soundId = events.at(0).at("sound_id").get<int>();
    ASSERT_GT(soundId, 0);

    SoundList table;
    deserialize(Blob::fromString(readFile(root / "icons" / "dsounds.bin")), &table, GameTitle::MM6);
    const SoundInfo *sound = table.soundInfo(static_cast<SoundId>(soundId));
    ASSERT_NE(sound, nullptr);
    ASSERT_FALSE(sound->name.empty());
    const SndReader snd{PathView(archive)};
    ASSERT_TRUE(snd.exists(sound->name));
    Blob wave = snd.read(sound->name);
    ASSERT_TRUE(wave);
    ASSERT_LE(wave.size(), 4 * 1024 * 1024);

    AudioBufferDataSource decoder(std::move(wave));
    ASSERT_TRUE(decoder.Open());
    const size_t sampleRate = decoder.GetSampleRate();
    const size_t channels = decoder.GetChannelCount();
    EXPECT_GE(sampleRate, 8000);
    EXPECT_LE(sampleRate, 96000);
    EXPECT_GE(channels, 1);
    EXPECT_LE(channels, 2);
    EXPECT_TRUE(std::isfinite(decoder.GetDuration()));
    EXPECT_GT(decoder.GetDuration(), 0.0f);
    EXPECT_LE(decoder.GetDuration(), 60.0f);
    const Blob samples = decoder.GetNextBuffer();
    ASSERT_TRUE(samples);
    ASSERT_GT(channels, 0);
    const size_t sampleCount = samples.size() / (channels * sizeof(std::int16_t));
    EXPECT_GT(sampleCount, 0);
    EXPECT_LE(sampleCount, 2'000'000);
    EXPECT_EQ(samples.size(), sampleCount * channels * sizeof(std::int16_t));
    decoder.Close();

    AudioBufferDataSource malformed(Blob::fromString("not a sound"));
    EXPECT_FALSE(malformed.Open());
}

TEST(MM6HouseServiceData, OpeningHours) {
    // An MM6 house closes at the start of its close hour, and wraps past midnight when the close hour comes first.
    EXPECT_TRUE(mm6::isHouseOpen(std::nullopt, std::nullopt, 3));
    EXPECT_FALSE(mm6::isHouseOpen(10, 14, 9));
    EXPECT_TRUE(mm6::isHouseOpen(10, 14, 10));
    EXPECT_TRUE(mm6::isHouseOpen(10, 14, 13));
    EXPECT_FALSE(mm6::isHouseOpen(10, 14, 14));
    EXPECT_TRUE(mm6::isHouseOpen(18, 6, 23));
    EXPECT_TRUE(mm6::isHouseOpen(18, 6, 5));
    EXPECT_FALSE(mm6::isHouseOpen(18, 6, 6));
    EXPECT_FALSE(mm6::isHouseOpen(18, 6, 17));
    EXPECT_TRUE(mm6::isHouseOpen(22, 0, 12));
}
