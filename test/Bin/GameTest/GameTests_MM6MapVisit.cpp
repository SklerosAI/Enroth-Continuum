#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "GUI/UI/UISaveLoad.h"

static constexpr std::array<std::string_view, 16> mm6Skies = {
    "sky01", "sky02", "sky03", "sky05", "sky06", "sky07", "sky08", "sky09", "sky10", "sky12", "sky13", "sky14", "sky15", "sky16", "sky18", "sky19"};

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 visit test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_VISIT_ENABLE");
    return flag && std::string_view(flag) == value;
}

static std::string fileText(const std::filesystem::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
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

static void walkOverEdge(EngineController &game, MapId map, Vec3f pos, int yaw) {
    game.teleportTo(map, pos, yaw); // Next to the edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    confirmTransition(game);
}

static void toOutd3AndBack(EngineController &game) {
    // Five days of travel each way.
    walkOverEdge(game, pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // The west edge.
    walkOverEdge(game, pMapTable->GetMapInfo("outd3.odm"), Vec3f(22450, -10000, 164), 0); // The east edge.
}

static void toD02AndBack(EngineController &game) {
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
    game.teleportTo(d02, Vec3f(16384, -19626, 864.5f), 270); // In front of the way out, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static std::string outdoorSky() {
    return uCurrentlyLoadedLevelType == LEVEL_OUTDOOR ? pOutdoor->weather.skyTextureName : std::string();
}

static Time loadedMapLastVisit() {
    return uCurrentlyLoadedLevelType == LEVEL_INDOOR ? pIndoor->lastVisitTime : pOutdoor->lastVisitTime;
}

static void faceFountain(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-10028, -7168, 0), 0); // West of the Sorpigal fountain, facing it.
    game.pointMouseAtFace(863); // Its water.
}

static void drinkFromFountain(EngineController &game) {
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

GAME_TEST(MM6MapVisit, LaterDayReturnRollsTheSky) {
    // An MM6 map never recorded the party leaving it, so a return days later was a first visit again, under sky01.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto timeTape = tapes.time();
    auto skyTape = tapes.custom([] { return outdoorSky(); });
    auto visitTape = tapes.custom([] { return loadedMapLastVisit(); });
    test.startTaping();

    toOutd3AndBack(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3, oute3));
    EXPECT_EQ(skyTape, tape("sky01", "", "sky01", "", "sky18")); // Both first visits are sky01, the loading screens have no sky.
    EXPECT_TRUE(std::ranges::contains(mm6Skies, skyTape.back()));
    ASSERT_EQ(visitTape.size(), 2);
    EXPECT_EQ(visitTape[0], Time());
    EXPECT_CONTAINS(timeTape, visitTape[1]); // The party left at the edge, before the trip.
    EXPECT_CONTAINS(timeTape, visitTape[1] + Duration::fromDays(5));
}

GAME_TEST(MM6MapVisit, SameDayReturnKeepsTheSky) {
    // An MM6 map never recorded the party leaving it, so a return on the same day was a first visit again, under sky01
    // instead of the sky the party left.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    toOutd3AndBack(game);
    auto mapTape = tapes.map();
    auto timeTape = tapes.time();
    auto skyTape = tapes.custom([] { return outdoorSky(); });
    auto visitTape = tapes.custom([] { return loadedMapLastVisit(); });
    test.startTaping();

    toD02AndBack(game);

    EXPECT_EQ(mapTape, tape(oute3, d02, oute3));
    ASSERT_EQ(skyTape.size(), 3);
    EXPECT_NE(skyTape[0], "sky01"); // Rolled on the return from outd3.
    EXPECT_EQ(skyTape[1], "");
    EXPECT_EQ(skyTape[2], skyTape[0]);
    EXPECT_EQ(timeTape.back().toDays(), timeTape.front().toDays());
    ASSERT_GE(visitTape.size(), 2);
    EXPECT_CONTAINS(timeTape, visitTape.back()); // Left at the d02 door.
    EXPECT_GT(visitTape.back(), visitTape.front());
}

