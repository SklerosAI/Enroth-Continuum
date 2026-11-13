#include "MM6House.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineIocContainer.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/MM6EventVariableDispatch.h"
#include "Engine/Evt/MM6ItemEventCompare.h"
#include "Engine/Localization.h"
#include "Engine/mm7_data.h"
#include "Engine/Graphics/Camera.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6HouseExit.h"
#include "Engine/MM6/MM6RoomMovie.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/HouseTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/TransitionTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIFont.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/MM6TopPanelResources.h"
#include "GUI/UI/MM6CharacterScreen.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/Houses/MM6Guild.h"
#include "GUI/UI/Houses/MM6Shop.h"
#include "GUI/UI/Houses/MM6ShopPrices.h"
#include "GUI/UI/Houses/MM6SkillGuild.h"
#include "GUI/UI/Houses/MM6Tavern.h"
#include "GUI/UI/Houses/MM6Temple.h"
#include "GUI/UI/Houses/MM6Training.h"
#include "GUI/UI/Houses/MM6Transport.h"

#include "Io/KeyboardInputHandler.h"
#include "Io/Mouse.h"

#include "Library/Logger/Logger.h"
#include "Library/Platform/Interface/Platform.h"

#include "Media/Audio/AudioPlayer.h"
#include "Media/MediaPlayer.h"

static constexpr LstrId LSTR_MM6_CANCEL = static_cast<LstrId>(34);
static constexpr LstrId LSTR_MM6_END_CONVERSATION = static_cast<LstrId>(74);
static constexpr LstrId LSTR_MM6_EXIT_BUILDING = static_cast<LstrId>(80);
static constexpr LstrId LSTR_MM6_HOW_MUCH = static_cast<LstrId>(112);

/** Portrait positions on the right panel, by the number of people in the house. */
static constexpr std::array<std::array<Pointi, 6>, 6> kPortraitPositions = {{
    {{{525, 34}}},
    {{{525, 34}, {525, 129}}},
    {{{525, 34}, {525, 129}, {525, 224}}},
    {{{525, 34}, {486, 129}, {564, 129}, {525, 224}}},
    {{{525, 34}, {486, 129}, {564, 129}, {486, 224}, {564, 224}}},
    {{{486, 34}, {564, 34}, {486, 129}, {564, 129}, {486, 224}, {564, 224}}},
}};
static constexpr Pointi kSelectedPortrait = {525, 34};
static constexpr Pointi kExitPicture = {526, 313};
static constexpr int kPanelTextRight = 630; // Names and portrait labels are centred between their margin and here.
static constexpr int kOptionTextX = 483;
static constexpr int kOptionTextWidth = 148;

static constexpr int kFirstMembershipTopic = 381; // npctopic.txt "Elemental Guild Membership", the Dark Guild's is 397.
static constexpr std::array<int, 17> kMembershipPrices = {100, 100, 25, 50, 50, 25, 50, 50, 50, 50, 50, 50, 50, 50, 50, 1000, 1000};

/**
 * @param guild                         Guild in npctopic.txt membership order.
 * @return                              Award that makes a character a member of the guild.
 */
static AwardId membershipAward(int guild) {
    return static_cast<AwardId>(64 + guild);
}

static constexpr int kFirstTeacherTopic = 200;
static constexpr int kLastTeacherTopic = 259;

/**
 * @param topic                         Teacher topic, 200 to 259. Two topics per skill, expert then master.
 * @return                              Skill the teacher teaches.
 */
static Skill teacherSkill(int topic) {
    int index = (topic - kFirstTeacherTopic) / 2;
    if (index > 27)
        ++index; // No one teaches Thievery, MM6 skill 28.
    return index == 30 ? SKILL_LEARNING : static_cast<Skill>(index); // MM6 numbers Learning 30.
}

/**
 * @offset 0x497102 (MM6)
 *
 * @param skill                         Skill taught.
 * @return                              Gold an expert teacher asks.
 */
static int expertTeacherPrice(Skill skill) {
    switch (skill) {
      case SKILL_LEATHER: case SKILL_CHAIN: case SKILL_PLATE: case SKILL_FIRE: case SKILL_AIR:
      case SKILL_WATER: case SKILL_EARTH: case SKILL_SPIRIT: case SKILL_MIND: case SKILL_BODY:
        return 1000;
      case SKILL_ITEM_ID: case SKILL_REPAIR: case SKILL_BODYBUILDING: case SKILL_MEDITATION:
      case SKILL_PERCEPTION: case SKILL_DIPLOMACY: case SKILL_TRAP_DISARM:
        return 500;
      default:
        return 2000;
    }
}

static Pointi portraitPosition(size_t count, size_t index) {
    return kPortraitPositions[count - 1][index];
}

/**
 * @param font                          Font the text is drawn in.
 * @param text                          Text to measure.
 * @param width                         Width the text wraps to.
 * @return                              Height of the text as MM6 counts it, the font height minus 3 for every line.
 */
static int mm6TextHeight(GUIFont *font, std::string_view text, int width) {
    return text.empty() ? 0 : font->CalcTextHeight(text, width, 0) + 3;
}

static bool isResidentNpc(const NPCData &npc, HouseId houseId) {
    return npc.house == houseId && !(npc.flags & NPC_HIRED);
}

