#include "GameStarter.h"

#include <filesystem>
#include <utility>
#include <string>
#include <vector>
#include <memory>

#include "Application/GameConfig.h"
#include "Application/Game.h"
#include "Application/GameWindowHandler.h"
#include "Application/GameTraceHandler.h"

#include "Core/Time/Time.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/EngineIocContainer.h"
#include "Engine/MM6/MM6Calendar.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Graphics/Renderer/RendererFactory.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Components/Trace/EngineTracePlayer.h"
#include "Engine/Components/Trace/EngineTraceRecorder.h"
#include "Engine/Components/Trace/EngineTraceSimplePlayer.h"
#include "Engine/Components/Trace/EngineTraceSimpleRecorder.h"
#include "Engine/Components/Trace/EngineTraceStateAccessor.h"
#include "Engine/Components/Control/EngineControlComponent.h"
#include "Engine/Components/Control/EngineController.h"
#include "Engine/Components/Deterministic/EngineDeterministicComponent.h"
#include "Engine/Components/Random/EngineRandomComponent.h"

#include "GUI/Overlay/OverlaySystem.h"

#include "Io/KeyboardController.h"

#include "Library/Environment/Interface/Environment.h"
#include "Library/Platform/Application/PlatformApplication.h"
#include "Library/Logger/Logger.h"
#include "Library/Image/Png.h"
#include "Library/Platform/Interface/Platform.h"
#include "Library/Platform/Null/NullPlatform.h"
#include "Library/Platform/Proxy/ProxyPlatform.h"
#include "Library/Platform/Proxy/ProxyWindow.h"
#include "Library/FileSystem/Memory/MemoryFileSystem.h"

#include "Scripting/AudioBindings.h"
#include "Scripting/ConfigBindings.h"
#include "Scripting/OverlayBindings.h"
#include "Scripting/GameBindings.h"
#include "Scripting/InputBindings.h"
#include "Scripting/InputScriptEventHandler.h"
#include "Scripting/LoggerBindings.h"
#include "Scripting/PlatformBindings.h"
#include "Scripting/RendererBindings.h"
#include "Scripting/ScriptingSystem.h"

#include "Utility/Exception.h"
#include "Utility/System/Fs.h"

#include "PathResolver.h"

constexpr std::string_view legacyConfigName = "openenroth.ini";
constexpr std::string_view mm7MacConfigName = "enroth-continuum-mm7.ini";
constexpr std::string_view mm7MacLogName = "logs/enroth-continuum-mm7.log";

static std::string_view configNameForTitle(GameTitle title) {
#if defined(__APPLE__)
    if (title == GameTitle::MM7)
        return mm7MacConfigName;
#endif
    return legacyConfigName;
}

// Installed only by the explicitly opted-in, isolated GameTest. SDL still
// owns a real hidden OpenGL window/context, but game code cannot reveal/raise
// it or alter the physical cursor and mouse.
class OffscreenCaptureGuard final : public ProxyPlatform, public ProxyWindow {
 public:
    void setCursorShown(bool) override {}
    bool isCursorShown() const override { return true; }
    void showMessageBox(const std::string &, const std::string &) const override {
        throw Exception("Offscreen capture refused a native message box");
    }
    void setVisible(bool visible) override {
        if (!visible)
            nonNullWindow()->setVisible(false);
    }
    void setWindowMode(PlatformWindowMode mode) override {
        if (mode != WINDOW_MODE_WINDOWED)
            throw Exception("Offscreen capture requires windowed mode");
        nonNullWindow()->setWindowMode(mode);
    }
    void setGrabsMouse(bool) override {}
    void setMouseRelative(bool) override {}
    void activate() override {}
    void warpMouse(Pointi) override {}

 private:
    PlatformWindow *nonNullWindow() const { return ProxyWindow::nonNullBase(); }
};

GameStarter::GameStarter(GameStarterOptions options, std::function<void(FileSystem *)> prepareUserFs): _options(std::move(options)) {
    // Note: basic logging is initialized in LogStarter constructor, so it's safe to log here.
    Engine::LogEngineBuildInfo();

    try {
        initialize(prepareUserFs);
    } catch (const std::exception &e) {
        MM_CRITICAL("Terminated with exception: {}", e.what());
        throw;
    }
}

