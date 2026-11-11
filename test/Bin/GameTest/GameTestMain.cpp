#include <gtest/gtest.h>

#include <algorithm>
#include <array>
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

/**
 * @param title                         Title the tests run for.
 * @param filter                        The `--gtest_filter` value.
 * @return                              Whether `filter` names exactly one of `title`'s offscreen capture tests.
 */
bool isOffscreenCaptureFilter(GameTitle title, std::string_view filter) {
    if (title == GameTitle::MM6)
        return filter == "MM6OffscreenFrame.DISABLED_FirstOutdoorCapture" || filter == "MM6OffscreenFrame.DISABLED_MapCapture";
    return filter == "MM7OffscreenFrame.DISABLED_FirstOutdoorCapture" || filter == "MM7OffscreenFrame.DISABLED_MapCapture";
}

/**
 * Admission rule for one disabled MM7 test that runs against a marked disposable profile.
 *
 * @param filter                        Exact `--gtest_filter` that selects the test.
 * @param writer                        Whether the test starts from a profile that holds nothing but the gate marker.
 * @param preserveSave                  Whether the test controller keeps the profile's saves when it resets the
 *                                      profile before the test.
 * @param chapter                       Campaign chain chapter. A chapter after C0 requires the previous chapter's
 *                                      slot and refuses its own and later slots. -1 is a legacy writer or reader
 *                                      pair that only knows `save000.mm7`.
 */
struct Mm7ProfileGate {
    std::string_view filter;
    bool writer;
    bool preserveSave;
    int chapter;
};

constexpr std::array<Mm7ProfileGate, 18> mm7ProfileGates = {{
    {"MM7DisposableSave.DISABLED_WriteFreshProfile", true, false, -1},
    {"MM7DisposableSave.DISABLED_ReloadFreshProcess", false, true, -1},
    {"MM7CastleRebuild.DISABLED_WriteFreshCheckpoint", true, false, -1},
    {"MM7CastleRebuild.DISABLED_ReloadFreshCheckpoint", false, true, -1},
    {"MM7LightWalls.DISABLED_WriteFreshCheckpoint", true, false, -1},
    {"MM7LightWalls.DISABLED_ReloadFreshCheckpoint", false, true, -1},
    {"MM7OverthrusterRoute.DISABLED_WriteFreshCheckpoint", true, false, -1},
    {"MM7OverthrusterRoute.DISABLED_ReloadFreshCheckpoint", false, true, -1},
    {"MM7LightSeam.DISABLED_WriteFreshCheckpoint", true, false, -1},
    {"MM7LightSeam.DISABLED_ReloadFreshCheckpoint", false, true, -1},
    {"MM7CampaignChain.DISABLED_Chapter0", true, false, 0},
    {"MM7CampaignChain.DISABLED_Chapter1", false, true, 1},
    {"MM7CampaignChain.DISABLED_Chapter2", false, true, 2},
    {"MM7CampaignChain.DISABLED_Chapter3", false, true, 3},
    {"MM7CampaignChain.DISABLED_Chapter4", false, true, 4},
    {"MM7CampaignChain.DISABLED_Chapter5", false, true, 5},
    {"MM7CampaignChain.DISABLED_Chapter6", false, true, 6},
    {"MM7CampaignChain.DISABLED_Chapter7", false, true, 7},
}};

void validateDisposableMm7Profile(GameTestOptions *opts, const Mm7ProfileGate &gate) {
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
            (gate.writer && name != ".ec-mm7-disposable-save-gate") ||
            (!gate.writer && name != ".ec-mm7-disposable-save-gate" &&
             name != "enroth-continuum-mm7.ini" && name != "saves"))
            throw Exception("MM7 save gate profile contains an unexpected entry.");
    }
    if (!gate.writer) {
        const fs::path saves = root / "saves";
        const std::string candidate = gate.chapter > 0 ? fmt::format("save{:03}.mm7", gate.chapter - 1) : "save000.mm7";
        if (!fs::is_directory(saves) || fs::is_symlink(saves) ||
            !fs::is_regular_file(saves / candidate) || fs::is_symlink(saves / candidate))
            throw Exception("MM7 save gate profile has no regular candidate save.");
        for (const fs::directory_entry &entry : fs::directory_iterator(saves)) {
            const std::string name = entry.path().filename().string();
            bool allowed = name == "autosave.mm7" || name == "autosave.mm7.bak";
            if (gate.chapter >= 0) {
                for (int chapter = 0; chapter < gate.chapter; ++chapter) {
                    const std::string slot = fmt::format("save{:03}.mm7", chapter);
                    allowed |= name == slot || name == slot + ".bak" || name == slot + ".passed";
                }
            } else {
                allowed |= name == "save000.mm7" || name == "save000.mm7.bak";
            }
            if (!entry.is_regular_file() || entry.is_symlink() || !allowed)
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
        if (opts.offscreenCapture && (!isOffscreenCaptureFilter(opts.gameTitle, GTEST_FLAG_GET(filter)) || !GTEST_FLAG_GET(also_run_disabled_tests)))
            throw Exception("--offscreen-gl requires one of the title's isolated disabled capture test filters.");
        const std::string filter = GTEST_FLAG_GET(filter);
        auto gate = std::ranges::find_if(mm7ProfileGates, [&filter](const Mm7ProfileGate &entry) {
            return filter == entry.filter;
        });
        const bool hasMm7ProfileGate = gate != mm7ProfileGates.end();
        if (opts.disposableMm7Profile || hasMm7ProfileGate) {
            if (!opts.disposableMm7Profile || !hasMm7ProfileGate ||
                !GTEST_FLAG_GET(also_run_disabled_tests))
                throw Exception("MM7 disposable save gate requires its exact disabled test filter and temporary profile.");
            validateDisposableMm7Profile(&opts, *gate);
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
            TestController test(game, &tfs, opts.speed, opts.disposableMm7Profile,
                                hasMm7ProfileGate && gate->preserveSave);
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