/**
 * @param hour                          Hour of the day, 0 to 23.
 * @return                              The hour on MM6's 12-hour clock, and whether it's pm. MM6 keeps 12 as 12 am.
 */
static std::pair<int, bool> amPmHour(int hour) {
    return hour > 12 ? std::pair(hour - 12, true) : std::pair(hour, false);
}

static bool hasResidentNpc(HouseId houseId) {
    for (int i = 1; i < pNPCStats->uNumNewNPCs; ++i)
        if (isResidentNpc(pNPCStats->pNPCData[i], houseId))
            return true;
    return false;
}

static bool carriesTanirsBell() {
    return std::ranges::any_of(pParty->pCharacters, [](const Character &character) {
        return mm6::hasEventItem(character, pParty->pPickedItem, 17, 461);
    });
}

/**
 * @param houseId                       House the party enters.
 * @param videoName                     Room's own movie.
 * @return                              Movie the room plays, and whether it loops.
 */
static std::pair<std::string, bool> roomMovie(HouseId houseId, const std::string &videoName) {
    if (std::optional<std::string_view> once = mm6::oneShotRoomMovie(std::to_underlying(houseId), carriesTanirsBell()))
        return {std::string(*once), false};
    return {videoName, true};
}

static std::string exitLabel(MapId map) {
    return localization->format(LSTR_ENTER_S, pMapTable->pInfos[map].name);
}

GUIWindow_MM6House::GUIWindow_MM6House(HouseId houseId, const mm6::HousePresentationState &presentation)
    : GUIWindow_House(houseId), _house(mm6::houseServiceData().at(std::to_underlying(houseId))) {
    _panel = _house.room.room ? assets->getIcon(fmt::format("evpan{:03}", _house.room.room->background)) : assets->getIcon("backevt");
    if (presentation.proprietorIconName)
        _residents.push_back({nullptr, assets->getIcon(*presentation.proprietorIconName),
                              houseTable[houseId].pProprieterName, houseTable[houseId].pProprieterTitle});
    for (int i = 1; i < pNPCStats->uNumNewNPCs; ++i) {
        NPCData &npc = pNPCStats->pNPCData[i];
        if (isResidentNpc(npc, houseId))
            _residents.push_back({&npc, assets->getIcon(fmt::format("npc{:03}", npc.portraitId)), npc.name, ""});
    }
    if (mm6::isHouseExitOffered(_house.exit, pParty->_questBits) && _residents.size() < 6) // The original lays out at most six portraits.
        _residents.push_back({nullptr, assets->getIcon(mm6::houseExitIcon(_house.exit.picture)), "", "", true});
    _services = mm6::houseServiceOptions(_house.type);

    bool loops = false;
    std::tie(_roomMovie, loops) = roomMovie(houseId, presentation.videoName);
    _roomMovieStart = platform->tickCount();
    if (!loops)
        _roomMovieEnd = _roomMovieStart + mm6::roomMovieLength(pMediaPlayer->houseMoviePayload(_roomMovie));

    CreateCharacterButtons();
    if (_residents.size() == 1) {
        selectResident(0);
    } else {
        showResidentButtons();
    }
}

void GUIWindow_MM6House::Update() {
    const int screenHeight = render->GetRenderDimensions().h;
    render->DrawQuad2D(assets->getIcon(mm6TopPanelIconForHour(GameTitle::MM6, pParty->uCurrentHour)), {468, 0}); // Only its edges show around the panel.
    render->DrawQuad2D(_panel, {481, 0});
    if (isExitSelected()) {
        pBtn_ExitCancel->label = localization->str(LSTR_MM6_CANCEL);
        drawExit();
        drawMM6CharacterScreenBorders();
        return;
    }
    pBtn_ExitCancel->label = localization->str(_selected == -1 ? LSTR_MM6_EXIT_BUILDING : LSTR_MM6_END_CONVERSATION);
    DrawTitleText(assets->pFontCreate.get(), 490, 2, colorTable.White, houseTable[houseId()].name, 3, Recti(0, 0, 622, screenHeight));

    if (_selected == -1) {
        drawResidents();
    } else {
        const Resident &resident = _residents[_selected];
        render->DrawQuad2D(resident.icon, kSelectedPortrait);
        DrawTitleText(assets->pFontCreate.get(), kOptionTextX, 108, colorTable.EasternBlue,
                      resident.npc ? NameAndTitle(resident.npc) : NameAndTitle(resident.name, resident.title), 3,
                      Recti(0, 0, kPanelTextRight, screenHeight));

        updateBankTransfer();
        const Pointi mouse = EngineIocContainer::ResolveMouse()->position();
        for (size_t i = 0; i < _optionButtons.size(); ++i)
            if (_optionButtons[i]->rect.h > 0 && _optionButtons[i]->Contains(mouse))
                _focusedOption = static_cast<int>(i);

        std::vector<std::string> labels;
        for (GUIButton *button : _optionButtons) {
            if (resident.npc && _membershipOffer) {
                labels.push_back(membershipLabel());
            } else if (resident.npc && _teacherOffer) {
                bool approved = false;
                labels.push_back(teacherLabel(&approved));
            } else if (resident.npc) {
                labels.push_back(npcDialogueOptionString(static_cast<DialogueId>(button->msg_param), resident.npc));
            } else {
                labels.push_back(serviceLabel(button->msg_param));
            }
        }
        if (!_bankTransfer)
            drawOptionLabels(labels); // MM6 keeps the bank's buttons but draws the how much prompt instead of their lines.
        if (isProprietorSelected() && _house.type == HOUSE_TYPE_BANK)
            drawPanelText(fmt::format("{}: {}", localization->str(LSTR_BALANCE), pParty->GetBankGold()), 220, colorTable.PaleCanary);
    }

    DrawDialoguePanel(current_npc_text);
    render->DrawQuad2D(assets->getIcon("buttesc1"), kExitPicture);
    drawMM6CharacterScreenBorders();
}