void GameStarter::initialize(const std::function<void(FileSystem *)> &prepareUserFs) {
    if (_options.gameTitle == GameTitle::MM6) {
        mm6::resetRawCursorShopProcessOwners(GameTitle::MM6);
        gameStartingYear = mm6::calendarStartingYear;
    }

    // Init environment.
    _environment = Environment::createStandardEnvironment();

    // Resolve user path & create user fs.
    resolveUserPath(_environment.get(), &_options);
    _fsStarter.initUserFs(_options.ramFsUserData, _options.userPath);
    MM_INFO("Using user path '{}'.", ufs->displayPath(""));
    if (_options.gameTitle == GameTitle::MM6)
        mm6::setStartupNativeProfileRoot(_options.ramFsUserData, std::filesystem::path(_options.userPath.native()));
    if (prepareUserFs)
        prepareUserFs(ufs);

    // Init config.
    _config = std::make_shared<GameConfig>();
    const std::string_view configName = configNameForTitle(_options.gameTitle);
    std::string_view loadConfigName = configName;
#if defined(__APPLE__)
    // An explicitly selected older MM7 profile can still supply its config.
    // Save only the new name; do not modify or import the old profile.
    if (_options.gameTitle == GameTitle::MM7 && !ufs->exists(configName) && ufs->exists(legacyConfigName))
        loadConfigName = legacyConfigName;
#endif
    if (ufs->exists(loadConfigName)) {
        _config->load(ufs->openForReading(loadConfigName).get());
        MM_INFO("Configuration file '{}' loaded!", ufs->displayPath(loadConfigName));
    } else {
        _config->reset();
#ifdef OE_DISTRIBUTION_BUILD
        _config->window.Mode.setValue(WINDOW_MODE_FULLSCREEN_BORDERLESS);
#endif
        MM_INFO("Could not read configuration file '{}'! Loaded default configuration instead!", ufs->displayPath(configName));
    }
#if defined(__APPLE__)
    if (_options.gameTitle == GameTitle::MM7 && _config->window.Title.value() == "OpenEnroth")
        _config->window.Title.setValue("Enroth Continuum MM7");
#endif
    MM_INFO("Built in resource override is {}.", _config->debug.OverrideBuiltInResources.value() ? "enabled" : "disabled");

    // Patch config.
    if (_options.quickStart) {
        _config->graphics.GenerateTiles.setValue(false);
        _config->graphics.Saturation.setValue(1.0f);
        _config->graphics.Lightness.setValue(1.0f);
    }
    if (_options.offscreenCapture) {
        if (_options.headless || (_options.gameTitle != GameTitle::MM6 &&
                                  _options.gameTitle != GameTitle::MM7))
            throw Exception("Offscreen capture requires a non-headless MM6 or MM7 test");
        _config->graphics.Renderer.setValue(RENDERER_OPENGL);
        _config->window.Mode.setValue(WINDOW_MODE_WINDOWED);
        _config->window.Width.setValue(640);
        _config->window.Height.setValue(480);
        _config->window.MouseGrab.setValue(false);
        _config->window.Resizable.setValue(false);
    }

    // Finish logger init now that we have user fs and know the desired log level.
    std::string_view logName = "logs/openenroth.log";
#if defined(__APPLE__)
    if (_options.gameTitle == GameTitle::MM7)
        logName = mm7MacLogName;
#endif
    _logStarter.initialize(ufs, _options.logLevel ? *_options.logLevel : _config->debug.LogLevel.value(), logName);

    // Resolve data path, create data fs.
    // TODO(captainurist): actually move datapath to config?
    resolveDataPath(_environment.get(), &_options);
    _fsStarter.initDataFs(_options.dataPath, _config->debug.OverrideBuiltInResources.value());
    MM_INFO("Using data path '{}'.", _options.dataPath); // Can't use dfs->displayPath("") b/c it'll show "embedded://"...

    // On macOS, the Enroth Continuum MM7 profile is populated only by an
    // explicit, verified migration. Existing save folders can have different
    // files with the same name, so do not auto-copy from the data directory.
#if !defined(__APPLE__)
    if (_options.gameTitle == GameTitle::MM7 && !_options.ramFsUserData && _options.dataPath.normalized() != _options.userPath.normalized())
        migrateSaves();
#endif

    // Create platform.
    if (_options.headless) {
        _platform = std::make_unique<NullPlatform>(NullPlatformOptions());
    } else {
        _platform = Platform::createStandardPlatform();
    }

    // Prepare OpenAL settings. Unfortunately the only way to do this is by manipulating the env variables.
    // If we don't do this, game tests in GH action on macos-14 will hang for 9 min on init.
    if (_options.headless || _options.offscreenCapture)
        _environment->setenv("ALSOFT_DRIVERS", "null");

    // Can validate the resolved data path now.
    failOnInvalidPath(_options.dataPath, _options.gameTitle, _platform.get(), !_options.offscreenCapture);

    // Create application.
    _application = std::make_unique<PlatformApplication>(_platform.get());
    ::application = _application.get();
    ::platform = _application->platform();
    ::eventLoop = _application->eventLoop();
    ::window = _application->window();
    ::eventHandler = _application->eventHandler();
    ::openGLContext = _application->openGLContext(); // OK to store into a global even if not yet initialized

    // On linux the only way to set window icon is through an API call. On other OSes this is handled by external
    // mechanisms.
#if defined(__linux__) && !defined(__ANDROID__)
    window->setIcon(png::decode(dfs->read("images/OpenEnroth.png")));
#endif

    // Install & set up components.
    //
    // Notes:
    // 1. Component installed last will be the 1st in the processing chain.
    // 2. `EngineControlComponent` should be the last in the chain b/c it's generating events from inside the
    //    `processMessages()` handler.
    // 3. `EngineTraceSimpleRecorder` should be placed in the chain after `EngineDeterministicComponent`. Deterministic
    //    component updates tick count in `swapBuffers()`, and then trace component stores the updated value in a
    //    recorded `PaintEvent`.
    // 4. `KeyboardController` should be placed after GameWindowHandler.
    // 5. `GameTraceHandler` should come before other input handlers, otherwise Ctrl+Shift+R will open up the rest menu.
    _application->installComponent(std::make_unique<EngineControlComponent>());
    _application->installComponent(std::make_unique<EngineTraceSimpleRecorder>());
    _application->installComponent(std::make_unique<EngineTraceSimplePlayer>());
    _application->installComponent(std::make_unique<EngineDeterministicComponent>());
    _application->installComponent(std::make_unique<EngineTraceRecorder>());
    _application->installComponent(std::make_unique<EngineTracePlayer>());
    _application->installComponent(std::make_unique<EngineRandomComponent>());
    _application->installComponent(std::make_unique<KeyboardController>());
    _application->installComponent(std::make_unique<GameWindowHandler>());
    _application->installComponent(std::make_unique<GameTraceHandler>());
    if (_options.offscreenCapture)
        _application->installComponent(std::make_unique<OffscreenCaptureGuard>());
    _application->component<EngineRandomComponent>()->setTracing(_options.tracingRng);

    // Init main window. Should happen before the renderer init, which depends on window dimensions & mode.
    _application->component<GameWindowHandler>()->UpdateWindowFromConfig(_config.get());

    // Init renderer.
    RendererType rendererType = _options.headless ? RENDERER_NULL : _config->graphics.Renderer.value();
    _renderer = RendererFactory().createRenderer(rendererType, _config);
    ::render = _renderer.get();
    _renderer->Initialize();

    // Init overlays.
    _overlaySystem = std::make_unique<OverlaySystem>(*_renderer, *_application);

    // Init io.
    ::keyboardActionMapping = std::make_shared<Io::KeyboardActionMapping>(_config);;
    ::keyboardInputHandler = std::make_shared<Io::KeyboardInputHandler>(_application->component<KeyboardController>(),
        keyboardActionMapping
    );
    ::mouse = EngineIocContainer::ResolveMouse();

    // Init engine.
    _engine = std::make_unique<Engine>(_config, *_overlaySystem, _options.gameTitle);
    ::engine = _engine.get();
    _engine->Initialize();

    // Init game.
    _game = std::make_unique<Game>(_application.get(), _config);

    // Init scripting system.
    _scriptingSystem = std::make_unique<ScriptingSystem>("scripts", "init.lua", *_application, *_logStarter.rootSink());
    _scriptingSystem->addBindings<LoggerBindings>("log");
    _scriptingSystem->addBindings<GameBindings>("game");
    _scriptingSystem->addBindings<ConfigBindings>("config");
    _scriptingSystem->addBindings<PlatformBindings>("platform", *_application);
    _scriptingSystem->addBindings<InputBindings>("input", *_application->component<InputScriptEventHandler>());
    _scriptingSystem->addBindings<OverlayBindings>("overlay", *_overlaySystem);
    _scriptingSystem->addBindings<AudioBindings>("audio");
    _scriptingSystem->addBindings<RendererBindings>("renderer");
    _scriptingSystem->executeEntryPoint();
}

