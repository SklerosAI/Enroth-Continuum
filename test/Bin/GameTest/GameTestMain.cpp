#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Application/Startup/GameStarter.h"

#include "Engine/Components/Control/EngineController.h"
#include "Engine/Resources/EngineFileSystem.h"

#include "Testing/Game/GameTest.h"
#include "Testing/Game/TestController.h"

#include "Library/StackTrace/StackTraceOnCrash.h"
#include "Library/Platform/Application/PlatformApplication.h"
#include "Library/FileSystem/Native/NativeFileSystem.h"

#include "Utility/String/Format.h"
#include "Utility/String/Split.h"
#include "Utility/UnicodeCrt.h"
#include "Utility/Exception.h"

#include "GameTestOptions.h"
#include "MM6HeadlessRules.h"

namespace {
bool isMm6OnlyFilter(std::string_view filter) {
    std::string_view positive = filter.substr(0, filter.find('-'));
    if (positive.empty())
        return false;

    for (std::string_view pattern : split(positive).by(':'))
        if (!pattern.starts_with("MM6"))
            return false;
    return true;
}

void validateDisposableMm7Profile(GameTestOptions *opts, bool writer) {
    namespace fs = std::filesystem;
    const fs::path supplied(opts->userPath.str());
    if (!supplied.is_absolute() || !fs::is_directory(supplied))
        throw Exception("MM7 save gate requires an absolute disposable profile directory.");
    const fs::path root = fs::canonical(supplied);
    if (root.parent_path() != fs::canonical(fs::temp_directory_path()) ||
        !root.filename().string().starts_with("mm7-save-gate-") ||
        (fs::status(root).permissions() & (fs::perms::group_all | fs::perms::others_all)) != fs::perms::none)
        throw Exception("MM7 save gate requires an owner-only system-temp profile.");
    const fs::path markerPath = root / ".ec-mm7-disposable-save-gate";
    if (!fs::is_regular_file(markerPath) || fs::is_symlink(markerPath))
        throw Exception("MM7 save gate profile marker is missing or invalid.");
    std::ifstream marker(markerPath);
    std::string value;
    if (!std::getline(marker, value) || value != "EnrothContinuum-MM7-save-gate-v1")
        throw Exception("MM7 save gate profile marker is missing or invalid.");
    for (const fs::directory_entry &entry : fs::directory_iterator(root)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_symlink() ||
            (writer && name != ".ec-mm7-disposable-save-gate") ||
            (!writer && name != ".ec-mm7-disposable-save-gate" &&
             name != "enroth-continuum-mm7.ini" && name != "saves"))
            throw Exception("MM7 save gate profile contains an unexpected entry.");
    }
    if (!writer) {
        const fs::path saves = root / "saves";
        if (!fs::is_directory(saves) || fs::is_symlink(saves) ||
            !fs::is_regular_file(saves / "save000.mm7") || fs::is_symlink(saves / "save000.mm7"))
            throw Exception("MM7 save gate profile has no regular candidate save.");
        for (const fs::directory_entry &entry : fs::directory_iterator(saves)) {
            const std::string name = entry.path().filename().string();
            if (!entry.is_regular_file() || entry.is_symlink() ||
                (name != "save000.mm7" && name != "save000.mm7.bak" &&
                 name != "autosave.mm7" && name != "autosave.mm7.bak"))
                throw Exception("MM7 save gate profile contains an unexpected save.");
        }
    }
    opts->userPath = Path(root.string());
}
} // anonymous namespace

void printGoogleTestHelp(char *app) {
    int argc = 2;
    char help[] = "--help";
    char *argv[] = { app, help, nullptr };
    testing::InitGoogleTest(&argc, argv);
}

