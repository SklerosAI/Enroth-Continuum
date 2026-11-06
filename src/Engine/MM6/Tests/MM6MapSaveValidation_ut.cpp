#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MapSaveValidation.h"

namespace {

std::vector<std::uint8_t> indoorBase() {
    std::vector<std::uint8_t> bytes(136 + 10 * 4, 0);
    bytes[136 + 4 * 4] = 200; // Fixed MM6 door slots.
    return bytes;
}

std::vector<std::uint8_t> indoorDelta() {
    return std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorBase() {
    return std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
}

std::vector<std::uint8_t> outdoorDelta() {
    return std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

std::vector<std::uint8_t> outdoorDeltaWithItem(std::int32_t itemId) {
    constexpr size_t groundCount = 8 + 2 * 88 * 11 + 4;
    std::vector<std::uint8_t> bytes(8 + 2 * 88 * 11 + 3 * 4 + 100 + 200 + 56, 0);
    bytes[groundCount] = 1;
    const size_t itemOffset = groundCount + 4 + 36;
    for (int shift = 0; shift < 32; shift += 8)
        bytes[itemOffset + shift / 8] = static_cast<std::uint8_t>(itemId >> shift);
    return bytes;
}

void writeItemId(std::vector<std::uint8_t> *bytes, size_t offset, std::int32_t itemId) {
    for (int shift = 0; shift < 32; shift += 8)
        (*bytes)[offset + shift / 8] = static_cast<std::uint8_t>(itemId >> shift);
}

std::vector<std::uint8_t> deltaWithChestItem(bool indoor, std::int32_t itemId) {
    constexpr size_t chestSize = 4204;
    const size_t header = indoor ? 8 + 875 : 8 + 2 * 88 * 11;
    const size_t doors = indoor ? 200 * 80 : 0;
    std::vector<std::uint8_t> bytes(header + 3 * 4 + chestSize + doors + 200 + 56, 0);
    bytes[header + 8] = 1; // One 4204-byte chest after zero actors and objects.
    writeItemId(&bytes, header + 3 * 4 + 4, itemId); // First 28-byte item slot.
    return bytes;
}

bool validGroundItem(std::int32_t id) { return id >= 1 && id <= 580; }

std::set<std::int32_t> retainedItemTemplates(const std::filesystem::path &researchDir) {
    std::ifstream stream(researchDir / "icons/items.txt");
    if (!stream)
        throw std::runtime_error("MM6 retained item table is missing");
    std::set<std::int32_t> ids;
    std::string line;
    while (std::getline(stream, line)) {
        const size_t firstTab = line.find('\t');
        if (firstTab == std::string::npos)
            continue;
        const size_t secondTab = line.find('\t', firstTab + 1);
        if (secondTab == std::string::npos)
            continue;
        const std::string idText = line.substr(0, firstTab);
        if (idText.empty() || !std::all_of(idText.begin(), idText.end(),
                                          [](unsigned char c) { return std::isdigit(c); }))
            continue;
        const int id = std::stoi(idText);
        if (id >= 1 && id <= 580 && secondTab > firstTab + 1 &&
            line.substr(firstTab + 1, secondTab - firstTab - 1) != "\"\"")
            ids.insert(id);
    }
    if (ids.size() != 578)
        throw std::runtime_error("MM6 retained item template coverage changed");
    return ids;
}

mm6::KnownSaveFields exampleFields() {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "Offline test";
    fields.metadata.header.mapFileName = "synthetic0.blv";
    fields.metadata.header.playingTicks = 42;
    fields.metadata.ui.activeCharacter = 2;
    fields.recovery.playingTicks = 42;
    fields.npcs.activeCount = 399;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"synthetic" + std::to_string(i) + ".dlv", indoorDelta()});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"outside" + std::to_string(i) + ".ddm", outdoorDelta()});
    fields.chest = {17, {}, "synthetic0.dlv", indoorDelta()};
    return fields;
}