GameStarter::~GameStarter() {
    _application->removeComponent<EngineControlComponent>(); // Join the control thread first.

    _game.reset();
    _engine.reset();
    _overlaySystem.reset();
    _scriptingSystem.reset();
    _renderer.reset();

    ::engine = nullptr;
    ::render = nullptr;
    ::application = nullptr;
    ::platform = nullptr;
    ::eventLoop = nullptr;
    ::window = nullptr;
    ::eventHandler = nullptr;
    ::openGLContext = nullptr;
}

void GameStarter::resolveUserPath(Environment *environment, GameStarterOptions *options) {
    if (options->userPath.isEmpty())
        options->userPath = options->gameTitle == GameTitle::MM6 ? resolveMm6UserPath(environment) : resolveMm7UserPath(environment);
}

void GameStarter::resolveDataPath(Environment *environment, GameStarterOptions *options) {
    std::vector<Path> candidates;
    if (!options->dataPath.isEmpty()) {
        candidates.push_back(options->dataPath);
    } else {
        candidates = options->gameTitle == GameTitle::MM6 ? resolveMm6Paths(environment) : resolveMm7Paths(environment);
    }
    assert(!candidates.empty());

    for (int i = 0; i < candidates.size(); i++) {
        std::string missingFile;
        if (!fs::exists(candidates[i])) {
            MM_INFO("Data path #{} ('{}') doesn't exist.", i + 1, candidates[i]);
        } else if (!(options->gameTitle == GameTitle::MM6 ? validateMm6Path(candidates[i], &missingFile) : validateMm7Path(candidates[i], &missingFile))) {
            MM_INFO("Data path #{} ('{}') is missing file '{}'.", i + 1, candidates[i], missingFile);
        } else {
            MM_INFO("Data path #{} ('{}') is OK!", i + 1, candidates[i]);
            options->dataPath = std::move(candidates[i]);
            break;
        }
    }

    // Just use the last data path if all paths are invalid. We'll throw later.
    if (options->dataPath.isEmpty())
        options->dataPath = candidates.back();
}

