#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Application/Startup/GameStarter.h"

#include "Engine/Components/Control/EngineController.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
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

/**
 * Where a title's save gate keeps its disposable profile, and what the profile may hold besides saves.
 */
struct DisposableProfileLayout {
    std::string_view gate; // Gate name for errors.
    std::string_view prefix; // Directory name prefix under the system temp root.
    std::string_view marker; // Marker file in the profile root.
    std::string_view markerText; // First line of the marker file.
    std::string_view ini; // Config file the game writes into the profile root, empty when it writes none.
    std::string_view extension; // Save file extension.
};

constexpr DisposableProfileLayout mm7ProfileLayout = {
    "MM7 save gate", "mm7-save-gate-", ".ec-mm7-disposable-save-gate", "EnrothContinuum-MM7-save-gate-v1", "enroth-continuum-mm7.ini", "mm7"
};

constexpr DisposableProfileLayout mm6ChainProfileLayout = {
    "MM6 campaign chain",
    "mm6-restart-chain-", // The MM6 save code takes a GameTest profile root only under the mm6-restart- prefix.
    ".ec-mm6-campaign-chain", "EnrothContinuum-MM6-campaign-chain-v1", "", "ec6"
};

constexpr std::array<std::string_view, 10> mm6ChainGates = {{
    "MM6CampaignChain.DISABLED_Chapter0",
    "MM6CampaignChain.DISABLED_Chapter1",
    "MM6CampaignChain.DISABLED_Chapter2",
    "MM6CampaignChain.DISABLED_Chapter3",
    "MM6CampaignChain.DISABLED_Chapter4",
    "MM6CampaignChain.DISABLED_Chapter5",
    "MM6CampaignChain.DISABLED_Chapter6",
    "MM6CampaignChain.DISABLED_Chapter7",
    "MM6CampaignChain.DISABLED_Chapter8",
    "MM6CampaignChain.DISABLED_Chapter9",
}};

/**
 * Checks a disposable save gate profile before a gated test runs in it.
 *
 * @param supplied                      Profile directory passed on the command line.
 * @param layout                        Title layout of the profile.
 * @param writer                        Whether the test starts from a profile that holds nothing but the marker.
 * @param chapter                       Campaign chain chapter, see `Mm7ProfileGate::chapter`.
 * @return                              Canonical profile root. Throws when the profile is refused.
 */
std::filesystem::path validateDisposableProfile(const std::filesystem::path &supplied, const DisposableProfileLayout &layout,
                                                bool writer, int chapter) {
    namespace fs = std::filesystem;
    if (!supplied.is_absolute() || !fs::is_directory(supplied))
        throw Exception("{} requires an absolute disposable profile directory.", layout.gate);
    const fs::path root = fs::canonical(supplied);
    if (root.parent_path() != fs::canonical(fs::temp_directory_path()) ||
        !root.filename().string().starts_with(layout.prefix) ||
        (fs::status(root).permissions() & (fs::perms::group_all | fs::perms::others_all)) != fs::perms::none)
        throw Exception("{} requires an owner-only system-temp profile.", layout.gate);
    const fs::path markerPath = root / layout.marker;
    if (!fs::is_regular_file(markerPath) || fs::is_symlink(markerPath))
        throw Exception("{} profile marker is missing or invalid.", layout.gate);
    std::ifstream marker(markerPath);
    std::string value;
    if (!std::getline(marker, value) || value != layout.markerText)
        throw Exception("{} profile marker is missing or invalid.", layout.gate);
    for (const fs::directory_entry &entry : fs::directory_iterator(root)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_symlink() ||
            (writer && name != layout.marker) ||
            (!writer && name != layout.marker && (layout.ini.empty() || name != layout.ini) && name != "saves"))
            throw Exception("{} profile contains an unexpected entry.", layout.gate);
    }
    if (!writer) {
        const fs::path saves = root / "saves";
        const std::string candidate = fmt::format("save{:03}.{}", std::max(chapter - 1, 0), layout.extension);
        if (!fs::is_directory(saves) || fs::is_symlink(saves) ||
            !fs::is_regular_file(saves / candidate) || fs::is_symlink(saves / candidate))
            throw Exception("{} profile has no regular candidate save.", layout.gate);
        const std::string autosave = fmt::format("autosave.{}", layout.extension);
        for (const fs::directory_entry &entry : fs::directory_iterator(saves)) {
            const std::string name = entry.path().filename().string();
            bool allowed = name == autosave || name == autosave + ".bak";
            if (chapter >= 0) {
                for (int earlier = 0; earlier < chapter; ++earlier) {
                    const std::string slot = fmt::format("save{:03}.{}", earlier, layout.extension);
                    allowed |= name == slot || name == slot + ".bak" || name == slot + ".passed";
                }
            } else {
                allowed |= name == candidate || name == candidate + ".bak";
            }
            if (!entry.is_regular_file() || entry.is_symlink() || !allowed)
                throw Exception("{} profile contains an unexpected save.", layout.gate);
        }
    }
    return root;
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
            opts.userPath = Path(validateDisposableProfile(opts.userPath.str(), mm7ProfileLayout, gate->writer, gate->chapter).string());
        }
        const auto mm6ChainGate = std::ranges::find(mm6ChainGates, filter);
        const bool hasMm6ChainGate = mm6ChainGate != mm6ChainGates.end();
        std::filesystem::path mm6ChainRoot;
        if (!opts.mm6ChainProfile.isEmpty() || hasMm6ChainGate) {
            if (opts.mm6ChainProfile.isEmpty() || !hasMm6ChainGate || !GTEST_FLAG_GET(also_run_disabled_tests))
                throw Exception("MM6 campaign chain requires its exact disabled test filter and temporary profile.");
            const int chapter = static_cast<int>(mm6ChainGate - mm6ChainGates.begin());
            mm6ChainRoot = validateDisposableProfile(opts.mm6ChainProfile.str(), mm6ChainProfileLayout, chapter == 0, chapter);
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
            if (!mm6ChainRoot.empty())
                game->runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(mm6ChainRoot); });
            exitCode = RUN_ALL_TESTS();
        });
        return exitCode;
    } catch (const std::exception &e) {
        // TODO(captainurist): we need a separate test that testing framework terminates correctly if the engine throws.
        fmt::print(stderr, "{}\n", e.what());
        return 1;
    }
}