void GUIWindow_MM6House::selectResident(int index) {
    if (index < 0 || index >= static_cast<int>(_residents.size()))
        return;
    current_npc_text.clear();
    _membershipOffer.reset();
    _teacherOffer.reset();
    _selected = index;
    if (_residents[index].npc) {
        refreshTopics();
    } else {
        clearOptionButtons();
        if (_residents[index].exit) {
            showExitButtons();
        } else {
            showServiceButtons();
        }
    }
}

void GUIWindow_MM6House::selectService(int index) {
    if (!isProprietorSelected() || index < 0 || index >= static_cast<int>(_services.size()))
        return;
    if (_services[index] == mm6::HouseServiceOption::TempleHeal) {
        healActiveCharacter();
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::Train) {
        mm6::trainActiveCharacter(std::to_underlying(houseId()), _house.value.value_or(0), &_trainingTimePassed);
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::TempleDonate) {
        donate();
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::RentRoom) {
        rentRoom();
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::FillPacks) {
        fillPacks();
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::HaveADrink) {
        haveDrink();
        return;
    }
    if (_services[index] == mm6::HouseServiceOption::Deposit || _services[index] == mm6::HouseServiceOption::Withdraw) {
        if (!_bankTransfer) {
            _bankTransfer = _services[index];
            keyboardInputHandler->StartTextInput(TEXT_INPUT_NUMBER, 10, this);
        }
        return;
    }
    engine->_statusBar->setEvent("This service is not available yet.");
}

void GUIWindow_MM6House::refreshTopics() {
    NPCData *npc = selectedNpc();
    if (!npc)
        return;
    clearOptionButtons();
    _focusedOption = 0;
    if (_membershipOffer) {
        _optionButtons.push_back(CreateButton("HouseNpcDialogue_Option0", {480, 160}, {139, 29}, BUTTON_TYPE_NORMAL, 0, UIMSG_SelectHouseNPCDialogueOption,
                                              std::to_underlying(DIALOGUE_MAGIC_GUILD_JOIN)));
        return;
    }
    if (_teacherOffer) {
        _optionButtons.push_back(CreateButton("HouseNpcDialogue_Option0", {480, 160}, {139, 29}, BUTTON_TYPE_NORMAL, 0, UIMSG_SelectHouseNPCDialogueOption,
                                              std::to_underlying(DIALOGUE_MASTERY_TEACHER_LEARN)));
        return;
    }
    int row = 0;
    for (DialogueId topic : prepareScriptedNPCDialogueTopics(npc)) {
        if (topic < DIALOGUE_SCRIPTED_LINE_1 || topic > DIALOGUE_SCRIPTED_LINE_6)
            continue;
        _optionButtons.push_back(CreateButton(fmt::format("HouseNpcDialogue_Option{}", row), {480, 160 + 30 * row}, {139, 29},
                                              BUTTON_TYPE_NORMAL, 0, UIMSG_SelectHouseNPCDialogueOption, std::to_underlying(topic)));
        ++row;
    }
}

void GUIWindow_MM6House::offerGuildMembership(int guild) {
    assert(guild >= 0 && guild < static_cast<int>(kMembershipPrices.size()));
    _membershipOffer = guild;
    current_npc_text = pNPCTopics[137 + guild].pText; // Text 138 and on of npctext.txt.
    refreshTopics();
}

void GUIWindow_MM6House::joinGuild() {
    if (!_membershipOffer || !pParty->hasActiveCharacter())
        return;
    const Character &character = pParty->activeCharacter();
    const AwardId award = membershipAward(*_membershipOffer);
    const int price = kMembershipPrices[*_membershipOffer];
    if (!character.CanAct() || character._achievedAwardsBits[award] || pParty->GetGold() < price)
        return;
    pParty->TakeGold(price);
    for (Character &member : pParty->pCharacters)
        mm6::writeCharacterEventVariable(member, EVENT_Set, std::to_underlying(VAR_Award), std::to_underlying(award));
    selectedNpc()->dialogue_3_evt_id = 0; // MM6 clears the third topic whichever one held the offer.
    _membershipOffer.reset();
    current_npc_text.clear();
    refreshTopics();
    pParty->activeCharacter().playReaction(SPEECH_JOINED_GUILD);
}

void GUIWindow_MM6House::offerExpertTraining(int topic) {
    assert(topic >= kFirstTeacherTopic && topic <= kLastTeacherTopic && topic % 2 == 0);
    _teacherOffer = topic;
    current_npc_text = pNPCTopics[topic - 1].pText; // The text with the topic's number.
    refreshTopics();
}

