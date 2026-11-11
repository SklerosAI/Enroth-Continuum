#pragma once

#include <charconv>
#include <string_view>
#include <vector>

#include "Engine/Components/Control/EngineController.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Library/Image/Image.h"
#include "Library/Image/Png.h"

#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

/**
 * Draws the current game screen with the world and the HUD into an image, without presenting it.
 *
 * @param game                          Controller to run the draw in.
 * @return                              The 640x480 frame.
 */
inline RgbaImage drawOffscreenGameFrame(EngineController &game) {
    RgbaImage frame;
    game.runGameRoutine([&frame] {
        uGameState = GAME_STATE_PLAYING;
        current_screen_type = SCREEN_GAME;
        render->ClearTarget(Color(0, 0, 0));
        engine->drawWorld();
        engine->drawHUD();
        render->flushAndScale();
        engine->drawOverlay();
        frame = render->MakeFullScreenshot(); // Read before swap; no display presentation.
    });
    return frame;
}

/**
 * @param path                          Destination to check.
 * @return                              Whether `path` is a normalized `.png` path under `/tmp/` or `/private/tmp/`.
 */
inline bool isOffscreenCapturePath(const char *path) {
    const std::string_view view(path);
    return (view.starts_with("/tmp/") || view.starts_with("/private/tmp/")) && Path(path).isNormalized() && Path(path).extension() == ".png";
}

inline void writeOffscreenCapture(const char *path, const RgbaImage &frame) {
    FileOutputStream output{Path(path)};
    output.write(png::encode(frame));
    output.close();
}

/**
 * @param value                         A map file name, or an index into `maps` that wraps around.
 * @param maps                          The title's maps, in table order.
 * @return                              The chosen map, `MAP_INVALID` if `value` names none.
 */
inline MapId offscreenCaptureMap(std::string_view value, const std::vector<MapId> &maps) {
    size_t index = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), index);
    if (error == std::errc() && end == value.data() + value.size())
        return maps.empty() ? MAP_INVALID : maps[index % maps.size()];
    return pMapTable->GetMapInfo(value);
}
