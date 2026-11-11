#include <unistd.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "Utility/ScopeGuard.h"

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

static void teleportToMap(EngineController &game, const char *map, Vec3f position, int viewYaw) {
    // A test teleport loads the map the way a trip there would, with its chests.
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.teleportTo(pMapTable->GetMapInfo(map), position, viewYaw);
    game.runGameRoutine([&] { chestOptIn.reset(); });
}

static void enterThroughDoor(EngineController &game, Vec3f position, int viewYaw) {
    // Same map teleport, then Space at the door in front of the party.
    game.teleportTo(engine->_currentLoadedMapId, position, viewYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void takeExit(EngineController &game) {
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static std::vector<std::string> portraitLabels() {
    // Hints of the portraits the house screen lists.
    std::vector<std::string> result;
    if (window_SpeakInHouse)
        for (GUIButton *button : window_SpeakInHouse->vButtons)
            if (button->id.starts_with("House_Npc"))
                result.push_back(button->label);
    return result;
}

static std::string autosaveMap(const std::filesystem::path &profile) {
    // Map the autosave was written on, empty while there is none.
    const std::filesystem::path path = profile / "saves/autosave.ec6";
    if (!std::filesystem::exists(path))
        return {};
    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    return mm6::decodeMetadataSections(mm6::decodeSaveContainer(bytes)).header.mapFileName;
}

GAME_TEST(MM6HouseExit, HighCouncilExitNeedsItsQuestBit) {
    // The High Council never offered its way down to the Oracle, so a party with the council's approval couldn't reach
    // it. The way shows only once the approval's quest bit is set.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    teleportToMap(game, "outc2.odm", Vec3f(12338, 5368, 320), 0);
    pNPCStats->pNPCData[304].house = HOUSE_INVALID; // Exposing the traitor sends Slicker Silvertongue away.
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    auto portraitTape = tapes.custom([] { return portraitLabels(); });
    auto bitTape = tapes.questBit(static_cast<QuestBit>(167));
    auto positionTape = tapes.custom([] { return pParty->pos; });
    test.startTaping();

    const std::string enterOracle = localization->format(LSTR_ENTER_S, pMapTable->pInfos[oracle].name);
    for (bool approved : {false, true}) {
        if (approved)
            pParty->_questBits.set(static_cast<QuestBit>(167)); // The council's approval.
        enterThroughDoor(game, Vec3f(12338, 5368, 320), 0);
        ASSERT_EQ(houseTape.back(), static_cast<HouseId>(165));
        EXPECT_EQ(portraitTape.back().size(), approved ? 6 : 5);
        EXPECT_EQ(portraitTape.back().back() == enterOracle, approved);
        if (!approved) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    game.pressGuiButton("House_Npc5");
    game.tick();
    takeExit(game);

    EXPECT_EQ(mapTape, tape(outc2, oracle));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(165), HOUSE_INVALID, static_cast<HouseId>(165), HOUSE_INVALID));
    EXPECT_EQ(bitTape, tape(false, true));
    EXPECT_EQ(positionTape.back(), Vec3f(7168, -6176, 0.5f)); // The Oracle's party start.
}

GAME_TEST(MM6HouseExit, SewerHouseArrival) {
    // MM6 houses with a trapdoor into the Free Haven sewer offered no way down. Takao's house takes the party to its
    // own spot in the sewer and autosaves on the way.
    char profileTemplate[] = "mm6-restart-house-exit-XXXXXX";
    std::string created = (std::filesystem::canonical(std::filesystem::temp_directory_path()) / profileTemplate).string();
    ASSERT_NE(mkdtemp(created.data()), nullptr);
    const std::filesystem::path profile = std::filesystem::canonical(created);
    MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId outc2 = pMapTable->GetMapInfo("outc2.odm");
    const MapId sewer = pMapTable->GetMapInfo("sewer.blv");
    teleportToMap(game, "outc2.odm", Vec3f(5304, 16806, 256), 270);
    std::filesystem::remove(profile / "saves/autosave.ec6"); // The new party's autosave.
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto portraitTape = tapes.custom([] { return portraitLabels(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto yawTape = tapes.custom([] { return pParty->_viewYaw; });
    auto autosaveTape = tapes.custom([&] { return autosaveMap(profile); });
    test.startTaping();

    enterThroughDoor(game, Vec3f(5304, 16806, 256), 270);
    const std::vector<std::string> portraits = {localization->format(LSTR_CONVERSE_WITH_S, "Takao"),
                                                localization->format(LSTR_ENTER_S, pMapTable->pInfos[sewer].name)};
    ASSERT_EQ(portraitTape.back(), portraits);
    game.pressGuiButton("House_Npc1");
    game.tick();
    EXPECT_EQ(portraitTape.back(), std::vector<std::string>());
    game.pressGuiButton("HouseExit_Cancel");
    game.tick();
    EXPECT_EQ(portraitTape.back(), portraits);
    game.pressGuiButton("House_Npc1");
    game.tick();
    takeExit(game);

    EXPECT_EQ(mapTape, tape(outc2, sewer));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(positionTape.back(), Vec3f(-6575, 13740, 176.5f)); // The arrival, settled on the floor.
    EXPECT_EQ(yawTape.back(), 1536);
    EXPECT_EQ(autosaveTape, tape(std::string(), std::string("outc2.odm")));
    game.runGameRoutine([] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}

GAME_TEST(MM6HouseExit, KingsLibraryNeedsTheBell) {
    // Bringing Tanir's Bell to the King's Library never woke Archibald, the room stayed on his statue. Without the bell
    // it still does.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    engine->config->debug.NoVideo.setValue(false); // The library shows nothing but its movie.
    teleportToMap(game, "outd3.odm", Vec3f(17160, 16626, 2784), 90);
    auto houseTape = tapes.house();
    test.startTaping();

    for (bool bell : {false, true}) {
        if (bell)
            pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(461)));
        enterThroughDoor(game, Vec3f(17160, 16626, 2784), 90);
        ASSERT_EQ(houseTape.back(), static_cast<HouseId>(168));
        game.tick(34); // The bell's movie plays for 3.4 seconds.
        EXPECT_EQ(houseTape.back(), static_cast<HouseId>(bell ? 553 : 168));
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(168), HOUSE_INVALID, static_cast<HouseId>(168),
                              static_cast<HouseId>(553), HOUSE_INVALID));
}

GAME_TEST(MM6HouseExit, ArchibaldLeavesTheLibrary) {
    // Archibald's room never emptied after his release. Once his topic sends him away, the party is in the empty room,
    // and the library door leads there from then on.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    engine->config->debug.NoVideo.setValue(false); // The library shows nothing but its movie.
    teleportToMap(game, "outd3.odm", Vec3f(17160, 16626, 2784), 90);
    pParty->pCharacters[0].inventory.add(Item(static_cast<ItemId>(461)));
    auto houseTape = tapes.house();
    auto releasedTape = tapes.questBit(static_cast<QuestBit>(177));
    auto archibaldTape = tapes.custom([] { return pNPCStats->pNPCData[12].house; });
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    test.startTaping();

    enterThroughDoor(game, Vec3f(17160, 16626, 2784), 90);
    game.tick(34); // The bell's movie plays for 3.4 seconds.
    ASSERT_EQ(houseTape.back(), static_cast<HouseId>(553));
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_EQ(houseTape.back(), static_cast<HouseId>(553));
    game.tick(3); // His room plays out its last pass.
    EXPECT_EQ(houseTape.back(), static_cast<HouseId>(554));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    enterThroughDoor(game, Vec3f(17160, 16626, 2784), 90);

    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(168), static_cast<HouseId>(553), static_cast<HouseId>(554),
                              HOUSE_INVALID, static_cast<HouseId>(554)));
    EXPECT_EQ(releasedTape, tape(false, true));
    EXPECT_EQ(archibaldTape, tape(static_cast<HouseId>(553), HOUSE_INVALID));
    EXPECT_EQ(heldTape, tape(ITEM_NULL, static_cast<ItemId>(544))); // His reward.
}