std::vector<std::string> namesOf(const mm6::KnownSaveFields &fields) {
    std::vector<std::string> names;
    for (const mm6::SavedMapDelta &delta : fields.maps.deltas)
        names.push_back(delta.name);
    return names;
}

Blob syntheticBase(std::string_view name) {
    const std::vector<std::uint8_t> bytes = name.ends_with(".blv") ? indoorBase() : outdoorBase();
    return Blob::copy(bytes.data(), bytes.size());
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("MM6 retained map fixture is missing");
    std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    return {bytes.begin(), bytes.end()};
}

} // namespace

UNIT_TEST(MM6MapSaveValidation, StagesWholeKnownSaveOnlyAfterEveryMapPasses) {
    mm6::KnownSaveFields source = exampleFields();
    const std::vector<std::string> expected = namesOf(source);
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    mm6::KnownSaveFields destination;
    destination.core.party.playingTicks = 999;
    mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase, validGroundItem, &destination);
    EXPECT_EQ(destination.core.party.playingTicks, 42);
    EXPECT_EQ(destination.maps, source.maps);

    source.maps.deltas[1].name = "absent.dlv";
    bytes = mm6::encodeKnownSave(source);
    destination.core.party.playingTicks = 999;
    int reads = 0;
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected,
        [&reads](std::string_view name) { ++reads; return syntheticBase(name); }, validGroundItem, &destination),
        std::exception);
    EXPECT_EQ(reads, 0);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source = exampleFields();
    source.maps.deltas.back().bytes.pop_back();
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
                                                              validGroundItem, &destination),
                 std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source = exampleFields();
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected,
        [](std::string_view) { return Blob(); }, validGroundItem, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected,
        [](std::string_view) {
            const std::vector<std::uint8_t> bytes = outdoorBase();
            return Blob::copy(bytes.data(), bytes.size());
        }, validGroundItem, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);
}

UNIT_TEST(MM6MapSaveValidation, LateChestSidecarMismatchLeavesKnownDestinationUntouched) {
    mm6::KnownSaveFields source = exampleFields();
    const std::vector<std::string> expected = namesOf(source);
    source.chest.rawActiveDelta = deltaWithChestItem(true, 42); // Valid wire, different chest count.
    const std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    mm6::KnownSaveFields destination = exampleFields();
    destination.core.party.playingTicks = 901;
    destination.chest.lcgState = 0xabcdef01;
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(
        bytes, expected, syntheticBase, validGroundItem, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 901);
    EXPECT_EQ(destination.chest.lcgState, 0xabcdef01);
    EXPECT_EQ(destination.chest.rawActiveDelta, indoorDelta());
}

UNIT_TEST(MM6MapSaveValidation, NonzeroGroundItemRequiresMM6TemplateBeforeMutation) {
    mm6::KnownSaveFields source = exampleFields();
    const std::vector<std::string> expected = namesOf(source);
    source.maps.deltas.back().bytes = outdoorDeltaWithItem(42);
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    mm6::KnownSaveFields destination;
    destination.core.party.playingTicks = 999;
    int verified = 0;
    mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [&verified](std::int32_t id) { ++verified; return id == 42; }, &destination);
    EXPECT_EQ(verified, 1);
    EXPECT_EQ(destination.maps.deltas.back().bytes, source.maps.deltas.back().bytes);

    destination.core.party.playingTicks = 999;
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t) { return false; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source.maps.deltas.back().bytes = outdoorDeltaWithItem(581);
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t) { return true; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source.maps.deltas.back().bytes = outdoorDeltaWithItem(-1);
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t) { return true; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        {}, &destination), std::exception);
}