void GUIWindow_MM6House::learnFromTeacher() {
    bool approved = false;
    teacherLabel(&approved);
    if (!approved)
        return;
    const Skill skill = teacherSkill(*_teacherOffer);
    Character &character = pParty->activeCharacter();
    pParty->TakeGold(expertTeacherPrice(skill));
    character.pActiveSkills[skill] = CombinedSkillValue(character.pActiveSkills[skill].level(), MASTERY_EXPERT);
    character.playReaction(SPEECH_SKILL_MASTERY_INC);
    _teacherOffer.reset();
    current_npc_text.clear();
    refreshTopics();
}

NPCData *GUIWindow_MM6House::selectedNpc() const {
    return _selected == -1 ? nullptr : _residents[_selected].npc;
}

bool GUIWindow_MM6House::goBack() {
    current_npc_text.clear();
    if (_membershipOffer || _teacherOffer) {
        _membershipOffer.reset();
        _teacherOffer.reset();
        refreshTopics();
        return true;
    }
    if (_selected == -1 || _residents.size() < 2)
        return false;
    _selected = -1;
    clearOptionButtons();
    showResidentButtons();
    return true;
}

void GUIWindow_MM6House::confirmExit() {
    if (!isExitSelected() || uGameState == GAME_STATE_CHANGE_LOCATION)
        return;
    startMapTransition(mm6::houseExitDestination(_house.exit));
    pAudioPlayer->playUISound(SOUND_StartMainChoice02); // After the transition, which stops every sound.
    engine->_messageQueue->addMessageCurrentFrame(UIMSG_Escape, 0, 0);
}

void GUIWindow_MM6House::stopRoomMovieLoop() {
    if (_roomMovieEnd)
        return;
    const int64_t length = mm6::roomMovieLength(pMediaPlayer->houseMoviePayload(_roomMovie));
    const int64_t played = platform->tickCount() - _roomMovieStart;
    _roomMovieEnd = _roomMovieStart + (length ? (played / length + 1) * length : 0);
}

std::optional<HouseId> GUIWindow_MM6House::roomAfterMovie() const {
    if (!_roomMovieEnd || platform->tickCount() < *_roomMovieEnd)
        return std::nullopt;
    if (std::optional<int> next = mm6::roomAfterMovie(std::to_underlying(houseId())))
        return static_cast<HouseId>(*next);
    return std::nullopt;
}

bool GUIWindow_MM6House::isProprietorSelected() const {
    return _selected != -1 && !_residents[_selected].npc && !_residents[_selected].exit;
}

void GUIWindow_MM6House::clearOptionButtons() {
    for (GUIButton *button : _optionButtons)
        button->Release();
    _optionButtons.clear();
}

bool GUIWindow_MM6House::isExitSelected() const {
    return _selected != -1 && _residents[_selected].exit;
}

Color GUIWindow_MM6House::optionFocusColor() const {
    if (!isProprietorSelected() || _house.family == mm6::HouseServiceFamily::Shop || _house.type == HOUSE_TYPE_TRAINING_GROUND)
        return colorTable.Sunflower;
    return colorTable.PaleCanary;
}

std::string GUIWindow_MM6House::teacherLabel(bool *approved) const {
    *approved = false;
    if (!pParty->hasActiveCharacter())
        return {};
    const Character &character = pParty->activeCharacter();
    const Skill skill = teacherSkill(*_teacherOffer);
    const CombinedSkillValue value = character.pActiveSkills[skill];
    if (!character.CanAct())
        return pNPCTopics[263].pText; // "Not in your condition!"
    if (!value.level())
        return pNPCTopics[261].pText; // "You must know the skill before you can become an expert in it!"
    if (value.mastery() >= MASTERY_EXPERT)
        return pNPCTopics[264].pText; // "You are already an expert in this skill."
    if (value.level() < 4)
        return pNPCTopics[260].pText; // "You don't meet the requirements, and cannot be taught until you do."
    const int price = expertTeacherPrice(skill);
    if (price > pParty->GetGold())
        return pNPCTopics[259].pText; // "You don't have enough gold!"
    *approved = true;
    return localization->format(LSTR_BECOME_S_IN_S_FOR_LU_GOLD, localization->str(LSTR_EXPERT), localization->skillName(skill), price);
}

std::string GUIWindow_MM6House::membershipLabel() const {
    if (!pParty->hasActiveCharacter())
        return {};
    const Character &character = pParty->activeCharacter();
    if (!character.CanAct())
        return pNPCTopics[263].pText; // "Not in your condition!"
    if (character._achievedAwardsBits[membershipAward(*_membershipOffer)])
        return pNPCTopics[136].pText; // "You're already a member of this guild."
    if (pParty->GetGold() < kMembershipPrices[*_membershipOffer])
        return pNPCTopics[259].pText; // "You don't have enough gold!"
    return pNPCTopics[154 + *_membershipOffer].pText; // "Join the Elemental Guild for 100 gold" and on.
}

