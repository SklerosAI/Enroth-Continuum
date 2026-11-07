#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/SaveLoad.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "Library/Platform/Application/PlatformApplication.h"

namespace {

std::filesystem::path disposableProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || supplied.lexically_normal() != supplied ||
        std::filesystem::is_symlink(std::filesystem::symlink_status(supplied)) ||
        !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 QuickLoad guard needs a native disposable profile");
    const auto root = std::filesystem::canonical(supplied);
    if (root.parent_path() !=
            std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 QuickLoad guard needs an owner-only temp profile");
    return root;
}

} // namespace

GAME_TEST(MM6MainMenuQuickLoadGuard, DISABLED_PresentDisposableEc6SlotStaysInMenu) {
    const char *enable = std::getenv("ENROTH_MM6_QUICKLOAD_GUARD_ENABLE");
    if (!enable || std::string_view(enable) != "strict-present-disposable-slot")
        GTEST_SKIP() << "Opt in to the disposable MM6 Main Menu QuickLoad gate";
    const auto profile = disposableProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to the disposable user path";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_EQ(GetCurrentMenuID(), MENU_MAIN);
    ASSERT_TRUE(engine->_pendingLoadFileName.empty());

    const std::string fileName = getCurrentQuickSave();
    ASSERT_TRUE(fileName.ends_with(".ec6"));
    const auto directory = profile / "saves";
    ASSERT_FALSE(std::filesystem::exists(directory));
    std::filesystem::create_directory(directory);
    const auto slot = directory / fileName;
    constexpr std::array<char, 4> marker = {'E', 'C', '6', '!'};
    {
        std::ofstream output(slot, std::ios::binary);
        ASSERT_TRUE(output.good());
        output.write(marker.data(), marker.size());
        ASSERT_TRUE(output.good());
    }
    ASSERT_TRUE(ufs->exists("saves/" + fileName));

    bool loadCalled = false;
    game.runGameRoutine([&] {
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride([&](std::string_view) {
            loadCalled = true;
        });
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });
    game.tick(2);
    game.runGameRoutine([] {
        mm6::testing::setLoadGameOverride({});
    });

    EXPECT_FALSE(loadCalled);
    EXPECT_TRUE(engine->_pendingLoadFileName.empty());
    EXPECT_EQ(GetCurrentMenuID(), MENU_MAIN);
    EXPECT_TRUE(ufs->exists("saves/" + fileName));
    EXPECT_EQ(std::filesystem::file_size(slot), marker.size());
    std::filesystem::remove(slot);
    std::filesystem::remove(directory);
}
