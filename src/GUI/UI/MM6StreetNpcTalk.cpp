#include "MM6StreetNpcTalk.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/MM6NpcDialogue.h"
#include "GUI/UI/UIStatusBar.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/IndexedArray.h"
#include "Utility/String/Ascii.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

static constexpr int PERSONALITY_COUNT = 13;

struct StreetNpcProfessionTalk {
    int personality = 0;
    std::array<std::string, 7> weekdayTopics; // Monday first.
    std::array<std::string, 7> weekdayTexts;
};

struct StreetNpcNewsRow {
    int map = 0;
    std::string topic;
    std::string text;
};

struct StreetNpcTalkTables {
    IndexedArray<StreetNpcProfessionTalk, NPC_PROFESSION_FIRST, NPC_PROFESSION_LAST> professions;
    std::array<std::array<bool, PERSONALITY_COUNT>, 3> accepts = {}; // Beg, threat and bribe, by personality.
    std::vector<std::array<std::string, PERSONALITY_COUNT>> greetings; // npcBTB.txt rows from "1", by personality.
    std::vector<StreetNpcNewsRow> news; // npcnews.txt rows from "1".
};

/**
 * npcBTB.txt rows of what a street NPC says.
 */
enum class StreetNpcLine {
    LINE_FIRST_GREETING = 1,
    LINE_SECOND_GREETING = 2,
    LINE_AFTER_BEG = 3,
    LINE_AFTER_BRIBE = 4,
    LINE_AFTER_THREAT = 5,
    LINE_FAME_TOO_LOW = 6,
    LINE_NOTORIOUS_TO_GOOD = 7,
    LINE_NOTORIOUS_TO_EVIL = 8,
    LINE_SAINTLY_TO_GOOD = 9,
    LINE_SAINTLY_TO_EVIL = 10,
    LINE_BAD_FIRST = 11,
    LINE_GOOD_TO_EVIL_FIRST = 12,
    LINE_NOT_GOOD_ENOUGH_FIRST = 13,
    LINE_NOT_BAD_ENOUGH_FIRST = 14,
    LINE_BEG_ACCEPTED = 19,
    LINE_BEG_REFUSED = 20,
    LINE_BRIBE_ACCEPTED = 21,
    LINE_BRIBE_REFUSED = 22,
    LINE_THREAT_ACCEPTED = 23,
    LINE_THREAT_REFUSED = 24,
};
using enum StreetNpcLine;

/**
 * Low seven bits of a street NPC's flags in MM6, the NPC's attitude after the last conversation.
 */
enum class StreetNpcState {
    STREET_NPC_NOT_MET = 0,
    STREET_NPC_MET = 1,
    STREET_NPC_BEGGED = 2,
    STREET_NPC_BRIBED = 3,
    STREET_NPC_THREATENED = 4,
};
using enum StreetNpcState;

/**
 * What MM6 does when the party begs, threatens or bribes a street NPC.
 */
struct StreetNpcActionRules {
    int acceptRow = 0; // Row of `StreetNpcTalkTables::accepts`.
    StreetNpcLine acceptedLine;
    StreetNpcLine refusedLine;
    StreetNpcState state;
    int reputationCost = 0;
    SpeechId acceptedSpeech;
    SpeechId refusedSpeech;
    DialogueId topicAfter; // The action's button turns into this topic once the NPC talks.
};

static const StreetNpcActionRules &actionRules(DialogueId action) {
    static const StreetNpcActionRules beg = {0, LINE_BEG_ACCEPTED, LINE_BEG_REFUSED, STREET_NPC_BEGGED, 10, SPEECH_BEG, SPEECH_BEG_FAIL,
                                             DIALOGUE_WEEKDAY_TOPIC};
    static const StreetNpcActionRules threat = {1, LINE_THREAT_ACCEPTED, LINE_THREAT_REFUSED, STREET_NPC_THREATENED, 50, SPEECH_THREAT, SPEECH_THREAT_FAIL,
                                                DIALOGUE_13_hiring_related};
    static const StreetNpcActionRules bribe = {2, LINE_BRIBE_ACCEPTED, LINE_BRIBE_REFUSED, STREET_NPC_BRIBED, 20, SPEECH_BRIBE, SPEECH_BRIBE_FAIL,
                                               DIALOGUE_NEWS};
    if (action == DIALOGUE_BEG)
        return beg;
    if (action == DIALOGUE_THREAT)
        return threat;
    assert(action == DIALOGUE_BRIBE);
    return bribe;
}

