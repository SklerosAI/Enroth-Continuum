#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/MM6SpriteEvent.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Graphics/OutdoorSkyResources.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/IconFrameTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/HudFrameResources.h"
#include "GUI/HudNavigationResources.h"
#include "GUI/HudStatusResources.h"
#include "GUI/HudVitalBarResources.h"
#include "GUI/LoadingScreenResources.h"
#include "GUI/MM6HudActionControls.h"
#include "GUI/MM6HudBookControls.h"
#include "GUI/UI/Books/CalendarBackgroundResources.h"
#include "GUI/UI/MainMenuResources.h"
#include "GUI/UI/UIGame.h"

#include "Library/FileSystem/Memory/MemoryFileSystem.h"
#include "Library/Platform/Null/NullEventLoop.h"
#include "Library/Platform/Null/NullPlatform.h"
#include "Library/Platform/Null/NullWindow.h"
#include "Library/Platform/Proxy/ProxyBase.h"

#include "Utility/Exception.h"

#include "GameTestOptions.h"
#include "MM6HeadlessRules.h"

namespace {
template<class T>
T *platformLeaf(T *value) {
    // PlatformApplication exposes a root proxy. Components installed by
    // GameStarter add more proxies before the final platform object.
    for (int depth = 0; depth < 64 && value; ++depth) {
        auto *proxy = dynamic_cast<ProxyBase<T> *>(value);
        if (!proxy)
            return value;
        value = proxy->base();
    }
    return nullptr; // Reject a missing leaf or malformed/cyclic chain.
}
} // namespace

GAME_TEST(MM6Headless, StartupAndChestGuard) {
    EXPECT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_NE(dynamic_cast<NullPlatform *>(platformLeaf(platform)), nullptr);
    EXPECT_NE(dynamic_cast<NullEventLoop *>(platformLeaf(eventLoop)), nullptr);
    EXPECT_NE(dynamic_cast<NullWindow *>(platformLeaf(window)), nullptr);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    EXPECT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    EXPECT_NE(dynamic_cast<MemoryFileSystem *>(ufs), nullptr);
    EXPECT_FALSE(ufs->exists("saves"));
    for (std::string_view name : mm6HeadlessRuleNames)
        EXPECT_TRUE(ufs->exists("rules/" + std::string(name)));

    // This is the first live-engine boundary, not a claim that the opening
    // map can load. Keep the existing MM6 guard closed until chest items are
    // staged and committed safely.
    EXPECT_THROW(mm6::generateChestItemsForTitle(GameTitle::MM6), std::runtime_error);
    game.tick(1);
}

GAME_TEST(MM6Headless, NewGameSaveGuard) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<MemoryFileSystem *>(ufs), nullptr);
    ASSERT_FALSE(ufs->exists("saves"));

    // Game::loop calls saveNewGame after party creation and before its first
    // gameLoop/map transition. The EC6 transaction is still guarded, so the
    // headless runner must stop here rather than attempt a partial map load.
    EXPECT_THROW(saveNewGame(), std::runtime_error);
    EXPECT_FALSE(ufs->exists("saves"));
}

