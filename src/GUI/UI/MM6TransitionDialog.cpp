#include "GUI/UI/MM6TransitionDialog.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/TransitionTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/UI/MM6HouseServiceData.h"
#include "GUI/UI/UIDialogue.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UITransition.h"

#include "Media/MediaPlayer.h"

#include "Utility/String/Ascii.h"

extern GraphicsImage *transition_ui_icon;

namespace mm6 {

TransitionDialogPlan planIndoorTransition(GameTitle title, HouseId houseId,
                                          unsigned exitPictureId,
                                          std::string_view transitionText) {
    // The original MM6 executable uses one-based picture IDs to select this
    // eight-entry table. The MM7 table has different indices and names.
    static constexpr std::array<std::string_view, 8> icons = {
        "castle", "dungeon", "idoor", "isecdoor",
        "istairdn", "istairup", "itrap", "outside"
    };

    if (title != GameTitle::MM6)
        throw std::invalid_argument("MM6 transition plan needs the MM6 title");
    if (exitPictureId > icons.size())
        throw std::invalid_argument("MM6 transition picture ID is out of range");
    if (houseId == HOUSE_INVALID && exitPictureId == 0)
        throw std::invalid_argument("MM6 event has no transition dialog");
    if (houseId != HOUSE_INVALID && transitionText.empty())
        throw std::invalid_argument("MM6 transition text row is missing");

    return {"backevt", exitPictureId ? icons[exitPictureId - 1] : std::string_view{},
            "buttyes2", "buttesc2",
            houseId == HOUSE_INVALID ? 0 : std::to_underlying(houseId)};
}

} // namespace mm6

namespace {

MapId mm6TransitionDestination(std::string_view locationName) {
    const std::string_view name = !locationName.empty() && locationName[0] == '0' &&
                                  engine->_currentLoadedMapId != MAP_INVALID
        ? pMapTable->pInfos[engine->_currentLoadedMapId].fileName : locationName;
    for (MapId id : pMapTable->pInfos.indices()) {
        if (ascii::noCaseEquals(pMapTable->pInfos[id].fileName, name))
            return id;
    }
    return MAP_INVALID;
}

} // namespace

void GUIWindow_IndoorEntryExit::initializeMM6(HouseId transitionHouse, unsigned exitPictureId) {
    const int houseId = std::to_underlying(transitionHouse);
    const std::string_view transitionText = houseId > 0 && houseId < pTransitionStrings.size()
        ? pTransitionStrings[houseId] : std::string_view{};
    bool refused = false;
    try {
        const mm6::TransitionDialogPlan plan = mm6::planIndoorTransition(
            GameTitle::MM6, transitionHouse, exitPictureId, transitionText);
        transition_ui_icon = plan.destinationIcon.empty() ? nullptr : assets->getIcon(plan.destinationIcon);
        _transitionStringId = plan.transitionStringId;

        if (transitionHouse != HOUSE_INVALID) {
            const mm6::HouseEntryPlan entry = mm6::planHouseEntry(mm6::houseServiceData(), houseId);
            if (entry.room)
                pMediaPlayer->OpenHouseMovie(entry.room->videoName, 1);
        }
    } catch (const std::logic_error &) {
        refused = true;
    } catch (const std::runtime_error &) {
        refused = true;
    }
    _mm6Unsupported = refused;
    if (refused) {
        // Missing or invalid local house data keeps the transition closed.
        transition_ui_icon = nullptr;
    }
    pBtn_ExitCancel = CreateButton("Transition_No", {556, 445}, {75, 33}, BUTTON_TYPE_NORMAL, 0,
                                   UIMSG_CancelIndoorEntryExit, 0, INPUT_ACTION_TRANSITION_NO,
                                   localization->str(LSTR_CANCEL), {ui_buttdesc2});
    if (refused)
        return;
    const MapId mapId = mm6TransitionDestination(_mapName);
    std::string hint = localization->str(LSTR_EXIT_DIALOGUE);
    if (transitionHouse != HOUSE_INVALID && mapId != MAP_INVALID)
        hint = localization->format(LSTR_ENTER_S, pMapTable->pInfos[mapId].name);
    else if (transitionHouse == HOUSE_INVALID && engine->_currentLoadedMapId != MAP_INVALID)
        hint = localization->format(LSTR_LEAVE_S, pMapTable->pInfos[engine->_currentLoadedMapId].name);
    pBtn_YES = CreateButton("Transition_Yes", {476, 445}, {75, 33}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnIndoorEntryExit, 0,
                            INPUT_ACTION_TRANSITION_YES, hint, {ui_buttyes2});
}

void GUIWindow_IndoorEntryExit::updateMM6() {
    render->DrawQuad2D(_dialogueBackground, {477, 0});
    if (transition_ui_icon)
        render->DrawQuad2D(transition_ui_icon, {pNPCPortraits_x[0][0], pNPCPortraits_y[0][0]});
    render->DrawQuad2D(ui_buttdesc2, {556, 445});
    if (!_mm6Unsupported)
        render->DrawQuad2D(ui_buttyes2, {476, 445});

    const MapId mapId = mm6TransitionDestination(_mapName);
    Recti transitionWindow = pPrimaryWindow->frameRect;
    transitionWindow.x = 493;
    transitionWindow.w = 126;
    if (mapId != MAP_INVALID) {
        DrawTitleText(assets->pFontCreate.get(), 0, 5, colorTable.White,
                      pMapTable->pInfos[mapId].name, 3, transitionWindow);
    }
    transitionWindow.x = SIDE_TEXT_BOX_POS_X;
    transitionWindow.w = SIDE_TEXT_BOX_WIDTH;
    const std::string text = _mm6Unsupported
        ? localization->str(LSTR_EXIT_DIALOGUE)
        : _transitionStringId
        ? pTransitionStrings[_transitionStringId]
        : mapId != MAP_INVALID
            ? localization->format(LSTR_DO_YOU_WISH_TO_LEAVE_S_2, pMapTable->pInfos[mapId].name)
            : localization->str(LSTR_EXIT_DIALOGUE);
    const int y = (212 - assets->pFontCreate->CalcTextHeight(text, transitionWindow.w, 0)) / 2 + 101;
    DrawTitleText(assets->pFontCreate.get(), 0, y, colorTable.White, text, 3, transitionWindow);
}