static int personalityIndex(std::string_view name) {
    // 0x468C42, the order of the names MM6 compares against. Anything else is a thief.
    static constexpr std::array<std::string_view, 12> names = {"Adventurer", "Fanatic", "Guard", "Merchant", "Noble", "Official",
                                                               "Paladin", "Peasant", "Priest", "Scholar", "Sorcerer", "Monster"};
    for (size_t i = 0; i < names.size(); ++i)
        if (ascii::noCaseEquals(name, names[i]))
            return i == 11 ? 12 : static_cast<int>(i);
    return 11;
}

static StreetNpcTalkTables loadTables() {
    StreetNpcTalkTables result;
    ResourceManager *resources = engine->resources();

    for (std::string_view line : split(resources->eventsData("npcprof.txt").str()).by("\r\n").skip("")) {
        std::array<std::string_view, 5> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= std::to_underlying(NPC_PROFESSION_FIRST) && id <= std::to_underlying(NPC_PROFESSION_LAST))
            result.professions[static_cast<NpcProfession>(id)].personality = personalityIndex(cells[4]);
    }

    for (std::string_view line : split(resources->eventsData("proftext.txt").str()).by("\r\n").skip("")) {
        std::array<std::string_view, 16> cells = split(line).by('\t');
        int id = 0;
        if (!tryDeserialize(cells[0], &id) || id < std::to_underlying(NPC_PROFESSION_FIRST) || id > std::to_underlying(NPC_PROFESSION_LAST))
            continue;
        StreetNpcProfessionTalk &talk = result.professions[static_cast<NpcProfession>(id)];
        for (int weekday = 0; weekday < 7; ++weekday) {
            int column = 2 + 2 * ((weekday + 1) % 7); // The file starts the week on Sunday.
            talk.weekdayTopics[weekday] = unquote(cells[column]);
            talk.weekdayTexts[weekday] = unquote(cells[column + 1]);
        }
    }

    // 0x4C1034, personality of each npcBTB.txt column from the third one.
    static constexpr std::array<int, PERSONALITY_COUNT> btbColumnPersonality = {7, 12, 11, 3, 10, 9, 0, 8, 5, 2, 1, 6, 4};
    static constexpr std::array<std::string_view, 3> acceptRowNames = {"Beg", "Threat", "Bribe"};
    for (std::string_view line : split(resources->eventsData("npcBTB.txt").str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2 + PERSONALITY_COUNT> cells = split(line).by('\t');
        auto acceptRow = std::ranges::find(acceptRowNames, cells[0]);
        if (acceptRow != acceptRowNames.end()) {
            for (int column = 0; column < PERSONALITY_COUNT; ++column) {
                int accepts = 0;
                result.accepts[acceptRow - acceptRowNames.begin()][btbColumnPersonality[column]] = tryDeserialize(cells[2 + column], &accepts) && accepts != 0;
            }
            continue;
        }
        int id = 0;
        if (!tryDeserialize(cells[0], &id) || id < 1)
            continue;
        result.greetings.resize(std::max<size_t>(result.greetings.size(), id));
        for (int column = 0; column < PERSONALITY_COUNT; ++column)
            result.greetings[id - 1][btbColumnPersonality[column]] = unquote(cells[2 + column]);
    }

    for (std::string_view line : split(resources->eventsData("npcnews.txt").str()).by("\r\n").skip("")) {
        std::array<std::string_view, 4> cells = split(line).by('\t');
        int id = 0;
        int map = 0;
        if (!tryDeserialize(cells[0], &id) || id < 1 || !tryDeserialize(cells[1], &map))
            continue;
        result.news.resize(std::max<size_t>(result.news.size(), id));
        result.news[id - 1] = {map, std::string(unquote(cells[2])), std::string(unquote(cells[3]))};
    }

    return result;
}