void GUIWindow_MM6House::showResidentButtons() {
    for (size_t i = 0; i < _residents.size(); ++i) {
        const std::string label = _residents[i].exit ? exitLabel(static_cast<MapId>(_house.exit.map))
                                                     : localization->format(LSTR_CONVERSE_WITH_S, _residents[i].name);
        _optionButtons.push_back(CreateButton(fmt::format("House_Npc{}", i), portraitPosition(_residents.size(), i), {62, 72},
                                              BUTTON_TYPE_NORMAL, 0, UIMSG_ClickHouseNPCPortrait, i, INPUT_ACTION_INVALID, label));
    }
}

void GUIWindow_MM6House::showExitButtons() {
    const std::string label = exitLabel(static_cast<MapId>(_house.exit.map));
    _optionButtons.push_back(CreateButton("HouseExit_Cancel", {566, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_Escape, 0,
                                          INPUT_ACTION_TRANSITION_NO, localization->str(LSTR_MM6_CANCEL), {assets->getIcon("buttesc2")}));
    _optionButtons.push_back(CreateButton("HouseExit_Yes", {486, 313}, {74, 32}, BUTTON_TYPE_NORMAL, 0, UIMSG_HouseTransitionConfirmation, 1,
                                          INPUT_ACTION_TRANSITION_YES, label, {assets->getIcon("buttyes2")}));
    _optionButtons.push_back(CreateButton("HouseExit_Portrait", kSelectedPortrait, {62, 72}, BUTTON_TYPE_NORMAL, 0,
                                          UIMSG_HouseTransitionConfirmation, 1, INPUT_ACTION_INTERACT, label));
    _optionButtons.push_back(CreateButton("HouseExit_Room", {8, 8}, {459, 343}, BUTTON_TYPE_NORMAL, 0, UIMSG_HouseTransitionConfirmation, 1,
                                          INPUT_ACTION_TRANSITION_YES, label));
}

void GUIWindow_MM6House::drawResidents() {
    const int screenHeight = render->GetRenderDimensions().h;
    for (size_t i = 0; i < _residents.size(); ++i) {
        const Resident &resident = _residents[i];
        render->DrawQuad2D(resident.icon, portraitPosition(_residents.size(), i));
        if (_residents.size() >= 4)
            continue;
        std::string label = resident.npc ? resident.npc->name : resident.title;
        if (resident.exit)
            label = pMapTable->pInfos[static_cast<MapId>(_house.exit.map)].name;
        DrawTitleText(assets->pFontCreate.get(), kOptionTextX, 108 + 94 * static_cast<int>(i), colorTable.EasternBlue, label, 3,
                      Recti(0, 0, kPanelTextRight, screenHeight));
    }
}

void GUIWindow_MM6House::drawExit() {
    const MapId map = static_cast<MapId>(_house.exit.map);
    const std::string &mapName = pMapTable->pInfos[map].name;
    render->DrawQuad2D(_residents[_selected].icon, kSelectedPortrait);
    render->DrawQuad2D(assets->getIcon("buttesc1"), {566, 313});
    render->DrawQuad2D(assets->getIcon("buttyes1"), {486, 313});
    DrawTitleText(assets->pFontCreate.get(), 0, 2, colorTable.White, mapName, 3, Recti(493, 0, 126, render->GetRenderDimensions().h));

    const Recti sidePanel(SIDE_TEXT_BOX_POS_X, 0, SIDE_TEXT_BOX_WIDTH, render->GetRenderDimensions().h);
    const std::string &transition = pTransitionStrings[std::to_underlying(map)]; // The original looks trans.txt up by map, not by house.
    const std::string text = transition.empty() ? exitLabel(map) : transition;
    const int y = (212 - assets->pFontCreate->CalcTextHeight(text, sidePanel.w, 0)) / 2 + 101;
    DrawTitleText(assets->pFontCreate.get(), 0, y, colorTable.White, text, 3, sidePanel);
}

void GUIWindow_MM6House::showServiceButtons() {
    _focusedOption = 0;
    for (size_t i = 0; i < _services.size(); ++i)
        _optionButtons.push_back(CreateButton(fmt::format("HouseDialogue_Option{}", i), {480, 146 + 30 * static_cast<int>(i)}, {139, 29},
                                              BUTTON_TYPE_NORMAL, 0, UIMSG_SelectProprietorDialogueOption, i));
}

void GUIWindow_MM6House::updateBankTransfer() {
    if (!_bankTransfer)
        return;
    const bool deposit = *_bankTransfer == mm6::HouseServiceOption::Deposit;
    if (keyboard_input_status == WINDOW_INPUT_IN_PROGRESS) {
        const std::string &typed = keyboardInputHandler->GetTextInput();
        drawPanelText(fmt::format("{}\n{}", localization->str(mm6::houseServiceOptionLabel(*_bankTransfer)), localization->str(LSTR_MM6_HOW_MUCH)), 146, colorTable.PaleCanary);
        drawPanelText(typed, 186, colorTable.White);
        DrawFlashingInputCursor(assets->pFontArrus->GetLineWidth(typed) / 2 + 80, 185, assets->pFontArrus.get(),
                                Recti(kOptionTextX, 0, kOptionTextWidth, render->GetRenderDimensions().h));
        return;
    }
    if (keyboard_input_status == WINDOW_INPUT_CONFIRMED) {
        int amount = std::strtol(keyboardInputHandler->GetTextInput().c_str(), nullptr, 10);
        const int available = deposit ? pParty->GetGold() : pParty->GetBankGold();
        if (amount > available) {
            mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
            amount = available;
        }
        if (amount > 0 && deposit) {
            pParty->TakeGold(amount);
            pParty->AddBankGold(amount);
            if (pParty->hasActiveCharacter())
                pParty->activeCharacter().playReaction(SPEECH_BANK_DEPOSIT);
        } else if (amount > 0) {
            pParty->TakeBankGold(amount);
            pParty->AddGold(amount);
        }
    }
    keyboard_input_status = WINDOW_INPUT_NONE;
    _bankTransfer.reset();
}

void GUIWindow_MM6House::healActiveCharacter() {
    if (!pParty->hasActiveCharacter())
        return;
    Character &character = pParty->activeCharacter();
    const int house = std::to_underlying(houseId());
    if (!mm6::templeOffersHeal(house, character))
        return;
    const int price = mm6::templeHealPrice(character, _house.value.value_or(0));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    mm6::templeHeal(house, &character);
    pAudioPlayer->playExclusiveSound(SOUND_heal);
    character.playReaction(SPEECH_TEMPLE_HEAL);
}

void GUIWindow_MM6House::donate() {
    if (!pParty->hasActiveCharacter())
        return;
    const int price = _house.value.value_or(0);
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_GENERAL_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    if (pParty->mm6Reputation < 200)
        pParty->mm6Reputation = std::min(pParty->mm6Reputation + 200, 200);
    // TODO(_): with an effective reputation above 200, the donation whose count this visit matches the day of the
    // month mod 7 casts spells at master rank day mod 7 + 1: Guardian Angel above 200, Wizard Eye above 400, Day of the
    // Gods above 600, Hour of Power above 800, Day of Protection above 1000. MM6 temple casts aren't verified yet.
    if (houseId() == static_cast<HouseId>(78) && pParty->mm6Reputation > -800)
        pParty->mm6Reputation -= 200; // Temple Baa.
    pParty->activeCharacter().playReaction(SPEECH_TEMPLE_DONATE);
    engine->_statusBar->setEvent(LSTR_THANK_YOU);
}

void GUIWindow_MM6House::rentRoom() {
    if (!pParty->hasActiveCharacter())
        return;
    const int price = mm6::tavernRoomPrice(_house.value.value_or(0), mm6::merchantPercent(pParty->activeCharacter()));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_RENT_ROOM);
    pMediaPlayer->Unload();
    engine->_messageQueue->addMessageCurrentFrame(UIMSG_RentRoom, std::to_underlying(houseId()), 1);
    window_SpeakInHouse = nullptr; // Destroys this window.
}

