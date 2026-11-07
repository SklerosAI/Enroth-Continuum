#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6IndoorFaceOverrides.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/MapTable.h"

namespace {

constexpr std::string_view slotName = "indoor-ifov-probe.ec6";

std::filesystem::path ownerOnlyProfile(bool createSaves) {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 indoor probe needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 indoor probe needs an owner-only system-temp profile");
    const std::filesystem::path saves = root / "saves";
    if (createSaves && !std::filesystem::exists(saves)) {
        std::filesystem::create_directory(saves);
        std::filesystem::permissions(saves, std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::replace);
    }
    if (!std::filesystem::is_directory(saves) || std::filesystem::is_symlink(saves) ||
        std::filesystem::canonical(saves).parent_path() != root)
        throw std::runtime_error("MM6 indoor probe needs a local saves directory");
    return root;
}

struct IndoorSourceFaces {
    std::string mapName;
    std::vector<mm6::IndoorFaceState> faces;
};

IndoorSourceFaces loadIndoorSourceFaces() {
    const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto names = maps.expectedNames();
    const auto entry = std::find_if(names.begin(), names.end(), [](const std::string &name) {
        return name.ends_with(".dlv");
    });
    if (entry == names.end())
        throw std::runtime_error("MM6 indoor probe has no retained DLV pair");
    const std::string mapName = entry->substr(0, entry->size() - 4) + ".blv";
    const MM6IndoorSnapshot base = deserializeMM6IndoorBase(maps.readBase(mapName));
    const MM6IndoorRuntimeDelta delta = decodeMM6IndoorRuntimeDelta(
        decodeMM6ArchiveEntry(maps.readDefaultDelta(*entry)), base);
    const IndoorLocation_MM7 projected = projectMM6IndoorBase(base);
    if (projected.faces.empty() || projected.faces.size() != projected.faceTextures.size() ||
        delta.doors.size() != projected.doorCount)
        throw std::runtime_error("MM6 indoor probe lacks a paired source face and DLV shape");
    IndoorSourceFaces result{mapName, {}};
    result.faces.reserve(projected.faces.size());
    for (std::size_t i = 0; i < projected.faces.size(); ++i) {
        const auto &name = projected.faceTextures[i];
        const auto end = std::find(name.begin(), name.end(), '\0');
        result.faces.push_back({projected.faces[i].attributes,
                                std::string(name.begin(), end)});
    }
    return result;
}

std::filesystem::path probeSlot(const std::filesystem::path &profile) {
    return profile / "saves" / slotName;
}

std::vector<std::uint8_t> readProbe(const std::filesystem::path &profile) {
    const std::filesystem::path slot = probeSlot(profile);
    if (!std::filesystem::is_regular_file(slot) || std::filesystem::is_symlink(slot) ||
        std::filesystem::file_size(slot) > 2 * 1024 * 1024)
        throw std::runtime_error("MM6 indoor probe slot is missing or outside its bound");
    std::ifstream input(slot, std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 indoor probe slot cannot be read");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void requireHeadless() {
    if (engine->gameTitle() != GameTitle::MM6 || window->nativeHandle() != nullptr ||
        dynamic_cast<NullRenderer *>(render) == nullptr)
        throw std::runtime_error("MM6 indoor probe requires a null-renderer MM6 process");
}

} // namespace

// This is an isolated IFOV owner proof using a real, private BLV/DLV pair.
// The controlled face edit is detached; the test does not claim a live indoor
// save, pre-OnMapReload provenance, or a playable indoor campaign.
GAME_TEST(MM6IndoorFaceFreshChild, DISABLED_WriteSourceAnchoredFaceOwner) {
    const char *enable = std::getenv("ENROTH_MM6_INDOOR_IFOV_WRITE_ENABLE");
    if (!enable || std::string_view(enable) != "strict-indoor-ifov")
        GTEST_SKIP() << "Opt in to the disposable indoor face writer";
    const auto profile = ownerOnlyProfile(true);
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    requireHeadless();
    const auto source = loadIndoorSourceFaces();
    auto changed = source.faces;
    changed.front().attributes ^= 1u;
    const auto override = mm6::captureIndoorFaceOverrides(
        source.mapName, source.faces, changed, GameTitle::MM6);
    ASSERT_TRUE(override);
    ASSERT_EQ(override->faces.size(), 1u);
    mm6::IndoorFaceOverrides fields{{*override}};
    const auto bytes = mm6::encodeSaveContainer(
        {{mm6::encodeIndoorFaceOverrideSection(fields)}});
    ASSERT_EQ(mm6::decodeIndoorFaceOverrideSection(mm6::decodeSaveContainer(bytes)), fields);
    const auto slot = probeSlot(profile);
    ASSERT_FALSE(std::filesystem::exists(slot));
    const auto pending = profile / "saves" / "indoor-ifov-probe.pending";
    ASSERT_FALSE(std::filesystem::exists(pending));
    {
        std::ofstream output(pending, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(output);
        output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        ASSERT_TRUE(output);
    }
    std::filesystem::permissions(pending, std::filesystem::perms::owner_read |
        std::filesystem::perms::owner_write, std::filesystem::perm_options::replace);
    std::filesystem::rename(pending, slot);
    EXPECT_EQ(readProbe(profile), bytes);
}

// Run in a fresh child after the writer. It reconstructs the independent BLV
// base and DLV, checks the IFOV source fingerprint, and proves field precedence
// without mutating the engine, installed data, or any existing save slot.
GAME_TEST(MM6IndoorFaceFreshChild, DISABLED_ReadSourceAnchoredFaceOwner) {
    const char *enable = std::getenv("ENROTH_MM6_INDOOR_IFOV_READ_ENABLE");
    if (!enable || std::string_view(enable) != "strict-indoor-ifov")
        GTEST_SKIP() << "Opt in to the disposable indoor face reader";
    const auto profile = ownerOnlyProfile(false);
    if (profile.empty())
        GTEST_SKIP() << "Set an owner-only system-temp MM6 restart profile";
    requireHeadless();
    const auto before = readProbe(profile);
    const auto source = loadIndoorSourceFaces();
    mm6::IndoorFaceOverrides staged;
    mm6::loadIndoorFaceOverridesBeforeMutation(
        before,
        [&source](std::string_view mapName) {
            if (mapName != source.mapName)
                throw std::runtime_error("MM6 indoor probe IFOV refers to another base map");
            return source.faces;
        },
        &staged, GameTitle::MM6);
    ASSERT_EQ(staged.maps.size(), 1u);
    ASSERT_EQ(staged.maps.front().faces.size(), 1u);
    auto afterReload = source.faces;
    if (afterReload.size() > 1)
        afterReload[1].attributes ^= 2u; // An unrelated reload-script field survives.
    const auto expectedOther = afterReload.size() > 1 ? afterReload[1].attributes : 0u;
    mm6::applyIndoorFaceOverridesAfterMapReload(
        staged, source.mapName, &afterReload, GameTitle::MM6);
    EXPECT_EQ(afterReload.front().attributes, source.faces.front().attributes ^ 1u);
    EXPECT_EQ(afterReload.front().textureName, source.faces.front().textureName);
    if (afterReload.size() > 1)
        EXPECT_EQ(afterReload[1].attributes, expectedOther);
    EXPECT_EQ(readProbe(profile), before);
}
