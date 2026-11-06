#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"

#include "GUI/UI/MainMenuResources.h"
#include "GUI/HudNavigationResources.h"

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