void GUIWindow_MM6House::fillPacks() {
    if (!pParty->hasActiveCharacter())
        return;
    const float value = _house.value.value_or(0);
    if (pParty->GetFood() >= value) {
        engine->_statusBar->setEvent(LSTR_YOUR_PACKS_ARE_ALREADY_FULL);
        pParty->activeCharacter().playReaction(SPEECH_PACKS_FULL);
        return;
    }
    const int price = mm6::tavernFoodPrice(value, mm6::merchantPercent(pParty->activeCharacter()));
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    pParty->SetFood(static_cast<int>(value));
    mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_BUY_FOOD);
}

void GUIWindow_MM6House::haveDrink() {
    if (!pParty->hasActiveCharacter())
        return;
    if (pParty->GetGold() < 1) {
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_NOT_ENOUGH_GOLD);
        return;
    }
    Character &character = pParty->activeCharacter();
    pParty->TakeGold(1);
    engine->_statusBar->setEvent(LSTR_HIC);
    mm6PlayHouseSound(houseId(), HOUSE_SOUND_TAVERN_BUY_FOOD);
    if (grng->random(2) == 1) {
        character.playReaction(SPEECH_TAVERN_GOT_DRUNK);
        if (grng->random(3) == 1)
            character.conditions.set(CONDITION_DRUNK, pParty->GetPlayingTime());
        return;
    }
    if (grng->random(4) == 1) {
        const Attribute stat = static_cast<Attribute>(grng->random(7)); // Might to Luck.
        character._statBonuses[stat] += grng->random(6) + 5;
    }
    character.playReaction(SPEECH_TAVERN_DRINK);
}

std::string GUIWindow_MM6House::serviceLabel(int index) const {
    const mm6::HouseServiceOption option = _services[index];
    if (option == mm6::HouseServiceOption::Train)
        return pParty->hasActiveCharacter() ? mm6::trainingLabel(std::to_underlying(houseId()), pParty->activeCharacter(), _house.value.value_or(0)) : std::string();
    if (option == mm6::HouseServiceOption::RentRoom || option == mm6::HouseServiceOption::FillPacks) {
        if (!pParty->hasActiveCharacter())
            return {};
        const float value = _house.value.value_or(0);
        const int merchant = mm6::merchantPercent(pParty->activeCharacter());
        if (option == mm6::HouseServiceOption::RentRoom)
            return localization->format(mm6::houseServiceOptionLabel(option), mm6::tavernRoomPrice(value, merchant));
        return localization->format(mm6::houseServiceOptionLabel(option), static_cast<int>(value), mm6::tavernFoodPrice(value, merchant));
    }
    if (option != mm6::HouseServiceOption::TempleHeal)
        return localization->str(mm6::houseServiceOptionLabel(option));
    if (!pParty->hasActiveCharacter() || !mm6::templeOffersHeal(std::to_underlying(houseId()), pParty->activeCharacter()))
        return {};
    const int price = mm6::templeHealPrice(pParty->activeCharacter(), _house.value.value_or(0));
    return fmt::format("{} {} {}", localization->str(mm6::houseServiceOptionLabel(option)), price, localization->str(LSTR_GOLD));
}

