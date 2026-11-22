#pragma once

#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <utility>
#include <string>
#include <type_traits>

#include "Engine/EngineCallObserver.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/MapEnums.h"
#include "Engine/PartyEnums.h"
#include "Engine/Data/SoundEnums.h"
#include "Engine/Graphics/FaceEnums.h"
#include "GUI/GUIEnums.h"
#include "GUI/GUIDialogues.h"

#include "Core/Time/Time.h"

#include "Library/Config/ConfigEntry.h"

#include "TestController.h"

void PrintTo(const DrawnQuad &quad, std::ostream *stream);
void PrintTo(const DrawnText &text, std::ostream *stream);
void PrintTo(const DrawnLine &line, std::ostream *stream);

class CommonTapeRecorder {
 public:
    explicit CommonTapeRecorder(TestController *controller);

    /**
     * Creates a tape object that will record changes in a value computed by the provided callback throughout the
     * execution of a trace.
     *
     * The callback is called on every frame, and also before and after running the trace, and unique values are stored
     * on a tape. This effectively means that if, for example, the provided callback always returns the same value,
     * then you'll get a tape of size 1.
     *
     * A typical use case is to create a tape, call `TestController::playTraceFromTestData` to play a trace, then check
     * the data stored on the tape.
     *
     * @param callback                  Callback that will calculate the values to store on a tape.
     * @return                          Tape object.
     */
    template<class Callback, class T = std::decay_t<std::invoke_result_t<Callback>>>
    TestTape<T> custom(Callback callback) {
        return _controller->recordTape(std::move(callback));
    }

    TestTape<int64_t> totalExperience();

    TestTape<int> totalHp();

    TestTape<int> totalItemCount();

    TestTape<int> totalItemCount(ItemId itemId);

    TestTape<int> totalItemCount(ItemType itemType);

    TestTape<int> totalHirelings();

    TestTape<bool> hasItem(ItemId itemId);

    TestTape<int> gold();

    TestTape<int> food();

    TestTape<int> reputation();

    TestTape<int> deaths();

    TestTape<MapId> map();

    TestTape<ScreenType> screen();

    TestTape<std::string> statusBar();

    TestTape<DialogueId> dialogueType();

    TestTape<Time> time();

    TestTape<bool> turnBasedMode();

    template<class T>
    TestTape<T> config(const ConfigEntry<T> &entry) {
        return custom([&] { return entry.value(); });
    }

    TestTape<int> mapItemCount();

    TestTape<int> mapItemCount(ItemId itemId);

    TestTape<HouseId> house();

    TestMultiTape<SpriteId> sprites();

    TestMultiTape<SpriteId> drawnSprites(); // Sprite objects that made it into the frame's billboard list.

    TestTape<int> activeCharacterIndex(); // Remember that -1 means none!

    TestTape<bool> questBit(QuestBit bit);

    TestTape<DoorState> doorState(int doorId); // The door must exist on every taped frame, and loading screens have no doors.

    TestMultiTape<DoorState> doorStates(std::initializer_list<int> doorIds);

    TestMultiTape<SoundId> sounds();

    TestMultiTape<int> musicPauses();

    TestMultiTape<MusicId> music(); // Tracks the game asked to play, also when sound is off.

    TestMultiTape<SpeechId> speeches(); // Speeches characters were asked to play, also when voices are off.

    TestMultiTape<std::string> hudTextures();

    TestMultiTape<DrawnQuad> hudQuads(); // Like hudTextures, but with positions, and paletted images drawn by DrawImage too.

    /**
     * Return a tape object of strings listing all the hints whenever a pop up message box was called.
     *
     * @return                          Tape object.
     */
    TestMultiTape<std::string> messageBoxes();

    /**
     * Return a tape object of strings listing all the text drawn by GUIWindow::DrawText.
     * TODO(pskelton): Tape will be spammy as it is recording everything.
     * 
     * @return                          Tape object.
     */
    TestMultiTape<std::string> allGUIWindowsText();

    TestMultiTape<DrawnText> drawnTexts(); // Like allGUIWindowsText, but with the color each string was drawn in.

    TestMultiTape<DrawnLine> titleLines(); // Lines drawn by GUIWindow::DrawTitleText, where they start and their color.

    /**
     * Return a tape object of SpecialAttackTypes listing all the special attack attempts on the party.
     *
     * @return                          Tape object.
     */
    TestMultiTape<MonsterSpecialAttack> specialAttacks();

    /**
     * Return a tape object of UIMessageTypes listing all the UI messages processed by the engine.
     *
     * @return                          Tape object.
     */
    TestMultiTape<UIMessageType> uiMessages();

 private:
    TestController *_controller = nullptr;
};
