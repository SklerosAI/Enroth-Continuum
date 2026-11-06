#include "OpenEnrothOptions.h"

#include <memory>
#include <utility>
#include <vector>
#include <string>

#include "Library/Cli/CliApp.h"

#include "Utility/Exception.h"
#include "Utility/System/Fs.h"

OpenEnrothOptions OpenEnrothOptions::parse(int argc, char **argv) {
    OpenEnrothOptions result;
    std::unique_ptr<CliApp> app = std::make_unique<CliApp>();
#ifdef EC_MM6_ONLY
    std::string game = "mm6";
#else
    std::string game = "mm7";
#endif

#ifdef EC_MM6_ONLY
    app->add_option("--game", game, "Game to launch: mm6 only.")->option_text("GAME");
#else
    app->add_option("--game", game, "Game to launch: mm6 or mm7. Defaults to mm7.")->option_text("GAME");
#endif

    std::optional<bool> portable;
    app->add_option(
        "--data-path", result.dataPath,
        "Selected game's data folder. If omitted, check its environment override, "
        "current folder, platform-specific installation paths, then its Mac support folder.")->check(CLI::ExistingDirectory)->option_text("PATH");
    app->add_option(
        "--user-path", result.userPath,
        "User data folder. Defaults to the selected game's separate profile.")->check(CLI::ExistingDirectory)->option_text("PATH");
    app->add_flag(
        "--portable", portable,
        "Run in portable mode, game & user data paths will default to current folder. "
        "If '.portable' file exists in the current folder, then this parameter defaults to 'true'.");
    app->add_flag(
        "--tracing-rng", result.tracingRng,
        "Use random number generators that print stack trace on each call.");
    app->add_option(
        "--log-level", result.logLevel,
        "Log level, one of 'none', 'trace', 'debug', 'info', 'warning', 'error', 'critical'.")->option_text("LOG_LEVEL");
    app->add_flag_callback(
        "-v,--verbose", [&] { result.logLevel = LOG_TRACE; },
        "Set log level to 'trace'.");
    app->set_help_flag("-h,--help", "Print help and exit.");

    CLI::App *play = app->add_subcommand("play", "Play provided traces.", result.subcommand, SUBCOMMAND_PLAY)->fallthrough();
    play->add_option(
        "--speed", result.play.speed,
        "Playback speed, default is '1.0'.")->option_text("SPEED");
    play->add_option(
        "TRACE", result.play.traces,
        "Path to trace file(s) to play.")->required()->option_text("...");
    play->set_help_flag("-h,--help", "Print help and exit."); // This places --help last in the command list.

    CLI::App *retrace = app->add_subcommand("retrace", "Retrace traces and exit.", result.subcommand, SUBCOMMAND_RETRACE)->fallthrough();
    app->add_flag(
        "--headless", result.headless,
        "Run in headless mode.");
    retrace->add_option(
        "TRACE", result.retrace.traces,
        "Path to trace file(s) to retrace.")->option_text("...");
    retrace->set_help_flag("-h,--help", "Print help and exit."); // This places --help last in the command list.

    app->parse(argc, argv, result.helpPrinted);

#ifdef EC_MM6_ONLY
    if (game != "mm6")
        throw Exception("This MM6 app only accepts --game mm6.");
    result.gameTitle = GameTitle::MM6;
#else
    if (game == "mm6") {
        result.gameTitle = GameTitle::MM6;
    } else if (game != "mm7") {
        throw Exception("Unsupported game '{}'; choose mm6 or mm7.", game);
    }
#endif

    // Trace playback still reads MM7 snapshots. Keep the MM6-only executable
    // from interpreting one as an MM6 save until the EC6 trace path exists.
    if (result.gameTitle == GameTitle::MM6 && result.subcommand != SUBCOMMAND_GAME)
        throw Exception("MM6 trace playback is unavailable until EC6 snapshots are implemented.");

    if (!portable && fs::exists(".portable"))
        portable = true;
    if (portable && *portable) {
        if (result.userPath.isEmpty())
            result.userPath = fs::cwd();
        if (result.dataPath.isEmpty())
            result.dataPath = fs::cwd();
    }

    if (result.subcommand == SUBCOMMAND_RETRACE) {
        result.ramFsUserData = true; // No config & no user data if retracing.
        result.quickStart = true;

        if (result.retrace.traces.empty())
            throw Exception("No trace files to retrace.");

        if (!result.logLevel)
            result.logLevel = LOG_ERROR; // Default log level for retracing is LOG_ERROR.
    }

    if (result.subcommand == SUBCOMMAND_PLAY) {
        result.ramFsUserData = true; // No config & no user data if playing a trace.
        result.quickStart = true;
    }

    return result;
}
