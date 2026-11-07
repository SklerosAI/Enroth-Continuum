#include <gtest/gtest.h>

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
            TestController test(game, &tfs, opts.speed);
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
