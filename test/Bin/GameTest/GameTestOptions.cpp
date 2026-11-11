#include "GameTestOptions.h"

#include <memory>
#include <string>

#include "Library/Cli/CliApp.h"

#include "Utility/Exception.h"

GameTestOptions GameTestOptions::parse(int argc, char **argv) {
    GameTestOptions result;
    result.ramFsUserData = true; // We want reproducible tests, so shouldn't depend on external user data.
    result.quickStart = true;
    std::optional<std::string> testPath;
    std::string game = "mm7";

    std::unique_ptr<CliApp> app = std::make_unique<CliApp>();

    std::string requiredOptions = "Required Options";
    std::string otherOptions = "Other Options";

    auto testPathOption = app->add_option("--test-path", testPath,
                                          "Path to test data dir.")->check(CLI::ExistingDirectory)->option_text("PATH")->group(requiredOptions);
    app->add_option("--game", game, "Game to test: mm6 or mm7. Defaults to mm7.")->option_text("GAME")->group(otherOptions);
    app->add_option(
        "--data-path", result.dataPath,
        "Path to game data dir.")->check(CLI::ExistingDirectory)->option_text("PATH")->group(otherOptions);
    app->add_option(
        "--rules-path", result.rulesPath,
        "Private MM6 rules directory to copy into the in-memory test profile.")
        ->check(CLI::ExistingDirectory)->option_text("PATH")->group(otherOptions);
    app->add_option(
        "--mm7-disposable-profile", result.userPath,
        "Owner-only temporary MM7 profile for the isolated save/reload gate.")
        ->check(CLI::ExistingDirectory)->option_text("PATH")->group(otherOptions);
    app->add_option(
        "--mm6-chain-profile", result.mm6ChainProfile,
        "Owner-only temporary MM6 profile for one campaign chain chapter.")
        ->check(CLI::ExistingDirectory)->option_text("PATH")->group(otherOptions);
    app->add_flag(
        "--headless", result.headless,
        "Run in headless mode.")->group(otherOptions);
    app->add_flag(
        "--offscreen-gl", result.offscreenCapture,
        "Run an isolated MM6 or MM7 frame-capture test using a hidden SDL OpenGL window.")->group(otherOptions);
    app->add_option(
        "--speed", result.speed,
        "Playback speed, default is infinite, use '1.0' for realtime playback.")->option_text("SPEED");
    app->add_flag(
        "--tracing-rng", result.tracingRng,
        "Use random number generators that print stack trace on each call.")->group(otherOptions);
    app->add_option(
        "--log-level", result.logLevel,
        "Log level, one of 'none', 'trace', 'debug', 'info', 'warning', 'error', 'critical'.")->option_text("LOG_LEVEL");
    app->add_flag_callback(
        "-v,--verbose", [&] { result.logLevel = LOG_TRACE; },
        "Set log level to 'trace'.");
    app->set_help_flag("-h,--help", "Print help and exit.")->group(otherOptions);
    app->add_flag(
        "--gtest_list_tests", result.listRequested,
        "List the names of all tests instead of running them.")->group(""); // group("") hides the option. It's shown in gtest's help.
    app->allow_extras();

    app->parse(argc, argv, result.helpPrinted);

    if (!result.userPath.isEmpty()) {
        if (game != "mm7" || !result.headless || result.offscreenCapture || result.dataPath.isEmpty())
            throw Exception("--mm7-disposable-profile requires MM7, --headless and explicit --data-path.");
        result.disposableMm7Profile = true;
        result.ramFsUserData = false;
    }

    if (!result.mm6ChainProfile.isEmpty() && (game != "mm6" || !result.headless || result.dataPath.isEmpty()))
        throw Exception("--mm6-chain-profile requires MM6, --headless and explicit --data-path.");

    if (game == "mm6") {
        result.gameTitle = GameTitle::MM6;
        if (result.headless && result.offscreenCapture)
            throw Exception("Choose either --headless or --offscreen-gl.");
        if (!result.headless && !result.offscreenCapture && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require --headless or --offscreen-gl.");
#if !defined(__APPLE__)
        if (result.offscreenCapture)
            throw Exception("MM6 --offscreen-gl is currently supported only on macOS.");
#endif
        if (result.dataPath.isEmpty() && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require an explicit --data-path.");
        if (result.rulesPath.isEmpty() && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require an explicit --rules-path.");
    } else if (game != "mm7") {
        throw Exception("Unsupported game '{}'; choose mm6 or mm7.", game);
    } else if (!result.rulesPath.isEmpty()) {
        throw Exception("--rules-path is only supported for MM6 game tests.");
    } else if (result.offscreenCapture) {
        if (result.headless)
            throw Exception("Choose either --headless or --offscreen-gl.");
#if !defined(__APPLE__)
        throw Exception("MM7 --offscreen-gl is currently supported only on macOS.");
#endif
        if (result.dataPath.isEmpty() && !result.helpPrinted && !result.listRequested)
            throw Exception("MM7 offscreen game tests require an explicit --data-path.");
    }

    if (!result.listRequested && !result.helpPrinted && !testPath)
        throw CLI::RequiredError(testPathOption->get_name());
    result.testPath = Path(testPath.value_or(""));

    return result;
}
