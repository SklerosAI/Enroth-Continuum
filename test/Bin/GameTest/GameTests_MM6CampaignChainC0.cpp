#include <filesystem>
#include <set>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/Tables/MapTable.h"

#include "MM6CampaignChain.h"

static void createParty(EngineController &game) {
    // Each character takes its first three optional skills, two of which fill the free slots, and up to three points
    // in every attribute, until the party has no points left.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
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
}

using SaveFiles = std::set<std::string>;

static SaveFiles saveFiles() {
    SaveFiles result;
    const std::filesystem::path saves = mm6CampaignChainProfile() / "saves";
    if (std::filesystem::is_directory(saves))
        for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(saves))
            result.insert(entry.path().filename().string());
    return result;
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter0) {
    // Fixtures: none. A party made on the creation screen enters the first map and saves there, the entry state of C1.
    assertMM6CampaignChainProfile();
    ASSERT_EQ(saveFiles(), SaveFiles());

    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    auto statusTape = tapes.statusBar();
    auto armedTape = tapes.custom([] { return mm6::isFirstMapManualSaveArmed(); });
    auto savesTape = tapes.custom([] { return saveFiles(); });

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_EQ(screenTape.back(), SCREEN_PARTY_CREATION);
    createParty(game);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAME);
    saveMM6CampaignChainSlot(game, 0);

    EXPECT_EQ(mapTape, tape(MAP_INVALID, pMapTable->GetMapInfo("oute3.odm")));
    EXPECT_EQ(armedTape, tape(false, true));
    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_EQ(savesTape, tape(SaveFiles(), SaveFiles{"autosave.ec6"}, SaveFiles{"autosave.ec6", "save000.ec6"}));
}