int platformMain(int argc, char **argv) {
    try {
        StackTraceOnCrash st(nullptr, STACK_TRACE_LOAD_SYMBOLS_ON_CRASH);
        UnicodeCrt _(argc, argv);
        GameTestOptions opts = GameTestOptions::parse(argc, argv);
        if (opts.helpPrinted) {
            fmt::print(stdout, "\n");
            printGoogleTestHelp(argv[0]);
            return 1;
        }

        testing::InitGoogleTest(&argc, argv);
        if (GTEST_FLAG_GET(filter) == "*") {
            // Keep the existing MM7 default while making an unfiltered MM6
            // run execute only tests that are safe for the MM6 engine.
            GTEST_FLAG_SET(filter, opts.gameTitle == GameTitle::MM6 ? "MM6*.*" : "*-MM6*.*");
        } else if (opts.gameTitle == GameTitle::MM6 && !isMm6OnlyFilter(GTEST_FLAG_GET(filter))) {
            throw Exception("MM6 game tests require an MM6-only --gtest_filter.");
        }
        if (opts.offscreenCapture &&
            (GTEST_FLAG_GET(filter) != (opts.gameTitle == GameTitle::MM6 ?
                 "MM6OffscreenFrame.DISABLED_FirstOutdoorCapture" :
                 "MM7OffscreenFrame.DISABLED_FirstOutdoorCapture") ||
             !GTEST_FLAG_GET(also_run_disabled_tests))) {
            throw Exception("--offscreen-gl requires the title's isolated disabled first-frame test filter.");
        }
        const bool mm7SaveWriter = GTEST_FLAG_GET(filter) == "MM7DisposableSave.DISABLED_WriteFreshProfile";
        const bool mm7SaveReader = GTEST_FLAG_GET(filter) == "MM7DisposableSave.DISABLED_ReloadFreshProcess";
        const bool mm7CastleWriter = GTEST_FLAG_GET(filter) == "MM7CastleRebuild.DISABLED_WriteFreshCheckpoint";
        const bool mm7CastleReader = GTEST_FLAG_GET(filter) == "MM7CastleRebuild.DISABLED_ReloadFreshCheckpoint";
        const bool mm7LightWriter = GTEST_FLAG_GET(filter) == "MM7LightWalls.DISABLED_WriteFreshCheckpoint";
        const bool mm7LightReader = GTEST_FLAG_GET(filter) == "MM7LightWalls.DISABLED_ReloadFreshCheckpoint";
        const bool mm7OverthrusterWriter = GTEST_FLAG_GET(filter) == "MM7OverthrusterRoute.DISABLED_WriteFreshCheckpoint";
        const bool mm7OverthrusterReader = GTEST_FLAG_GET(filter) == "MM7OverthrusterRoute.DISABLED_ReloadFreshCheckpoint";
        const bool mm7LightSeamWriter = GTEST_FLAG_GET(filter) == "MM7LightSeam.DISABLED_WriteFreshCheckpoint";
        const bool mm7LightSeamReader = GTEST_FLAG_GET(filter) == "MM7LightSeam.DISABLED_ReloadFreshCheckpoint";
        const bool mm7Writer = mm7SaveWriter || mm7CastleWriter || mm7LightWriter || mm7OverthrusterWriter || mm7LightSeamWriter;
        const bool mm7Reader = mm7SaveReader || mm7CastleReader || mm7LightReader || mm7OverthrusterReader || mm7LightSeamReader;
        if (opts.disposableMm7Profile || mm7Writer || mm7Reader) {
            if (!opts.disposableMm7Profile || !(mm7Writer || mm7Reader) ||
                !GTEST_FLAG_GET(also_run_disabled_tests))
                throw Exception("MM7 disposable save gate requires its exact disabled test filter and temporary profile.");
            validateDisposableMm7Profile(&opts, mm7Writer);
        }
        if (opts.listRequested)
            return RUN_ALL_TESTS();

        std::function<void(FileSystem *)> prepareUserFs;
        if (opts.gameTitle == GameTitle::MM6) {
            prepareUserFs = [&opts] (FileSystem *userFs) {
                NativeFileSystem source(opts.rulesPath);
                seedMm6HeadlessRules(source, userFs);
            };
        }
        GameStarter starter(opts, prepareUserFs);

        int exitCode = 0;
        starter.runInstrumented([&] (EngineController *game) {
            NativeFileSystem tfs(opts.testPath);
            TestController test(game, &tfs, opts.speed, opts.disposableMm7Profile, mm7Reader);
            GameTest::init(game, &test);
            exitCode = RUN_ALL_TESTS();
        });
        return exitCode;
    } catch (const std::exception &e) {
        // TODO(captainurist): we need a separate test that testing framework terminates correctly if the engine throws.
        fmt::print(stderr, "{}\n", e.what());
        return 1;
    }
}