GAME_TEST(MM6Headless, FirstOutdoorResourcePreflight) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    EXPECT_TRUE(isMapOutdoor(firstMap));
    EXPECT_FALSE(isMapIndoor(firstMap));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, firstMap));
    EXPECT_EQ(pMapTable->pInfos[firstMap].fileName, "oute3.odm");
    ASSERT_TRUE(pGames_LOD->exists("oute3.odm"));
    EXPECT_TRUE(pGames_LOD->exists("oute3.ddm"));
    EXPECT_GT(engine->resources()->eventsData("oute3.evt").size(), 0);
    EXPECT_GT(engine->resources()->eventsData("oute3.str").size(), 0);
    EvtProgram events = EvtProgram::load(engine->resources()->eventsData("oute3.evt"), GameTitle::MM6);
    EXPECT_EQ(events.enumerateTriggers(EVENT_OnMapReload).size(), 11);
    EXPECT_EQ(engine->resources()->iconSize("oute3"), Sizei(512, 512));

    const LoadingScreenResources loading = fullscreenLoadingResources(GameTitle::MM6, 0);
    EXPECT_EQ(engine->resources()->iconSize(loading.background), Sizei(640, 480));
    EXPECT_TRUE(loading.progress.empty());
    EXPECT_EQ(engine->resources()->iconSize("bardata"), Sizei(300, 100));
    EXPECT_EQ(pIconsFrameTable->animationId("turnhour"), -1);

    OutdoorLocation_MM7 base;
    deserializeMM6Outdoor(decodeMM6ArchiveEntry(pGames_LOD->read("oute3.odm")), &base);
    size_t reloadSpriteActions = 0;
    for (const EventTrigger trigger : events.enumerateTriggers(EVENT_OnMapReload)) {
        for (const EvtInstruction &instruction : events.function(trigger.eventId)) {
            if (instruction.opcode != EVENT_SetSprite)
                continue;
            EXPECT_GE(instruction.data.sprite_texture_descr.cog, 0);
            EXPECT_LT(static_cast<size_t>(instruction.data.sprite_texture_descr.cog), base.decorations.size());
            ++reloadSpriteActions;
        }
    }
    EXPECT_GT(reloadSpriteActions, 0);
    const auto terminator = std::find(base.skyTexture.begin(), base.skyTexture.end(), '\0');
    const std::string_view sky(base.skyTexture.data(), terminator - base.skyTexture.begin());
    EXPECT_EQ(mm6OutdoorBaseSky(GameTitle::MM6, sky), "plansky2");
    EXPECT_EQ(engine->resources()->bitmapSize(sky), Sizei(256, 256));
}

TEST(MM6HeadlessFirstOutdoorSpriteEvents, IndexAndTitleIsolation) {
    std::vector<LevelDecoration> decorations(3);
    decorations[0].uCog = 1;
    decorations[1].uCog = 0;
    decorations[2].uCog = 1;
    decorations[1].uFlags |= LEVEL_DECORATION_INVISIBLE;

    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM7, decorations, 1, true, std::nullopt));
    EXPECT_TRUE(decorations[1].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 1, true, std::nullopt));
    EXPECT_FALSE(decorations[1].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 2, false, std::nullopt));
    EXPECT_TRUE(decorations[2].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_FALSE(decorations[0].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 1, true,
                                    DecorationId::DECORATION_FOUNTAIN_3));
    EXPECT_EQ(decorations[1].uDecorationDescID, DecorationId::DECORATION_FOUNTAIN_3);
    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM6, decorations, -1, false, std::nullopt));
    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM6, decorations, 3, false, std::nullopt));
}

TEST(MM6HeadlessFirstOutdoorResources, TitleIsolation) {
    const LoadingScreenResources mm6 = fullscreenLoadingResources(GameTitle::MM6, 1);
    EXPECT_EQ(mm6.background, "loading.pcx");
    EXPECT_TRUE(mm6.progress.empty());
    const LoadingScreenResources mm7First = fullscreenLoadingResources(GameTitle::MM7, 1);
    const LoadingScreenResources mm7Last = fullscreenLoadingResources(GameTitle::MM7, 5);
    EXPECT_EQ(mm7First.background, "loading1.pcx");
    EXPECT_EQ(mm7Last.background, "loading5.pcx");
    EXPECT_EQ(mm7First.progress, "loadprog");
    EXPECT_EQ(mm7Last.progress, "loadprog");
    EXPECT_EQ(mm6OutdoorBaseSky(GameTitle::MM6, "plansky2"), "plansky2");
    EXPECT_TRUE(mm6OutdoorBaseSky(GameTitle::MM7, "plansky2").empty());
}

TEST(MM6HeadlessFirstOutdoorMapId, HirelingRuleIsolation) {
    // MM6's first outdoor map has numeric ID 15, reused by MM7's Shoals.
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, MAP_SHOALS));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, MAP_LINCOLN));
    EXPECT_TRUE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_SHOALS));
    EXPECT_TRUE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_LINCOLN));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_HARMONDALE));
}

GAME_TEST(MM6Headless, MainMenuResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    Sizei background = engine->resources()->iconSize(resources.background);
    EXPECT_GT(background.w, 0);
    EXPECT_GT(background.h, 0);
    for (const MainMenuButtonResource &button : resources.buttons) {
        Sizei size = engine->resources()->iconSize(button.icon);
        EXPECT_GT(size.w, 0) << button.icon;
        EXPECT_GT(size.h, 0) << button.icon;
    }
}

