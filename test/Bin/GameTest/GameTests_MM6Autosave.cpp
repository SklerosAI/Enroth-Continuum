#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 autosave test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_AUTOSAVE_ENABLE");
    return flag && std::string_view(flag) == value;
}

static std::vector<std::uint8_t> fileBytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}


static std::string slotSummary(const std::filesystem::path &path) {
    // The EC6 version of a slot and the map it was saved on, or empty while there is no slot.
    if (!std::filesystem::exists(path))
        return {};
    const mm6::SaveContainer container = mm6::decodeSaveContainer(fileBytes(path));
    return fmt::format("v{} {}", container.version, mm6::decodeMetadataSections(container).header.mapFileName);
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void confirmTransition(EngineController &game) {
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void walkOverWestEdge(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // Next to the west edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);
}

static void standAtD06Door(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(16512, -14550, 100), 90); // South of the d06 door, facing it.
}

static void enterD06(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}




static std::string partySummary() {
    // Where the party stands, its gold and food, and every character's hit points, spell points and bag.
    std::string result = fmt::format("{} {} {} {} gold {} food {}", pParty->pos.x, pParty->pos.y, pParty->pos.z, pParty->_viewYaw,
                                     pParty->GetGold(), pParty->GetFood());
    for (const Character &character : pParty->pCharacters) {
        result += fmt::format(" | hp {} sp {}", character.health, character.mana);
        for (InventoryConstEntry entry : character.inventory.entries())
            result += fmt::format(" {}@{},{}", std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y);
    }
    return result;
}


GAME_TEST(MM6AutosaveHeadless, DISABLED_EdgeAndDoorWriteTheAutosave) {
    // MM6 never wrote an autosave, the original writes one for a new party and on every trip to another map.
    if (!enabled("strict-autosave-write"))
        GTEST_SKIP() << "Run only as the first autosave child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path autosave = profile / "saves/autosave.ec6";
    ASSERT_FALSE(std::filesystem::exists(autosave));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId d06 = pMapTable->GetMapInfo("d06.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto autosaveTape = tapes.custom([&] { return slotSummary(autosave); });
    auto backupTape = tapes.custom([&] { return slotSummary(profile / "saves/autosave.ec6.bak"); });

    walkOverWestEdge(game);
    standAtD06Door(game);
    const std::string party = partySummary();
    enterD06(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3, d06));
    EXPECT_EQ(autosaveTape, tape("v5 oute3.odm", "v7 oute3.odm", "v7 outd3.odm")); // A new party on the first map, then each map the party leaves.
    EXPECT_EQ(backupTape, tape("", "v5 oute3.odm", "v7 oute3.odm"));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-autosave.txt") << party << "\n";
}
