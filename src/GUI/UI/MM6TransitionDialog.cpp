#include "GUI/UI/MM6TransitionDialog.h"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/TransitionTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/MM6TopPanelResources.h"
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

    return {exitPictureId ? icons[exitPictureId - 1] : std::string_view{},
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

/**
 * @param locationName                  Map an MM6 event moves the party to.
 * @return                              `trans.txt` line MM6 shows for the move whatever the event's house, 0 if none.
 */
static int mm6SpecialTransitionText(std::string_view locationName) {
    if (ascii::noCaseEquals(locationName, "zddb01.blv")) // Dragon's Lair.
        return 1;
    if (ascii::noCaseEquals(locationName, "zdwj02.blv")) // Devil Outpost.
        return 2;
    return 0;
}

static int mm6TextHeight(std::string_view text, int width) {
    return text.empty() ? 0 : assets->pFontCreate->CalcTextHeight(text, width, 0) + 3; // MM6 counts every line as font height - 3, CalcTextHeight counts the first one as font height - 6.
}

void GUIWindow_IndoorEntryExit::initializeMM6(HouseId transitionHouse, unsigned exitPictureId) {
    const int houseId = std::to_underlying(transitionHouse);
    const std::string_view transitionText = houseId > 0 && houseId < pTransitionStrings.size()
        ? pTransitionStrings[houseId] : std::string_view{};
    const int specialText = mm6SpecialTransitionText(_mapName);
    bool refused = false;
    _dialogueBackground = assets->getIcon("evpan004");
    try {
        const mm6::TransitionDialogPlan plan = mm6::planIndoorTransition(
            GameTitle::MM6, transitionHouse, exitPictureId, transitionText);
        transition_ui_icon = plan.destinationIcon.empty() ? nullptr : assets->getIcon(plan.destinationIcon);
        _transitionStringId = specialText ? specialText : plan.transitionStringId;

        if (transitionHouse != HOUSE_INVALID) {
            const mm6::HouseEntryPlan entry = mm6::planHouseEntry(mm6::houseServiceData(), houseId);
            if (entry.room) {
                _dialogueBackground = assets->getIcon(fmt::format("evpan{:03}", entry.room->background));
                if (!specialText)
                    pMediaPlayer->OpenHouseMovie(entry.room->videoName, 1);
            }
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
    pBtn_ExitCancel = CreateButton("Transition_No", {566, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0,
                                   UIMSG_CancelIndoorEntryExit, 0, INPUT_ACTION_TRANSITION_NO,
                                   localization->str(LSTR_CANCEL), {ui_buttdesc2});
    if (refused)
        return;

    const bool entering = transitionHouse != HOUSE_INVALID || specialText;
    const MapId destination = mm6TransitionDestination(_mapName);
    _mm6TitleMap = entering ? destination : engine->_currentLoadedMapId;
    if (entering) {
        sHint = destination != MAP_INVALID ? localization->format(LSTR_ENTER_S, pMapTable->pInfos[destination].name) : localization->str(LSTR_ENTER);
    } else {
        sHint = engine->_currentLoadedMapId != MAP_INVALID
            ? localization->format(LSTR_LEAVE_S, pMapTable->pInfos[engine->_currentLoadedMapId].name) : localization->str(LSTR_EXIT_DIALOGUE);
    }
    pBtn_YES = CreateButton("Transition_Yes", {486, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnIndoorEntryExit, 0,
                            INPUT_ACTION_TRANSITION_YES, sHint, {ui_buttyes2});
    CreateButton({525, 34}, {62, 72}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnIndoorEntryExit, 1, INPUT_ACTION_INTERACT, sHint);
    CreateButton({8, 8}, {459, 343}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnIndoorEntryExit, 1, INPUT_ACTION_INVALID, sHint);
}

void GUIWindow_IndoorEntryExit::updateMM6() {
    render->DrawQuad2D(assets->getIcon(mm6TopPanelIconForHour(GameTitle::MM6, pParty->uCurrentHour)), {468, 0}); // Only its left edge shows beside the panel.
    render->DrawQuad2D(_dialogueBackground, {481, 0});
    if (transition_ui_icon)
        render->DrawQuad2D(transition_ui_icon, {525, 34});
    render->DrawQuad2D(assets->getIcon("buttesc1"), {566, 313});
    if (!_mm6Unsupported)
        render->DrawQuad2D(assets->getIcon("buttyes1"), {486, 313});

    const int screenHeight = render->GetRenderDimensions().h;
    if (_mm6TitleMap != MAP_INVALID)
        DrawTitleText(assets->pFontCreate.get(), 0, 2, colorTable.White, pMapTable->pInfos[_mm6TitleMap].name, 3, Recti(493, 0, 126, screenHeight));

    std::string text;
    if (_mm6Unsupported) {
        text = localization->str(LSTR_EXIT_DIALOGUE);
    } else if (_transitionStringId) {
        text = pTransitionStrings[_transitionStringId];
    } else if (_mm6TitleMap != MAP_INVALID) {
        text = localization->format(LSTR_DO_YOU_WISH_TO_LEAVE_S_2, pMapTable->pInfos[_mm6TitleMap].name);
    }
    const Recti panel(483, 0, 148, screenHeight);
    const int y = (212 - mm6TextHeight(text, panel.w)) / 2 + 101;
    DrawTitleText(assets->pFontCreate.get(), 0, y, colorTable.White, text, 3, panel);
}

void GUIWindow_Travel::initializeMM6(const std::string &leaveHint) {
    sHint = leaveHint;
    _dialogueBackground = assets->getIcon("evpan004");
    pBtn_ExitCancel = CreateButton("Transition_No", {566, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_CancelTravelByFoot, 0,
                                   INPUT_ACTION_TRANSITION_NO, localization->str(LSTR_STAY_IN_THIS_AREA), {ui_buttdesc2});
    pBtn_YES = CreateButton("Transition_Yes", {486, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnTravelByFoot, 0,
                            INPUT_ACTION_TRANSITION_YES, leaveHint, {ui_buttyes2});
    CreateButton({525, 34}, {62, 72}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnTravelByFoot, 1, INPUT_ACTION_INTERACT, leaveHint);
    CreateButton({8, 8}, {459, 343}, BUTTON_TYPE_NORMAL, 0, UIMSG_OnTravelByFoot, 1, INPUT_ACTION_INVALID, leaveHint);
}

void GUIWindow_Travel::updateMM6() {
    render->DrawQuad2D(_dialogueBackground, {481, 0});
    render->DrawQuad2D(transition_ui_icon, {525, 34});
    render->DrawQuad2D(assets->getIcon("buttesc1"), {566, 313});
    render->DrawQuad2D(assets->getIcon("buttyes1"), {486, 313});

    const MapId destination = pOutdoor->getTravelDestination(pParty->pos.x, pParty->pos.y).map();
    if (destination == MAP_INVALID)
        return;
    const std::string &destinationName = pMapTable->pInfos[destination].name;
    const int screenHeight = render->GetRenderDimensions().h;
    DrawTitleText(assets->pFontCreate.get(), 0, 2, colorTable.White, destinationName, 3, Recti(493, 0, 126, screenHeight));

    // MM6 has no one-day line, a one-day trip reads "1 days".
    const std::string text = localization->format(LSTR_IT_WILL_TAKE_D_DAYS_TO_TRAVEL_TO_S, getTravelTime(), destinationName) + "\n \n" +
                             localization->format(LSTR_DO_YOU_WISH_TO_LEAVE_S_1, pMapTable->pInfos[engine->_currentLoadedMapId].name);
    const Recti panel(483, 0, 148, screenHeight);
    const int y = (212 - mm6TextHeight(text, panel.w)) / 2 + 101;
    DrawTitleText(assets->pFontCreate.get(), 0, y, colorTable.White, text, 3, panel);
}