static const StreetNpcTalkTables &tables() {
    static StreetNpcTalkTables result = loadTables();
    return result;
}

static const StreetNpcProfessionTalk &professionTalk(const NPCData &npc) {
    static const StreetNpcProfessionTalk none;
    if (npc.profession < NPC_PROFESSION_FIRST || npc.profession > NPC_PROFESSION_LAST)
        return none;
    return tables().professions[npc.profession];
}

static std::string_view line(const NPCData &npc, StreetNpcLine row) {
    const auto &greetings = tables().greetings;
    size_t index = std::to_underlying(row) - 1;
    if (index >= greetings.size())
        return {};
    return greetings[index][professionTalk(npc).personality];
}

static StreetNpcState npcState(const NPCData &npc) {
    return static_cast<StreetNpcState>(static_cast<NpcFlags::underlying_type>(npc.flags) & 0x7F);
}

static void setNpcState(NPCData *npc, StreetNpcState state) {
    npc->flags = (npc->flags & NPC_HIRED) | NpcFlags(std::to_underlying(state));
}

static bool reputationsAgree(int npcReputation, int reputation) {
    return npcReputation == 0 || (reputation > 0 && npcReputation > 0 && reputation > npcReputation) ||
           (reputation < 0 && npcReputation < 0 && reputation < npcReputation);
}

static int mapNewsNumber(std::string_view mapFileName) {
    // MM6 numbers the maps by their place in games.lod, which lists them in name order.
    std::vector<std::string> names;
    for (const MapData &map : pMapTable->pInfos)
        if (!map.fileName.empty())
            names.push_back(ascii::toLower(map.fileName));
    std::ranges::sort(names);
    auto pos = std::ranges::find(names, ascii::toLower(mapFileName));
    return pos == names.end() ? 1 : static_cast<int>(pos - names.begin()) + 1;
}

static int addressingAwardIndex = -1;

static int talkerDiplomacy() {
    return pParty->hasActiveCharacter() ? mm6::diplomacyBonus(pParty->activeCharacter(), &CheckHiredNPCSpeciality) : 0;
}

mm6::StreetNpcTalkMode mm6::streetNpcTalkMode(const NPCData &npc, int fame, int reputation) {
    if ((npc.flags & NPC_HIRED) || std::to_underlying(npc.house) < 0)
        return STREET_NPC_TALK_TOPICS;
    if (fame <= npc.fame)
        return STREET_NPC_TALK_FAME_TOO_LOW;
    switch (npcState(npc)) {
    case STREET_NPC_NOT_MET:
    case STREET_NPC_MET:
        return reputationsAgree(npc.rep, reputation) ? STREET_NPC_TALK_TOPICS : STREET_NPC_TALK_REPUTATION;
    case STREET_NPC_BEGGED:
    case STREET_NPC_BRIBED:
        return STREET_NPC_TALK_REPUTATION;
    case STREET_NPC_THREATENED:
        return STREET_NPC_TALK_TOPICS;
    default:
        return STREET_NPC_TALK_FAME_TOO_LOW;
    }
}

std::vector<DialogueId> mm6::streetNpcTopics(StreetNpcTalkMode mode) {
    switch (mode) {
    case STREET_NPC_TALK_TOPICS:
        return {DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS};
    case STREET_NPC_TALK_REPUTATION:
        return {DIALOGUE_BEG, DIALOGUE_THREAT, DIALOGUE_BRIBE};
    default:
        return {};
    }
}

void mm6::pickStreetNpcNews(NPCData *npc, std::string_view mapFileName) {
    if (!npc->field_24 || npc->newsTopic != 0)
        return;

    const std::vector<StreetNpcNewsRow> &news = tables().news;
    auto rowsOf = [&](int map) { return static_cast<int>(std::ranges::count(news, map, &StreetNpcNewsRow::map)); };
    int map = mapNewsNumber(mapFileName);
    if (rowsOf(map) == 0)
        map = 1;
    int count = rowsOf(map);
    if (count == 0)
        return;

    int first = static_cast<int>(std::ranges::find(news, map, &StreetNpcNewsRow::map) - news.begin());
    npc->newsTopic = first + grng->random(count); // The first row of map 1 reads as no news yet, MM6 picks again next time.
}