GAME_TEST(MM6Headless, HudNavigationResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudNavigationResources resources = hudNavigationResources(engine->gameTitle(),
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    for (std::string_view name : {resources.compass, resources.zoomIn, resources.zoomOut}) {
        Sizei size = engine->resources()->iconSize(name);
        EXPECT_GT(size.w, 0) << name;
        EXPECT_GT(size.h, 0) << name;
    }
}

GAME_TEST(MM6Headless, HudFrameResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudFrameAssets frames = hudFrameAssets(GameTitle::MM6, {}, {}, {}, {});
    const std::array<std::pair<std::string_view, Sizei>, 4> expected = {{
        {frames.right, {172, 339}}, {frames.bottom, {469, 109}},
        {frames.top, {468, 8}}, {frames.left, {8, 344}}
    }};
    for (const auto &[name, size] : expected)
        EXPECT_EQ(engine->resources()->iconSize(name), size) << name;
}

GAME_TEST(MM6Headless, StatusStripAndMapArrowsAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_EQ(engine->resources()->iconSize(hudStatusBarIcon(GameTitle::MM6, {})), Sizei(483, 24));
    // These eight inherited minimap directions already exist in the MM6
    // archive; the dynamic top-frame state remains a separate open mapping.
    for (int i = 1; i <= 8; ++i) {
        const std::string icon = "MAPDIR" + std::to_string(i);
        EXPECT_EQ(engine->resources()->iconSize(icon), Sizei(7, 7)) << icon;
    }
}

GAME_TEST(MM6Headless, PortraitVitalBarsAndHitBoxes) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudVitalBarResources bars = hudVitalBarResources(GameTitle::MM6, {}, {}, {}, {});
    EXPECT_EQ(engine->resources()->iconSize(bars.green), Sizei(6, 78));
    EXPECT_EQ(engine->resources()->iconSize(bars.blue), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_green->size(), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_blue->size(), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_yellow, nullptr);
    EXPECT_EQ(game_ui_bar_red, nullptr);

    // The headless startup fixture has not entered the game-screen UI.
    // Construct this button group explicitly, then let the window clean it up.
    GUIWindow buttons(WINDOW_NULL, {0, 0}, {640, 480});
    buttons.CreateCharacterButtons();
    std::vector<Recti> actual;
    for (GUIButton *button : buttons.vButtons)
        if (button->uData == UIMSG_ShowStatus_ManaHP)
            actual.push_back(button->rect);
    const auto health = mm6HudHealthBarRects();
    const auto mana = mm6HudManaBarRects();
    ASSERT_EQ(actual.size(), 8);
    for (size_t i = 0; i < health.size(); ++i) {
        EXPECT_EQ(actual[2 * i], health[i]);
        EXPECT_EQ(actual[2 * i + 1], mana[i]);
        EXPECT_EQ(health[i].y + health[i].h, 463);
        EXPECT_EQ(mana[i].y + mana[i].h, 463);
    }
}

TEST(MM6HeadlessVitalBars, TitleIsolation) {
    const HudVitalBarResources mm6 = hudVitalBarResources(GameTitle::MM6,
        "ib-statB", "ib-statG", "ib-statY", "ib-statR");
    EXPECT_EQ(mm6.blue, "manafull");
    EXPECT_EQ(mm6.green, "hitsfull");
    EXPECT_TRUE(mm6.yellow.empty());
    EXPECT_TRUE(mm6.red.empty());
    const HudVitalBarResources mm7 = hudVitalBarResources(GameTitle::MM7,
        "ib-statB", "ib-statG", "ib-statY", "ib-statR");
    EXPECT_EQ(mm7.blue, "ib-statB");
    EXPECT_EQ(mm7.green, "ib-statG");
    EXPECT_EQ(mm7.yellow, "ib-statY");
    EXPECT_EQ(mm7.red, "ib-statR");
    EXPECT_EQ(mm6HudHealthBarRects()[0], Recti(94, 385, 6, 78));
    EXPECT_EQ(mm6HudManaBarRects()[3], Recti(447, 385, 6, 78));
}

