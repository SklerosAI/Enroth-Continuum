#pragma once

#include <string>
#include <typeinfo>

#include "Library/Color/Color.h"
#include "Library/Geometry/Point.h"

enum class EngineCall {
    CALL_PLAY_SOUND,
    CALL_DRAW_2D_TEXTURE,
    CALL_DRAW_2D_QUAD,
    CALL_DRAW_MESSAGE_BOX,
    CALL_GUIWINDOW_DRAWTEXT,
    CALL_GUIWINDOW_DRAWN_TEXT,
    CALL_GUIWINDOW_DRAWN_TITLE_LINE,
    CALL_SPECIAL_ATTACK,
    CALL_PROCESS_UIMSG,
    CALL_PAUSE_MUSIC,
    CALL_PLAY_MUSIC,

    CALL_FIRST = CALL_PLAY_SOUND,
    CALL_LAST = CALL_PLAY_MUSIC
};
using enum EngineCall;

/**
 * Payload of `CALL_DRAW_2D_QUAD`, a texture drawn by `Renderer::DrawQuad2D` or `Renderer::DrawImage` and the top left
 * corner it was drawn at.
 */
struct DrawnQuad {
    std::string texture;
    Pointi position;

    friend bool operator==(const DrawnQuad &l, const DrawnQuad &r) = default;
};

/**
 * Payload of `CALL_GUIWINDOW_DRAWN_TEXT`, a string drawn by `GUIWindow::DrawText`, the color it was drawn in and the
 * position it was drawn at.
 */
struct DrawnText {
    std::string text;
    Color color;
    Pointi position;

    friend bool operator==(const DrawnText &l, const DrawnText &r) = default;
};

/**
 * Payload of `CALL_GUIWINDOW_DRAWN_TITLE_LINE`, a line drawn by `GUIWindow::DrawTitleText`, where it starts after
 * centering and the color it was drawn in.
 */
struct DrawnLine {
    std::string text;
    Pointi position;
    Color color;

    friend bool operator==(const DrawnLine &l, const DrawnLine &r) = default;
};

/**
 * An interface that's used to implement poor man's gmock for recording calls into different engine functions.
 * Currently it's used by the testing framework to check that the calls that the testing framework expects to be made
 * are actually made.
 *
 * When adding new calls to `EngineCall`, please make sure you don't add any complex logic around them. Otherwise we'll
 * end up with test code spilling over into the engine, and we generally want to avoid that. See how existing calls
 * are handled for examples.
 *
 * @see TestCallObserver
 * @see CommonTapeRecorder::sounds
 */
class EngineCallObserver {
 public:
    virtual ~EngineCallObserver() = default;

    template<class T>
    void notify(EngineCall call, const T &data) {
        notify(call, typeid(T), &data);
    }

 protected:
    virtual void notify(EngineCall call, const std::type_info &type, const void *data) = 0;
};
