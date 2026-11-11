#include "GUI/UI/MM6SaveLoadMenu.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6Finale.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6V5FirstMapLoadAttempt.h"
#include "Engine/MM6/MM6V5FirstMapNativeSlotPolicy.h"
#include "Engine/SaveLoad.h"
#include "Engine/Timer.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"

#include "Library/Logger/Logger.h"

#include "Utility/String/Ascii.h"

static void reportMM6MenuRefusal(std::string_view message) {
    MM_WARNING("MM6 save menu: {}", message);
    if (engine->_statusBar)
        engine->_statusBar->setEvent(message);
    if (pAudioPlayer)
        pAudioPlayer->playUISound(SOUND_error);
}

static void reportUnsavedOutcome(mm6::ManualSaveOutcome outcome) {
    switch (outcome) {
    case mm6::MANUAL_SAVE_COMMITTED:
        break;
    case mm6::MANUAL_SAVE_NOT_WRITTEN:
        reportMM6MenuRefusal("Save failed, the slot was not changed");
        break;
    case mm6::MANUAL_SAVE_UNCONFIRMED:
        reportMM6MenuRefusal("Save could not be confirmed, check the slot before saving again");
        break;
    case mm6::MANUAL_SAVE_REFUSED:
        reportMM6MenuRefusal(mm6::isManualSaveArmed() ? "This game can't be saved here yet" :
                                                                 "Might & Magic VI saving is not available yet");
        break;
    }
}

std::vector<SavegameSlot> mm6LoadMenuSlots() {
    std::vector<SavegameSlot> result;
    for (const mm6::SaveSlotInfo &info : mm6::listFirstMapManualSlots()) {
        SavegameSlot slot;
        slot.fileName = info.fileName;
        slot.header.name = info.header.displayName;
        if (slot.fileName == mm6::AUTOSAVE_SLOT) {
            slot.header.name = localization->str(LSTR_AUTOSAVE);
        } else if (isMM6AutoOrQuickSaveSlot(slot.fileName)) {
            slot.header.name = localization->str(LSTR_QUICKSAVE);
        }
        slot.header.locationName = info.header.mapFileName;
        slot.header.playingTime = Time::fromTicks(info.header.playingTicks);
        result.push_back(std::move(slot));
    }
    std::ranges::sort(result, [](const SavegameSlot &l, const SavegameSlot &r) {
        if (int result = ascii::noCaseCompare(l.header.name, r.header.name))
            return result < 0;
        return l.fileName < r.fileName;
    });
    return result;
}

bool isMM6AutoOrQuickSaveSlot(std::string_view fileName) {
    return mm6::isMM6SaveSlot(fileName) && !mm6::isNumberedV5FirstMapSlot(fileName);
}

void mm6SaveFromMenu(std::string_view fileName, std::string_view title) {
    const mm6::ManualSaveOutcome outcome = mm6::saveFirstMapManualSlot(fileName, title).outcome;
    if (outcome != mm6::MANUAL_SAVE_COMMITTED) {
        reportUnsavedOutcome(outcome);
        return;
    }
    GUI_UpdateWindows();
    pGUIWindow_CurrentMenu = nullptr;
    current_screen_type = SCREEN_GAME;
    gameTimer->setPaused(false);
    engine->_statusBar->setEvent(LSTR_GAME_SAVED);
}

bool mm6MayLoadFromMainMenu(std::string_view fileName) {
    if (mm6::mayLoadFirstMapManualSlot(fileName))
        return true;
    MM_WARNING("MM6 slot '{}' was not loaded, it is not a numbered slot of the MM6 profile", fileName);
    if (pAudioPlayer)
        pAudioPlayer->playUISound(SOUND_error);
    return false;
}

bool mm6LoadOverGame(std::string_view fileName) {
    try {
        if (mm6::loadSlotOverSession(fileName))
            return true;
    } catch (const std::exception &) {
        mm6::abandonMainMenuLoad();
        return false;
    }
    reportMM6MenuRefusal("This game can't be loaded");
    return false;
}

void mm6AutoSave() {
    if (mm6::isHiveFinaleUnderway())
        return;
    if (!mm6::isManualSaveArmed()) {
        MM_INFO("MM6 autosave skipped, this game can't be saved");
        return;
    }
    (void) mm6::saveSessionSlot(mm6::AUTOSAVE_SLOT, localization->str(LSTR_AUTOSAVE), mm6::V5FirstMapCaptureBoundary::SettledInput);
}

void mm6QuickSave() {
    auto &quickSaveIndex = engine->config->gameplay.QuickSavesCount;
    const int previousIndex = quickSaveIndex.value();
    quickSaveIndex.cycleIncrement();
    const mm6::ManualSaveOutcome outcome =
        mm6::saveSessionSlot(getCurrentQuickSave(), localization->str(LSTR_QUICKSAVE), mm6::V5FirstMapCaptureBoundary::SettledInput).outcome;
    if (outcome != mm6::MANUAL_SAVE_COMMITTED) {
        quickSaveIndex.setValue(previousIndex);
        reportUnsavedOutcome(outcome);
        return;
    }
    engine->_statusBar->setEvent(LSTR_GAME_SAVED);
    pAudioPlayer->playUISound(SOUND_StartMainChoice02);
}