TEST(MM6HeadlessStatusStrip, TitleIsolation) {
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM6, "IB-Foot-a.pcx"), "footer");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-a.pcx"), "IB-Foot-a.pcx");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-b.pcx"), "IB-Foot-b.pcx");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-c.pcx"), "IB-Foot-c.pcx");
}

GAME_TEST(MM6Headless, FourBookHitBoxesAndActions) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(pPrimaryWindow, nullptr);
    const auto expected = mm6HudBookControls();
    const std::array<GUIButton *, 4> actual = {pBtn_Quests, pBtn_Autonotes, pBtn_Maps, pBtn_Calendar};
    for (size_t i = 0; i < actual.size(); ++i) {
        ASSERT_NE(actual[i], nullptr);
        EXPECT_EQ(actual[i]->rect, expected[i].rect);
        EXPECT_EQ(actual[i]->msg, expected[i].message);
        EXPECT_EQ(actual[i]->action, expected[i].action);
        EXPECT_TRUE(actual[i]->vTextures.empty()); // Book art is baked into the MM6 frame.
        EXPECT_TRUE(actual[i]->Contains(expected[i].rect.center()));
        for (size_t j = 0; j < actual.size(); ++j)
            if (i != j)
                EXPECT_FALSE(actual[j]->Contains(expected[i].rect.center()));
    }
    EXPECT_EQ(pBtn_History, nullptr);
}

GAME_TEST(MM6Headless, BakedActionHitBoxesAndCalendarBackground) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(pPrimaryWindow, nullptr);
    const auto expected = mm6HudActionControls();
    const std::array<GUIButton *, 4> actual = {
        pBtn_CastSpell, pBtn_Rest, pBtn_QuickReference, pBtn_GameSettings
    };
    for (size_t i = 0; i < actual.size(); ++i) {
        ASSERT_NE(actual[i], nullptr);
        EXPECT_EQ(actual[i]->rect, expected[i].rect);
        EXPECT_EQ(actual[i]->msg, expected[i].message);
        EXPECT_EQ(actual[i]->action, expected[i].action);
        EXPECT_TRUE(actual[i]->vTextures.empty());
        EXPECT_TRUE(actual[i]->Contains(expected[i].rect.center()));
        for (size_t j = 0; j < actual.size(); ++j)
            if (i != j)
                EXPECT_FALSE(actual[j]->Contains(expected[i].rect.center()));
    }
    EXPECT_EQ(game_ui_btn_cast, nullptr);
    EXPECT_EQ(game_ui_btn_rest, nullptr);
    EXPECT_EQ(game_ui_btn_quickref, nullptr);
    EXPECT_EQ(game_ui_btn_settings, nullptr);
    EXPECT_EQ(engine->resources()->iconSize(calendarBackgroundIcon(GameTitle::MM6)), Sizei(360, 300));
}

TEST(MM6HeadlessActionControls, TitleIsolation) {
    EXPECT_TRUE(useMm6HudActionControls(GameTitle::MM6));
    EXPECT_FALSE(useMm6HudActionControls(GameTitle::MM7));
    const auto controls = mm6HudActionControls();
    EXPECT_EQ(controls[0].message, UIMSG_SpellBookWindow);
    EXPECT_EQ(controls[0].action, INPUT_ACTION_OPEN_SPELLBOOK);
    EXPECT_EQ(controls[1].message, UIMSG_RestWindow);
    EXPECT_EQ(controls[1].action, INPUT_ACTION_REST);
    EXPECT_EQ(controls[2].message, UIMSG_QuickReference);
    EXPECT_EQ(controls[2].action, INPUT_ACTION_OPEN_QUICK_REFERENCE);
    EXPECT_EQ(controls[3].message, UIMSG_GameMenuButton);
    EXPECT_EQ(controls[3].action, INPUT_ACTION_INVALID);
    EXPECT_EQ(calendarBackgroundIcon(GameTitle::MM6), "time_bg");
    EXPECT_EQ(calendarBackgroundIcon(GameTitle::MM7), "sbdate-time");
}

