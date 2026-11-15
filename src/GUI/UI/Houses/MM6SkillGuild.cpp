#include "MM6SkillGuild.h"

#include <algorithm>
#include <ranges>
#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"

#include "MM6Guild.h"

static constexpr int kFirstClassName = 253; // MM6's global.txt names the 18 classes from here, class.txt spells two of them differently.

GUIWindow_MM6SkillGuild::GUIWindow_MM6SkillGuild(HouseId houseId, const mm6::HousePresentationState &presentation)
    : GUIWindow_MM6House(houseId, presentation) {}

void GUIWindow_MM6SkillGuild::Update() {
    GUIWindow_MM6House::Update();
    if (!isProprietorSelected() || !pParty->hasActiveCharacter())
        return;

    const Character &character = pParty->activeCharacter();
    std::string refusal;
    if (!mm6::isGuildMember(houseId(), character)) {
        refusal = pNPCTopics[171].pText; // Text 172 of npctext.txt.
    } else if (!character.CanAct()) {
        refusal = localization->format(LSTR_S_IS_IN_NO_CONDITION_TO_S, character.name, localization->str(LSTR_DO_ANYTHING));
    }
    if (!refusal.empty()) {
        drawPanelText(refusal, (212 - panelTextHeight(refusal)) / 2 + 101, colorTable.PaleCanary);
        return;
    }
    const auto learnable = [this](int index) { return learnableSkill(index) != SKILL_INVALID; };
    if (std::ranges::any_of(std::views::iota(0, static_cast<int>(_services.size())), learnable)) {
        drawPanelText(localization->format(LSTR_SKILL_COST_LU, skillPrice()), 146, colorTable.White);
        return;
    }
    const std::string &className = localization->str(static_cast<LstrId>(kFirstClassName + std::to_underlying(character.classType)));
    const std::string nothing = fmt::format("{}\n \n{}", localization->format(LSTR_SEEK_KNOWLEDGE_ELSEWHERE_S_THE_S, character.name, className),
                                            localization->str(LSTR_I_CAN_OFFER_YOU_NOTHING_FURTHER));
    drawPanelText(nothing, (174 - panelTextHeight(nothing)) / 2 + 138, colorTable.PaleCanary);
}

void GUIWindow_MM6SkillGuild::selectService(int index) {
    if (!isProprietorSelected() || !servesActiveCharacter() || index < 0 || index >= static_cast<int>(_services.size()))
        return;
    const Skill skill = learnableSkill(index);
    if (skill == SKILL_INVALID) {
        pAudioPlayer->playUISound(SOUND_error);
        return;
    }
    const int price = skillPrice();
    if (pParty->GetGold() < price) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_MM6_SKILL_GUILD_NOT_ENOUGH_GOLD);
        return;
    }
    pParty->TakeGold(price);
    pParty->activeCharacter().pActiveSkills[skill] = CombinedSkillValue::novice();
    mm6PlayHouseSound(houseId(), HOUSE_SOUND_MM6_SKILL_GUILD_LEARN);
}

std::string GUIWindow_MM6SkillGuild::serviceLabel(int index) const {
    if (!servesActiveCharacter())
        return {};
    const Skill skill = learnableSkill(index);
    return skill == SKILL_INVALID ? std::string() : localization->skillName(skill);
}

bool GUIWindow_MM6SkillGuild::servesActiveCharacter() const {
    if (!pParty->hasActiveCharacter())
        return false;
    const Character &character = pParty->activeCharacter();
    return mm6::isGuildMember(houseId(), character) && character.CanAct();
}

Skill GUIWindow_MM6SkillGuild::learnableSkill(int index) const {
    if (!pParty->hasActiveCharacter())
        return SKILL_INVALID;
    const Skill skill = mm6::skillGuildSkills(_house.type, std::to_underlying(houseId()))[index];
    const Character &character = pParty->activeCharacter();
    return mm6::classLearnsSkill(character, skill) && !character.pActiveSkills[skill] ? skill : SKILL_INVALID;
}

int GUIWindow_MM6SkillGuild::skillPrice() const {
    const int unitPrice = _house.type == HOUSE_TYPE_MERCENARY_GUILD ? 250 : 100;
    return mm6::guildSkillPrice(_house.value.value_or(0), unitPrice, pParty->activeCharacter());
}