std::string_view mm6::streetNpcWeekdayTopic(const NPCData &npc, int dayOfMonth) {
    return professionTalk(npc).weekdayTopics[dayOfMonth % 7];
}

std::string_view mm6::streetNpcWeekdayText(const NPCData &npc, int dayOfMonth) {
    return professionTalk(npc).weekdayTexts[dayOfMonth % 7];
}

std::string_view mm6::streetNpcNewsTopic(const NPCData &npc) {
    const std::vector<StreetNpcNewsRow> &news = tables().news;
    return npc.newsTopic >= 0 && static_cast<size_t>(npc.newsTopic) < news.size() ? std::string_view(news[npc.newsTopic].topic) : std::string_view();
}

std::string_view mm6::streetNpcNewsText(const NPCData &npc) {
    const std::vector<StreetNpcNewsRow> &news = tables().news;
    return npc.newsTopic >= 0 && static_cast<size_t>(npc.newsTopic) < news.size() ? std::string_view(news[npc.newsTopic].text) : std::string_view();
}

std::string_view mm6::streetNpcGreeting(const NPCData &npc, StreetNpcTalkMode mode, int reputation) {
    if (mode == STREET_NPC_TALK_FAME_TOO_LOW)
        return line(npc, LINE_FAME_TOO_LOW);

    StreetNpcState state = npcState(npc);
    if (state == STREET_NPC_BEGGED)
        return line(npc, LINE_AFTER_BEG);
    if (state == STREET_NPC_BRIBED)
        return line(npc, LINE_AFTER_BRIBE);
    if (state == STREET_NPC_THREATENED)
        return line(npc, LINE_AFTER_THREAT);

    // Rows 11 to 14 have their second greeting four rows further down, rows 15 to 18.
    int secondOffset = state == STREET_NPC_MET ? 4 : 0;
    int npcReputation = npc.rep;
    if (!reputationsAgree(npcReputation, reputation)) {
        if (reputation <= -1000 && npcReputation > 0)
            return line(npc, LINE_NOTORIOUS_TO_GOOD);
        if (reputation >= 1000 && npcReputation < 0)
            return line(npc, LINE_SAINTLY_TO_EVIL);
        StreetNpcLine row;
        if (npcReputation > 0) {
            row = reputation <= 0 ? LINE_BAD_FIRST : LINE_NOT_GOOD_ENOUGH_FIRST;
        } else {
            row = reputation >= 0 ? LINE_GOOD_TO_EVIL_FIRST : LINE_NOT_BAD_ENOUGH_FIRST;
        }
        return line(npc, static_cast<StreetNpcLine>(std::to_underlying(row) + secondOffset));
    }

    if (reputation <= -1000 && npcReputation < 0)
        return line(npc, LINE_NOTORIOUS_TO_EVIL);
    if (reputation >= 1000 && npcReputation > 0)
        return line(npc, LINE_SAINTLY_TO_GOOD);
    return line(npc, state == STREET_NPC_MET ? LINE_SECOND_GREETING : LINE_FIRST_GREETING);
}

int mm6::diplomacyBonus(const Character &character, bool (*isHired)(NpcProfession)) {
    CombinedSkillValue skill = character.pActiveSkills[SKILL_DIPLOMACY];
    int bonus = skill.level();
    if (isHired(static_cast<NpcProfession>(23))) // Counselor.
        bonus += 4;
    if (isHired(static_cast<NpcProfession>(24))) // Barrister.
        bonus += 8;
    if (isHired(static_cast<NpcProfession>(49))) // Negotiator.
        bonus += 4;
    if (skill.mastery() == MASTERY_MASTER)
        return 4 * bonus;
    return (skill.mastery() == MASTERY_EXPERT ? 3 : 2) * bonus;
}