TEST(MM6HeadlessBookControls, TitleIsolation) {
    EXPECT_TRUE(useMm6HudBookControls(GameTitle::MM6));
    EXPECT_FALSE(useMm6HudBookControls(GameTitle::MM7));
    const auto books = mm6HudBookControls();
    ASSERT_EQ(books.size(), 4);
    EXPECT_EQ(books[0].message, UIMSG_OpenQuestBook);
    EXPECT_EQ(books[1].message, UIMSG_OpenAutonotes);
    EXPECT_EQ(books[2].message, UIMSG_OpenMapBook);
    EXPECT_EQ(books[3].message, UIMSG_OpenCalendar);
    EXPECT_EQ(books[0].action, INPUT_ACTION_OPEN_QUESTS);
    EXPECT_EQ(books[1].action, INPUT_ACTION_OPEN_AUTONOTES);
    EXPECT_EQ(books[2].action, INPUT_ACTION_OPEN_MAP);
    EXPECT_EQ(books[3].action, INPUT_ACTION_OPEN_CALENDAR);
}

TEST(MM6HeadlessHudFrames, TitleIsolation) {
    const HudFrameAssets mm6 = hudFrameAssets(GameTitle::MM6, {}, {}, {}, {});
    EXPECT_EQ(mm6.right, "border1.pcx");
    EXPECT_EQ(mm6.bottom, "border2.pcx");
    EXPECT_EQ(mm6.top, "border3");
    EXPECT_EQ(mm6.left, "border4");
    const HudFramePositions mm6Positions = hudFramePositions(GameTitle::MM6);
    EXPECT_EQ(mm6Positions.right, Pointi(468, 141));
    EXPECT_EQ(mm6Positions.bottom, Pointi(0, 371));
    EXPECT_EQ(mm6Positions.top, Pointi(0, 0));
    EXPECT_EQ(mm6Positions.left, Pointi(0, 8));

    const HudFrameAssets mm7Neutral = hudFrameAssets(GameTitle::MM7,
        "ib-r-A.pcx", "ib-b-A.pcx", "ib-t-A.pcx", "ib-l-A.pcx");
    EXPECT_EQ(mm7Neutral.right, "ib-r-A.pcx");
    EXPECT_EQ(mm7Neutral.bottom, "ib-b-A.pcx");
    EXPECT_EQ(mm7Neutral.top, "ib-t-A.pcx");
    EXPECT_EQ(mm7Neutral.left, "ib-l-A.pcx");
    const HudFrameAssets mm7Good = hudFrameAssets(GameTitle::MM7,
        "ib-r-B.pcx", "ib-b-B.pcx", "ib-t-B.pcx", "ib-l-B.pcx");
    EXPECT_EQ(mm7Good.right, "ib-r-B.pcx");
    EXPECT_EQ(mm7Good.bottom, "ib-b-B.pcx");
    EXPECT_EQ(mm7Good.top, "ib-t-B.pcx");
    EXPECT_EQ(mm7Good.left, "ib-l-B.pcx");
    const HudFrameAssets mm7Evil = hudFrameAssets(GameTitle::MM7,
        "ib-r-C.pcx", "ib-b-C.pcx", "ib-t-C.pcx", "ib-l-C.pcx");
    EXPECT_EQ(mm7Evil.right, "ib-r-C.pcx");
    EXPECT_EQ(mm7Evil.bottom, "ib-b-C.pcx");
    EXPECT_EQ(mm7Evil.top, "ib-t-C.pcx");
    EXPECT_EQ(mm7Evil.left, "ib-l-C.pcx");
    const HudFramePositions mm7Positions = hudFramePositions(GameTitle::MM7);
    EXPECT_EQ(mm7Positions.right, Pointi(468, 0));
    EXPECT_EQ(mm7Positions.bottom, Pointi(0, 352));
    EXPECT_EQ(mm7Positions.top, Pointi(0, 0));
    EXPECT_EQ(mm7Positions.left, Pointi(0, 8));
}

