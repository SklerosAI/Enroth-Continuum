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
    app->add_flag(
        "--headless", result.headless,
        "Run in headless mode.")->group(otherOptions);
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

    if (game == "mm6") {
        result.gameTitle = GameTitle::MM6;
        if (!result.headless && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require --headless.");
        if (result.dataPath.isEmpty() && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require an explicit --data-path.");
        if (result.rulesPath.isEmpty() && !result.helpPrinted && !result.listRequested)
            throw Exception("MM6 game tests require an explicit --rules-path.");
    } else if (game != "mm7") {
        throw Exception("Unsupported game '{}'; choose mm6 or mm7.", game);
    } else if (!result.rulesPath.isEmpty()) {
        throw Exception("--rules-path is only supported for MM6 game tests.");
    }

    if (!result.listRequested && !result.helpPrinted && !testPath)
        throw CLI::RequiredError(testPathOption->get_name());
    result.testPath = Path(testPath.value_or(""));

    return result;
}
