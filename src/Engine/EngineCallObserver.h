#pragma once

#include <string>
#include <typeinfo>

#include "Library/Geometry/Point.h"

enum class EngineCall {
    CALL_PLAY_SOUND,
    CALL_DRAW_2D_TEXTURE,
    CALL_DRAW_2D_QUAD,
    CALL_DRAW_MESSAGE_BOX,
    CALL_GUIWINDOW_DRAWTEXT,
    CALL_SPECIAL_ATTACK,
    CALL_PROCESS_UIMSG,
    CALL_PAUSE_MUSIC,

    CALL_FIRST = CALL_PLAY_SOUND,
    CALL_LAST = CALL_PAUSE_MUSIC
};
using enum EngineCall;

/**
 * Payload of `CALL_DRAW_2D_QUAD`, a texture drawn by `Renderer::DrawQuad2D` and the top left corner it was drawn at.
 */
struct DrawnQuad {
    std::string texture;
    Pointi position;

    friend bool operator==(const DrawnQuad &l, const DrawnQuad &r) = default;
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