int mm6::bribePrice(int diplomacy, int bribes) {
    return std::max(10, (100 - diplomacy) * (bribes + 1) * 50 / 100);
}

int mm6::streetNpcBribePrice(const Character &talker) {
    return bribePrice(diplomacyBonus(talker, &CheckHiredNPCSpeciality), pParty->mm6Bribes);
}

bool mm6::streetNpcAccepts(const NPCData &npc, DialogueId action) {
    return tables().accepts[actionRules(action).acceptRow][professionTalk(npc).personality];
}

std::string mm6::streetNpcActionLabel(DialogueId action) {
    if (action == DIALOGUE_BEG)
        return localization->str(static_cast<LstrId>(27)); // "Beg".
    if (action == DIALOGUE_THREAT)
        return localization->str(static_cast<LstrId>(226)); // "Threat".
    assert(action == DIALOGUE_BRIBE);
    int price = bribePrice(talkerDiplomacy(), pParty->mm6Bribes);
    return fmt::format("{} {} {}", localization->str(static_cast<LstrId>(31)), price, localization->str(LSTR_GOLD)); // "Bribe".
}

std::string_view mm6::streetNpcActionText(const NPCData &npc, DialogueId action) {
    const StreetNpcActionRules &rules = actionRules(action);
    return line(npc, streetNpcAccepts(npc, action) ? rules.acceptedLine : rules.refusedLine);
}

DialogueId mm6::takeStreetNpcAction(NPCData *npc, DialogueId action, GUIWindow *window) {
    const StreetNpcActionRules &rules = actionRules(action);
    auto react = [](SpeechId speech) {
        if (pParty->hasActiveCharacter())
            pParty->activeCharacter().playReaction(speech);
    };

    if (action == DIALOGUE_BEG && npcState(*npc) == STREET_NPC_BEGGED)
        return DIALOGUE_NULL; // Begging twice gets the greeting again.

    if (!streetNpcAccepts(*npc, action)) {
        react(rules.refusedSpeech);
        return action;
    }

    int diplomacy = talkerDiplomacy();
    if (action == DIALOGUE_BRIBE) {
        int price = bribePrice(diplomacy, pParty->mm6Bribes);
        if (pParty->GetGold() < price) {
            react(SPEECH_NOT_ENOUGH_GOLD);
            engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
            return DIALOGUE_NULL;
        }
        pParty->TakeGold(price);
        pParty->mm6Bribes++;
    }

    setNpcState(npc, rules.state);
    pParty->mm6Reputation -= std::max(0, rules.reputationCost - diplomacy);
    for (GUIButton *button : window->vButtons) {
        if (button->msg != UIMSG_SelectNPCDialogueOption)
            continue;
        DialogueId topic = static_cast<DialogueId>(button->msg_param);
        if (topic == DIALOGUE_BEG || topic == DIALOGUE_THREAT || topic == DIALOGUE_BRIBE)
            button->msg_param = std::to_underlying(actionRules(topic).topicAfter);
    }
    react(rules.acceptedSpeech);
    return action;
}

void mm6::showStreetNpcJoinTopics(GUIWindow *window, const NPCData &npc) {
    std::vector<DialogueId> topics;
    if (!pNPCStats->pProfessions[npc.profession].pBenefits.empty())
        topics.push_back(DIALOGUE_PROFESSION_DETAILS);
    topics.push_back(DIALOGUE_HIRE_FIRE);

    window->DeleteButtons();
    pBtn_ExitCancel = createNpcDialogueExitButton(window);
    for (int i = 0; i < topics.size(); i++)
        createNpcDialogueOptionButton(window, i, topics[i], 160 + i * 30, 30);
    window->setKeyboardControlGroup(topics.size(), false, 0, 1);
    window->CreateCharacterButtons();
}

int mm6::addressingAwardChoice(int count) {
    assert(count > 0);
    if (addressingAwardIndex < 0)
        addressingAwardIndex = vrng->random(count);
    return addressingAwardIndex % count; // MM6 reads past the list when the character gained or lost awards since the pick.
}

void mm6::forgetAddressingAwardChoice() {
    addressingAwardIndex = -1;
}
