#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PrivateMapSource.h"

namespace {

constexpr const char *REPORTS[] = {
    "F3_GATE_REPORT_HASHES_2026-11-06.json",
    "F3_IDENTITY_2026-11-05.json",
    "F3_CAPTURE_2026-11-05.json",
    "F3_CONTENT_RECONCILIATION_2026-11-05.json",
    "F3_CONTENT_SCHEMA_SIGNOFF_2026-11-05.json",
    "map-probe.json",
    "MM6_EC6_MAP_NAME_MANIFEST_F4.json",
};

std::vector<std::uint8_t> readBytes(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("MM6 retained map test file is missing");
    const std::string bytes(std::istreambuf_iterator<char>{stream}, {});
    return {bytes.begin(), bytes.end()};
}

std::string baseName(std::string_view deltaName) {
    return std::string(deltaName.substr(0, deltaName.size() - 4)) +
           (deltaName.ends_with(".dlv") ? ".blv" : ".odm");
}

mm6::KnownSaveFields knownFields(const std::filesystem::path &root,
                                 std::span<const std::string> names) {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = 42;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "Offline test";
    fields.metadata.header.mapFileName = baseName(names.front());
    fields.metadata.header.playingTicks = 42;
    fields.metadata.ui.activeCharacter = 2;
    fields.recovery.playingTicks = 42;
    fields.npcs.activeCount = 399;
    for (const std::string &name : names) {
        const std::filesystem::path directory = name.ends_with(".dlv") ? "F4_INDOOR_DECODED" :
                                                                          "F4_OUTDOOR_DECODED";
        fields.maps.deltas.push_back({name, readBytes(root / directory / name)});
    }
    fields.chest = {17, {}, names.front(), fields.maps.deltas.front().bytes};
    return fields;
}

class ScratchRoot {
 public:
    explicit ScratchRoot(const std::filesystem::path &source) {
        const auto id = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("mm6-private-map-test-" + std::to_string(id));
        std::filesystem::create_directory(path);
        for (const char *name : REPORTS)
            std::filesystem::copy_file(source / name, path / name);
        std::filesystem::permissions(path / REPORTS[6],
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace);
    }

    ~ScratchRoot() { std::filesystem::remove_all(path); }
    ScratchRoot(const ScratchRoot &) = delete;
    ScratchRoot &operator=(const ScratchRoot &) = delete;

    std::filesystem::path path;
};

} // namespace

UNIT_TEST(MM6PrivateMapSource, F3IdentityContainmentAndStaging) {
    const char *privateRoot = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!privateRoot)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path root(privateRoot);
    mm6::PrivateMapSource source(root);
    ASSERT_EQ(source.expectedNames().size(), 67);
    EXPECT_FALSE(source.readBase(baseName(source.expectedNames().front())).empty());
    EXPECT_THROW((void) source.readBase("../escape.blv"), std::exception);

    mm6::KnownSaveFields fields = knownFields(root, source.expectedNames());
    const std::vector<std::uint8_t> bytes = mm6::encodeKnownSave(fields);
    mm6::KnownSaveFields destination;
    destination.core.party.playingTicks = 999;
    mm6::loadKnownSaveWithPrivateMapsBeforeMutation(bytes, root,
        [](std::int32_t id) { return id >= 1 && id <= 580; }, &destination);
    EXPECT_EQ(destination.core.party.playingTicks, 42);
    EXPECT_EQ(destination.maps, fields.maps);

    ScratchRoot scratch(root);
    std::filesystem::create_directory_symlink(root / "F4_INDOOR_DECODED", scratch.path / "F4_INDOOR_DECODED");
    std::filesystem::create_directory_symlink(root / "F4_OUTDOOR_DECODED", scratch.path / "F4_OUTDOOR_DECODED");
    mm6::PrivateMapSource escaped(scratch.path);
    EXPECT_THROW((void) escaped.readBase(baseName(escaped.expectedNames().front())), std::exception);
    destination.core.party.playingTicks = 999;
    EXPECT_THROW(mm6::loadKnownSaveWithPrivateMapsBeforeMutation(bytes, scratch.path,
        [](std::int32_t id) { return id >= 1 && id <= 580; }, &destination),
                 std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);

    const std::filesystem::path manifest = scratch.path / REPORTS[6];
    std::ifstream manifestInput(manifest, std::ios::binary);
    std::string manifestBytes(std::istreambuf_iterator<char>{manifestInput}, {});
    manifestInput.close();
    const size_t namesHash = manifestBytes.find("\"names_sha256\": \"");
    ASSERT_NE(namesHash, std::string::npos);
    const size_t firstDigit = namesHash + sizeof("\"names_sha256\": \"") - 1;
    manifestBytes[firstDigit] = manifestBytes[firstDigit] == 'a' ? 'b' : 'a';
    std::ofstream manifestOutput(manifest, std::ios::binary | std::ios::trunc);
    manifestOutput.write(manifestBytes.data(), manifestBytes.size());
    manifestOutput.close();
    EXPECT_THROW((void) mm6::PrivateMapSource(scratch.path), std::exception);
    std::filesystem::copy_file(root / REPORTS[6], manifest,
                               std::filesystem::copy_options::overwrite_existing);

    std::ofstream tamper(scratch.path / REPORTS[2], std::ios::binary | std::ios::app);
    tamper.put('x');
    tamper.close();
    EXPECT_THROW((void) mm6::PrivateMapSource(scratch.path), std::exception);
    EXPECT_THROW(mm6::loadKnownSaveWithPrivateMapsBeforeMutation(bytes, scratch.path,
        [](std::int32_t id) { return id >= 1 && id <= 580; }, &destination),
                 std::exception);
    EXPECT_EQ(destination.core.party.playingTicks, 999);
}