void GameStarter::failOnInvalidPath(PathView dataPath, GameTitle gameTitle, Platform *platform, bool allowDialog) {
    std::string missingFile;
    if (gameTitle == GameTitle::MM6 ? validateMm6Path(dataPath, &missingFile) : validateMm7Path(dataPath, &missingFile))
        return;

    std::string message = fmt::format(
        "Required file '{}' not found.\n"
        "\n"
        "Select a licensed M&M {} data folder at {}.",
        missingFile,
        gameTitle == GameTitle::MM6 ? "VI" : "VII",
        dataPath
    );
    if (allowDialog)
        platform->showMessageBox("CRITICAL ERROR: missing resources", message);
    throw Exception("Data folder '{}' validation failed, missing '{}'", dataPath, missingFile);
}

void GameStarter::migrateSaves() {
    MM_INFO("Migrating save files from '{}' to '{}'...", dfs->displayPath("saves"), ufs->displayPath("saves"));

    if (ufs->exists("saves") && !ufs->ls("saves").empty()) {
        MM_INFO("    Target saves directory is not empty, skipping saves migration.");
    } else if (!dfs->exists("saves")) {
        MM_INFO("    No save files to migrate.");
    } else {
        for (const DirectoryEntry &entry : dfs->ls("saves")) {
            if (entry.type == FILE_REGULAR) {
                std::string path = fmt::format("saves/{}", entry.name);
                ufs->write(path, dfs->read(path));
                MM_INFO("    Copied '{}'.", entry.name);
            }
        }
    }
}

void GameStarter::run() {
    try {
        _game->run();

        _application->component<GameWindowHandler>()->UpdateConfigFromWindow(_config.get());
        const std::string_view configName = configNameForTitle(_options.gameTitle);
        _config->save(ufs->openForWriting(configName).get());
        MM_INFO("Configuration file '{}' saved!", ufs->displayPath(configName));
    } catch (const std::exception &e) {
        // Log the exception so that it goes to all registered loggers.
        MM_CRITICAL("Terminated with exception: {}", e.what());
        throw;
    }
}

void GameStarter::runInstrumented(std::function<void(EngineController *)> controlRoutine) {
    // Instrumentation implies that we'll be running traces, either hand-crafted, or from files. So calling
    // `prepareForPlayback` here makes sense. This also disables intro videos.
    EngineTraceStateAccessor::prepareForPlayback(_config.get(), {});

    _application->component<EngineControlComponent>()->runControlRoutine([controlRoutine = std::move(controlRoutine)] (EngineController *game) {
        game->tick(10); // Let the game thread initialize everything.

        controlRoutine(game);

        game->goToMainMenu();
        game->pressGuiButton("MainMenu_ExitGame");
    });

    run();
}