int GUIWindow_MM6House::stackedMenuTop() const {
    return 146;
}

int GUIWindow_MM6House::panelTextHeight(std::string_view text) {
    return mm6TextHeight(assets->pFontArrus.get(), text, kOptionTextWidth);
}

void GUIWindow_MM6House::drawPanelText(std::string_view text, int y, Color color) {
    DrawTitleText(assets->pFontArrus.get(), 0, y, color, text, 3, Recti(kOptionTextX, 0, kOptionTextWidth, render->GetRenderDimensions().h));
}

void GUIWindow_MM6House::drawOptionLabels(const std::vector<std::string> &labels) {
    GUIFont *font = assets->pFontArrus.get();
    std::vector<int> heights;
    int total = 0;
    int shown = 0;
    for (const std::string &label : labels) {
        heights.push_back(panelTextHeight(label));
        total += heights.back();
        shown += !label.empty();
    }

    const bool proprietor = isProprietorSelected();
    const bool stacked = proprietor && (_house.type == HOUSE_TYPE_TAVERN || _house.family == mm6::HouseServiceFamily::Transport);
    const bool fixed = proprietor && _house.type == HOUSE_TYPE_BANK; // The bank leaves its lines where it created their buttons.
    int spacing = 0;
    int y = 0;
    if (stacked) {
        y = stackedMenuTop();
    } else if (proprietor && _house.type == HOUSE_TYPE_TRAINING_GROUND && shown == 1) {
        y = (212 - total) / 2 + 101;
    } else if (shown > 0) {
        const bool skillCostLine = _house.family == mm6::HouseServiceFamily::MagicGuild || _house.type == HOUSE_TYPE_MERCENARY_GUILD ||
                                   _house.type == HOUSE_TYPE_SHADOW_GUILD;
        const bool guild = proprietor && skillCostLine; // Leaves room for the skill cost line.
        const int area = guild ? 149 : 174;
        int base = guild ? 162 : 138;
        if (proprietor && _house.type == HOUSE_TYPE_GENERAL_STORE)
            base = 130;
        spacing = std::min((area - total) / shown, 32);
        y = (area - shown * spacing - total) / 2 - spacing / 2 + base;
    }

    for (size_t i = 0; i < labels.size(); ++i) {
        GUIButton *button = _optionButtons[i];
        if (labels[i].empty()) {
            button->rect.h = 0;
            continue;
        }
        button->label = labels[i];
        const Color color = static_cast<int>(i) == _focusedOption ? optionFocusColor() : colorTable.White;
        if (fixed) {
            drawPanelText(labels[i], button->rect.y, color);
            continue;
        }
        if (stacked) {
            button->rect.y = y;
            y += heights[i] + font->GetHeight() - 3;
        } else {
            y += spacing;
            button->rect.y = y;
            y += heights[i] - 1;
        }
        button->rect.h = heights[i];
        drawPanelText(labels[i], button->rect.y, color);
    }
}

bool mm6EnterHouse(HouseId houseId) {
    const int id = std::to_underlying(houseId);
    const mm6::HouseServiceData *houses = nullptr;
    try {
        houses = &mm6::houseServiceData();
    } catch (const std::exception &e) {
        MM_WARNING("MM6 house {} refused: {}", id, e.what());
        engine->_statusBar->setEvent("This place is unavailable.");
        return false;
    }
    const mm6::HouseEntryPlan entry = mm6::planHouseEntry(*houses, id);
    if (entry.kind != mm6::HouseEntryKind::TableBacked) {
        if (entry.kind == mm6::HouseEntryKind::Terminal) {
            engine->_statusBar->setEvent("This passage is unavailable.");
        } else {
            engine->_statusBar->setEvent("This place is unavailable.");
        }
        return false;
    }
    if (!mm6::isHouseOpen(entry.openHour, entry.closeHour, pParty->GetPlayingTime().toCivilTime().hour)) {
        const auto [open, openPm] = amPmHour(*entry.openHour);
        const auto [close, closePm] = amPmHour(*entry.closeHour);
        engine->_statusBar->setEvent(LSTR_THIS_PLACE_IS_OPEN_FROM_DS_TO_DS, open, localization->amPm(openPm), close, localization->amPm(closePm));
        if (pParty->hasActiveCharacter())
            pParty->activeCharacter().playReaction(SPEECH_STORE_CLOSED);
        return false;
    }
    const mm6::HousePresentationState presentation = mm6::planHousePresentation(GameTitle::MM6, *houses, id);
    const auto [movie, loops] = roomMovie(houseId, presentation.videoName);
    const bool iconMissing = entry.proprietorIconName && engine->resources()->iconSize(*entry.proprietorIconName).isEmpty();
    if (iconMissing || !pMediaPlayer->hasHouseMoviePayload(movie)) {
        MM_WARNING("MM6 house {} refused: its room media is unavailable", id);
        engine->_statusBar->setEvent("This place is unavailable.");
        return false;
    }
    if (pMediaPlayer->IsMoviePlaying())
        return false;

    engine->_statusBar->clearAll();
    engine->_messageQueue->clear();
    current_npc_text.clear();
    houseNpcs.clear();
    currentHouseNpc = -1;
    uCurrentHouse_Animation = 0; // The MM7 jail sentinel in event dispatch must not remap this house.
    const bool showsExit = mm6::isHouseExitOffered(houses->at(id).exit, pParty->_questBits);
    if (!pMediaPlayer->tryOpenHouseMovie(movie, loops) && !presentation.proprietorIconName && !hasResidentNpc(houseId) && !showsExit) {
        engine->_statusBar->setEvent("This place is unavailable.");
        return false;
    }
    return true;
}