TEST(MM6HeadlessHudResources, TitleIsolation) {
    const HudNavigationResources mm6 = hudNavigationResources(GameTitle::MM6,
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    EXPECT_EQ(mm6.compass, "compass");
    EXPECT_EQ(mm6.zoomIn, "zoom+on");
    EXPECT_EQ(mm6.zoomOut, "zoom-on");

    const HudNavigationResources mm7Neutral = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    EXPECT_EQ(mm7Neutral.compass, "IB-COMP-A");
    EXPECT_EQ(mm7Neutral.zoomIn, "ib-autout-a");
    EXPECT_EQ(mm7Neutral.zoomOut, "ib-autin-a");
    const HudNavigationResources mm7Good = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-B", "ib-autout-B", "ib-autin-B");
    EXPECT_EQ(mm7Good.compass, "IB-COMP-B");
    EXPECT_EQ(mm7Good.zoomIn, "ib-autout-B");
    EXPECT_EQ(mm7Good.zoomOut, "ib-autin-B");
    const HudNavigationResources mm7Evil = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-C", "ib-autout-C", "ib-autin-C");
    EXPECT_EQ(mm7Evil.compass, "IB-COMP-C");
    EXPECT_EQ(mm7Evil.zoomIn, "ib-autout-C");
    EXPECT_EQ(mm7Evil.zoomOut, "ib-autin-C");
}

TEST(MM6HeadlessMenuAssets, TitleSpecificLookup) {
    const MainMenuResources mm6 = mainMenuResources(GameTitle::MM6);
    const MainMenuResources mm7 = mainMenuResources(GameTitle::MM7);
    EXPECT_EQ(mm6.background, "title.pcx");
    EXPECT_EQ(mm7.background, "title.pcx");
    EXPECT_EQ(mm6.buttons[0].icon, "start00a");
    EXPECT_EQ(mm6.buttons[1].icon, "start00b");
    EXPECT_EQ(mm6.buttons[2].icon, "start00c");
    EXPECT_EQ(mm6.buttons[3].icon, "start00d");
    EXPECT_EQ(mm6.buttons[0].position, Pointi(482, 9));
    EXPECT_EQ(mm6.buttons[1].position, Pointi(482, 71));
    EXPECT_EQ(mm6.buttons[2].position, Pointi(482, 133));
    EXPECT_EQ(mm6.buttons[3].position, Pointi(482, 195));
    EXPECT_EQ(mm7.buttons[0].icon, "title_new");
    EXPECT_EQ(mm7.buttons[1].icon, "title_load");
    EXPECT_EQ(mm7.buttons[2].icon, "title_cred");
    EXPECT_EQ(mm7.buttons[3].icon, "title_exit");
    EXPECT_EQ(mm7.buttons[0].position, Pointi(495, 172));
    EXPECT_EQ(mm7.buttons[1].position, Pointi(495, 227));
    EXPECT_EQ(mm7.buttons[2].position, Pointi(495, 282));
    EXPECT_EQ(mm7.buttons[3].position, Pointi(495, 337));
}

TEST(MM6HeadlessRules, RejectsMissingRulesOption) {
    char app[] = "game-test";
    char gameFlag[] = "--game";
    char mm6[] = "mm6";
    char headless[] = "--headless";
    char dataFlag[] = "--data-path";
    char testFlag[] = "--test-path";
    char dot[] = ".";
    char *argv[] = {app, gameFlag, mm6, headless, dataFlag, dot, testFlag, dot};
    EXPECT_THROW(GameTestOptions::parse(8, argv), Exception);
}

TEST(MM6HeadlessRules, SeedsOnlyCompleteBoundedRuleSet) {
    MemoryFileSystem source("source");
    MemoryFileSystem destination("destination");
    for (std::string_view name : mm6HeadlessRuleNames)
        source.write(name, Blob::fromString("{}"));

    source.remove("spells.json");
    EXPECT_THROW(seedMm6HeadlessRules(source, &destination), Exception);
    EXPECT_FALSE(destination.exists("rules"));

    source.write("spells.json", Blob::fromString("{}"));
    source.write("creation.json", Blob::fromString(std::string(1024 * 1024 + 1, 'x')));
    EXPECT_THROW(seedMm6HeadlessRules(source, &destination), Exception);
    EXPECT_FALSE(destination.exists("rules"));

    source.write("creation.json", Blob::fromString("{}"));
    seedMm6HeadlessRules(source, &destination);
    for (std::string_view name : mm6HeadlessRuleNames) {
        Blob staged = destination.read("rules/" + std::string(name));
        EXPECT_EQ(staged.str(), "{}");
    }
    Blob original = source.read("creation.json");
    EXPECT_EQ(original.str(), "{}");
}
