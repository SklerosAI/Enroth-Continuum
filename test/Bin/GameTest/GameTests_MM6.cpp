#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/SaveLoad.h"

#include "Library/FileSystem/Memory/MemoryFileSystem.h"
#include "Library/Platform/Null/NullEventLoop.h"
#include "Library/Platform/Null/NullPlatform.h"
#include "Library/Platform/Null/NullWindow.h"

#include "Utility/Exception.h"

#include "GameTestOptions.h"
#include "MM6HeadlessRules.h"

GAME_TEST(MM6Headless, StartupAndChestGuard) {
    EXPECT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_NE(dynamic_cast<NullPlatform *>(platform), nullptr);
    EXPECT_NE(dynamic_cast<NullEventLoop *>(eventLoop), nullptr);
    EXPECT_NE(dynamic_cast<NullWindow *>(window), nullptr);
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