void mm6CreateHouseUI(HouseId houseId) {
    const mm6::HousePresentationState presentation =
        mm6::planHousePresentation(GameTitle::MM6, mm6::houseServiceData(), std::to_underlying(houseId));
    const mm6::HouseServiceRow &house = mm6::houseServiceData().at(std::to_underlying(houseId));
    const mm6::HouseServiceFamily family = house.family;
    if (family == mm6::HouseServiceFamily::Shop) {
        window_SpeakInHouse = std::make_unique<GUIWindow_MM6Shop>(houseId, presentation);
    } else if (family == mm6::HouseServiceFamily::MagicGuild) {
        window_SpeakInHouse = std::make_unique<GUIWindow_MM6Guild>(houseId, presentation);
    } else if (family == mm6::HouseServiceFamily::Transport) {
        window_SpeakInHouse = std::make_unique<GUIWindow_MM6Transport>(houseId, presentation);
    } else if (house.type == HOUSE_TYPE_MERCENARY_GUILD || house.type == HOUSE_TYPE_SHADOW_GUILD) {
        window_SpeakInHouse = std::make_unique<GUIWindow_MM6SkillGuild>(houseId, presentation);
    } else {
        window_SpeakInHouse = std::make_unique<GUIWindow_MM6House>(houseId, presentation);
    }
}

void mm6SelectHouseTopic(DialogueId topic) {
    GUIWindow_MM6House *house = mm6HouseWindow();
    NPCData *npc = house->selectedNpc();
    if (npc && topic == DIALOGUE_MAGIC_GUILD_JOIN) {
        house->joinGuild();
        return;
    }
    if (npc && topic == DIALOGUE_MASTERY_TEACHER_LEARN) {
        house->learnFromTeacher();
        return;
    }
    if (!npc || topic < DIALOGUE_SCRIPTED_LINE_1 || topic > DIALOGUE_SCRIPTED_LINE_6)
        return;
    const std::array<unsigned, 6> events = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                                            npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    const int event = events[std::to_underlying(topic) - std::to_underlying(DIALOGUE_SCRIPTED_LINE_1)];
    if (event >= kFirstMembershipTopic && event < kFirstMembershipTopic + static_cast<int>(kMembershipPrices.size())) { // MM6.exe answers these topics itself.
        house->offerGuildMembership(event - kFirstMembershipTopic);
        return;
    }
    if (event >= kFirstTeacherTopic && event <= kLastTeacherTopic && event % 2 == 0) {
        house->offerExpertTraining(event);
        return;
    }
    handleScriptedNPCTopicSelection(topic, npc);
    if (GUIWindow_MM6House *house = mm6HouseWindow())
        house->refreshTopics(); // The event may have closed the house.
}

GUIWindow_MM6House *mm6HouseWindow() {
    if (engine->gameTitle() != GameTitle::MM6)
        return nullptr;
    return dynamic_cast<GUIWindow_MM6House *>(window_SpeakInHouse.get());
}

void mm6UpdateHouseRoom() {
    GUIWindow_MM6House *house = mm6HouseWindow();
    if (!house || uGameState == GAME_STATE_CHANGE_LOCATION)
        return;
    const std::optional<HouseId> next = house->roomAfterMovie();
    if (!next)
        return;

    const int yaw = pParty->_viewYaw; // Closing a house turns the party around with flip_on_exit, and it stays inside.
    pMediaPlayer->Unload();
    if (!mm6EnterHouse(*next)) {
        window_SpeakInHouse = nullptr;
        current_screen_type = SCREEN_GAME;
        return;
    }
    mm6CreateHouseUI(*next);
    pParty->_viewYaw = yaw;
    pCamera3D->_viewYaw = yaw;
}

void mm6OnNpcMoved() {
    GUIWindow_MM6House *house = mm6HouseWindow();
    if (house && mm6::npcMoveStopsRoomLoop(std::to_underlying(house->houseId())))
        house->stopRoomMovieLoop();
}

void mm6PlayHouseSound(HouseId houseId, HouseSoundType type) {
    const mm6::HouseServiceRow &house = mm6::houseServiceData().at(std::to_underlying(houseId));
    if (house.room.room && house.room.room->soundGroup)
        pAudioPlayer->playHouseSound(static_cast<SoundId>(30000 + 100 * house.room.room->soundGroup + std::to_underlying(type)), true);
}