GAME_TEST(MM6MapVisit, FountainRefillsADayAfterTheLastVisit) {
    // A drained MM6 fountain was full again whenever the party came back, its daily refill went off on every return as
    // if the map had never been visited. It refills on a return a day or more after the party left, not the same day.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const std::string healed = engine->_levelStrings[14]; // Hit points restored.
    const std::string empty = engine->_levelStrings[8]; // Refreshing, and nothing else.
    auto statusTape = tapes.statusBar();
    auto timeTape = tapes.time();
    test.startTaping();

    game.tick(10); // The event processor checks the map's timers 30 game seconds after the map loads, the fountain fills.
    faceFountain(game);
    drinkFromFountain(game);
    ASSERT_EQ(statusTape.back(), healed);
    for (int drink = 0; drink < 100 && statusTape.back() != empty; drink++)
        drinkFromFountain(game);
    ASSERT_EQ(statusTape.back(), empty);

    for (bool laterDay : {false, true}) {
        if (laterDay) {
            toOutd3AndBack(game);
        } else {
            toD02AndBack(game);
        }
        game.tick(10); // The event processor checks the map's timers 30 game seconds after the map loads.
        faceFountain(game);
        drinkFromFountain(game);
        EXPECT_EQ(statusTape.back(), laterDay ? healed : empty) << laterDay;
    }
    EXPECT_GE(timeTape.back() - timeTape.front(), Duration::fromDays(10));
}

GAME_TEST(MM6MapVisitHeadless, DISABLED_SaveInDungeonAfterSkyRoll) {
    // A save in a dungeon keeps the visit of the outdoor map the party came from, its time and its sky.
    if (!enabled("strict-visit-write"))
        GTEST_SKIP() << "Run only as the first visit child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    toOutd3AndBack(game);
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto skyTape = tapes.custom([] { return outdoorSky(); });
    test.startTaping();

    game.teleportTo(oute3, Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_V);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.

    EXPECT_EQ(mapTape, tape(oute3, d02));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    ASSERT_TRUE(std::filesystem::is_regular_file(profile / "saves/save000.ec6"));
    ASSERT_EQ(skyTape.size(), 2);
    EXPECT_NE(skyTape[0], "sky01"); // Rolled on the return from outd3.
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-visit.txt") << skyTape[0] << "\n";
}

GAME_TEST(MM6MapVisitHeadless, DISABLED_LoadedSaveKeepsTheVisit) {
    // A fresh MM6 process loads the dungeon save, and the outdoor map the party walks back out to is a same-day return
    // under the sky it left, last visited when the door autosaved. The dungeon was last visited at the save.
    if (!enabled("strict-visit-reload"))
        GTEST_SKIP() << "Run only after the visit write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    std::istringstream expected(fileText(profile / "expected-visit.txt"));
    std::string savedSky;
    ASSERT_TRUE(std::getline(expected, savedSky));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    test.startTaping();
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto timeTape = tapes.time();
    auto skyTape = tapes.custom([] { return outdoorSky(); });
    auto visitTape = tapes.custom([] { return engine->_currentLoadedMapId == MAP_INVALID ? Time() : loadedMapLastVisit(); });

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    ASSERT_EQ(saveLoadMenu()->slots().size(), 2); // The autosave from the d02 door, then the dungeon slot.
    ASSERT_EQ(saveLoadMenu()->slots()[1].fileName, "save000.ec6");
    const Time doorTime = saveLoadMenu()->slots()[0].header.playingTime;
    const Time saveTime = saveLoadMenu()->slots()[1].header.playingTime;
    game.pressGuiButton("LoadMenu_Slot1");
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(mapTape.back(), d02);
    game.teleportTo(d02, Vec3f(16384, -19626, 864.5f), 270); // In front of the way out, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(MAP_INVALID, d02, oute3));
    EXPECT_EQ(skyTape.back(), savedSky);
    ASSERT_GE(visitTape.size(), 3);
    EXPECT_EQ(visitTape[1], saveTime);
    EXPECT_EQ(visitTape.back(), doorTime);
    EXPECT_EQ(timeTape.back().toDays(), saveTime.toDays());
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}