UNIT_TEST(MM6MapSaveValidation, FixedChestItemSlotsRequireMM6TemplateBeforeMutation) {
    mm6::KnownSaveFields source = exampleFields();
    const std::vector<std::string> expected = namesOf(source);
    source.maps.deltas.front().bytes = deltaWithChestItem(true, 42);
    source.chest.rawActiveDelta = source.maps.deltas.front().bytes;
    source.maps.deltas.back().bytes = deltaWithChestItem(false, 43);
    std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    mm6::KnownSaveFields destination;
    destination.core.party.playingTicks = 999;
    std::vector<std::int32_t> verified;
    mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [&verified](std::int32_t id) { verified.push_back(id); return id == 42 || id == 43; },
        &destination);
    // The active chest item is checked in both MAPS and the CHST sidecar.
    EXPECT_EQ(verified, (std::vector<std::int32_t>{42, 42, 43}));
    EXPECT_EQ(destination.maps, source.maps);

    destination.core.party.playingTicks = 999;
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t id) { return id == 42; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source.maps.deltas.back().bytes = deltaWithChestItem(false, 581);
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t) { return true; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    source.maps.deltas.back().bytes = deltaWithChestItem(false, -6);
    bytes = mm6::encodeKnownSave(source);
    int randomTemplateCalls = 0;
    mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [&randomTemplateCalls](std::int32_t id) {
            ++randomTemplateCalls;
            return id == 42;
        }, &destination);
    EXPECT_EQ(randomTemplateCalls, 2);
    EXPECT_EQ(destination.maps, source.maps);

    destination.core.party.playingTicks = 999;
    source.maps.deltas.back().bytes = deltaWithChestItem(false, -7);
    bytes = mm6::encodeKnownSave(source);
    EXPECT_THROW(mm6::loadKnownSaveWithMapBasesBeforeMutation(bytes, expected, syntheticBase,
        [](std::int32_t) { return true; }, &destination), std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);
}

UNIT_TEST(MM6MapSaveValidation, RetainedSixtySevenBaseDeltaPairs) {
    const char *indoor = std::getenv("ENROTH_MM6_INDOOR_DECODED_DIR");
    const char *outdoor = std::getenv("ENROTH_MM6_OUTDOOR_DECODED_DIR");
    if (!indoor || !outdoor)
        GTEST_SKIP() << "Set ignored decoded MM6 indoor and outdoor fixture directories";

    const std::filesystem::path indoorDir(indoor);
    const std::filesystem::path outdoorDir(outdoor);
    const std::set<std::int32_t> templates = retainedItemTemplates(indoorDir.parent_path());
    mm6::KnownSaveFields source = exampleFields();
    source.maps.deltas.clear();
    for (const auto &[directory, suffix] : {std::pair{indoorDir, ".dlv"}, std::pair{outdoorDir, ".ddm"}}) {
        for (const auto &entry : std::filesystem::directory_iterator(directory)) {
            if (entry.path().extension() == suffix)
                source.maps.deltas.push_back({entry.path().filename().string(), readBytes(entry.path())});
        }
    }
    ASSERT_EQ(source.maps.deltas.size(), 67);
    source.metadata.header.mapFileName = source.maps.deltas.front().name;
    source.metadata.header.mapFileName.replace(source.metadata.header.mapFileName.size() - 4, 4,
        source.metadata.header.mapFileName.ends_with(".dlv") ? ".blv" : ".odm");
    source.chest.activeDeltaName = source.maps.deltas.front().name;
    source.chest.rawActiveDelta = source.maps.deltas.front().bytes;
    const std::vector<std::string> expected = namesOf(source);
    const std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(source);
    int reads = 0;
    mm6::KnownSaveFields restored = mm6::decodeKnownSaveWithMapBases(bytes, expected,
        [&](std::string_view baseName) {
            ++reads;
            const auto &directory = baseName.ends_with(".blv") ? indoorDir : outdoorDir;
            const std::vector<std::uint8_t> base = readBytes(directory / baseName);
            return Blob::copy(base.data(), base.size());
        }, [&templates](std::int32_t id) { return templates.contains(id); });
    EXPECT_EQ(reads, 67);
    EXPECT_EQ(restored.maps, source.maps);
}
