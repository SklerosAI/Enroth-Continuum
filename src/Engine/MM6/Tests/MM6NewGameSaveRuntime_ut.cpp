#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6NewGameSaveRuntime.h"

namespace {

std::vector<std::string> expectedNames() {
    std::vector<std::string> names;
    for (int i = 0; i < 52; ++i)
        names.push_back("d" + std::to_string(i) + ".dlv");
    names.push_back("oute3.ddm");
    for (int i = 1; i < 15; ++i)
        names.push_back("o" + std::to_string(i) + ".ddm");
    return names;
}

mm6::KnownSaveFields seed(std::int64_t ticks) {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = ticks;
    fields.core.party.activeCharacter = 1;
    fields.core.party.position = {-10178, -11206, 340};
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "New game";
    fields.metadata.header.mapFileName = std::string(mm6::NEW_GAME_FIRST_MAP);
    fields.metadata.header.playingTicks = ticks;
    fields.metadata.ui.activeCharacter = 1;
    fields.recovery.playingTicks = ticks;
    return fields;
}

std::vector<std::uint8_t> defaultDelta(std::string_view name) {
    return name.ends_with(".dlv")
        ? std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0)
        : std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

mm6::DefaultDeltaReader defaultReader() {
    return [](std::string_view name) {
        const std::vector<std::uint8_t> bytes = defaultDelta(name);
        return Blob::copy(bytes.data(), bytes.size());
    };
}

mm6::MapBaseReader baseReader() {
    return [](std::string_view name) {
        std::vector<std::uint8_t> bytes = name.ends_with(".blv")
            ? std::vector<std::uint8_t>(136 + 10 * 4, 0)
            : std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
        if (name.ends_with(".blv"))
            bytes[136 + 4 * 4] = 200;
        return Blob::copy(bytes.data(), bytes.size());
    };
}

#ifndef _WIN32
class TestProfile {
 public:
    TestProfile() {
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-newgame-XXXXXX").string();
        char *result = ::mkdtemp(pattern.data());
        if (!result)
            throw std::runtime_error("cannot create MM6 new-game test profile");
        _root = result;
    }

    ~TestProfile() {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    const std::filesystem::path &root() const { return _root; }
    std::filesystem::path primary() const { return _root / "saves" / "autosave.ec6"; }

 private:
    std::filesystem::path _root;
};

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

UNIT_TEST(MM6NewGameSaveRuntime, WritesAndPreflightsOnlyMM6FirstMapAutosave) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    int deltaReads = 0;
    auto readDefault = [&](std::string_view name) {
        ++deltaReads;
        return defaultReader()(name);
    };
    std::vector<std::uint8_t> bytes = mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(42), names, readDefault, baseReader(),
        [](std::int32_t id) { return id >= 1 && id <= 580; }, GameTitle::MM6);
    EXPECT_EQ(deltaReads, 67);
    EXPECT_EQ(readFile(profile.primary()), bytes);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).metadata.header.mapFileName, "oute3.odm");

    auto selected = mm6::preflightRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT,
        [&](std::span<const std::uint8_t> candidate) {
            (void) mm6::decodeKnownSaveWithActiveMapCounts(candidate, names, baseReader(),
                [](std::int32_t id) { return id >= 1 && id <= 580; });
        });
    ASSERT_TRUE(selected);
    EXPECT_FALSE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, bytes);

    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(43), names, readDefault, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM7), std::exception);
    mm6::KnownSaveFields wrongMap = seed(43);
    wrongMap.metadata.header.mapFileName = "d0.blv";
    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), wrongMap, names, readDefault, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6), std::exception);
    EXPECT_EQ(deltaReads, 67);
    EXPECT_EQ(readFile(profile.primary()), bytes);
}

UNIT_TEST(MM6NewGameSaveRuntime, LateFinalMapFailurePreservesPriorAutosave) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    std::vector<std::uint8_t> prior = mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(42), names, defaultReader(), baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6);
    int reads = 0;
    auto malformedLast = [&](std::string_view name) {
        ++reads;
        if (name == "o14.ddm") {
            const std::uint8_t byte = 1;
            return Blob::copy(&byte, 1);
        }
        return defaultReader()(name);
    };
    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(43), names, malformedLast, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6), std::exception);
    EXPECT_EQ(reads, 67);
    EXPECT_EQ(readFile(profile.primary()), prior);
}

UNIT_TEST(MM6NewGameSaveRuntime, BackupPreflightAndRecoveryAfterCorruptPrimary) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    const auto write = [&](std::int64_t ticks) {
        return mm6::writeOfflineNewGameAutosave(profile.root(), seed(ticks), names,
            defaultReader(), baseReader(), [](std::int32_t) { return true; }, GameTitle::MM6);
    };
    std::vector<std::uint8_t> prior = write(42);
    std::vector<std::uint8_t> next = write(43);
    EXPECT_NE(next, prior);
    {
        std::ofstream corrupt(profile.primary(), std::ios::binary | std::ios::trunc);
        corrupt << "broken";
    }
    auto selected = mm6::preflightRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT,
        [&](std::span<const std::uint8_t> candidate) {
            (void) mm6::decodeKnownSaveWithActiveMapCounts(candidate, names, baseReader(),
                [](std::int32_t) { return true; });
        });
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, prior);
    EXPECT_NE(readFile(profile.primary()), prior); // Preflight never mutates disk.
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT), prior);
    EXPECT_EQ(readFile(profile.primary()), prior);
}
#endif

} // namespace
