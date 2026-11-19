#include "MM6StreetNpcTalk.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Random/Random.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/IndexedArray.h"
#include "Utility/String/Ascii.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

namespace {

constexpr int PERSONALITY_COUNT = 13;

struct ProfessionTalk {
    int personality = 0;
    std::array<std::string, 7> weekdayTopics; // Monday first.
    std::array<std::string, 7> weekdayTexts;
};

struct NewsRow {
    int map = 0;
    std::string topic;
    std::string text;
};

struct StreetNpcTalkTables {
    IndexedArray<ProfessionTalk, NPC_PROFESSION_FIRST, NPC_PROFESSION_LAST> professions;
    std::vector<std::array<std::string, PERSONALITY_COUNT>> greetings; // npcBTB.txt rows from "1", by personality.
    std::vector<NewsRow> news; // npcnews.txt rows from "1".
};

/**
 * npcBTB.txt rows MM6 greets with.
 */
enum class GreetingRow {
    GREETING_FIRST = 1,
    GREETING_SECOND = 2,
    GREETING_AFTER_BEG = 3,
    GREETING_AFTER_BRIBE = 4,
    GREETING_AFTER_THREAT = 5,
    GREETING_FAME_TOO_LOW = 6,
    GREETING_NOTORIOUS_TO_GOOD = 7,
    GREETING_NOTORIOUS_TO_EVIL = 8,
    GREETING_SAINTLY_TO_GOOD = 9,
    GREETING_SAINTLY_TO_EVIL = 10,
    GREETING_BAD_FIRST = 11,
    GREETING_GOOD_TO_EVIL_FIRST = 12,
    GREETING_NOT_GOOD_ENOUGH_FIRST = 13,
    GREETING_NOT_BAD_ENOUGH_FIRST = 14,
};
using enum GreetingRow;

int personalityIndex(std::string_view name) {
    // 0x468C42, the order of the names MM6 compares against. Anything else is a thief.
    static constexpr std::array<std::string_view, 12> names = {"Adventurer", "Fanatic", "Guard", "Merchant", "Noble", "Official",
                                                               "Paladin", "Peasant", "Priest", "Scholar", "Sorcerer", "Monster"};
    for (size_t i = 0; i < names.size(); ++i)
        if (ascii::noCaseEquals(name, names[i]))
            return i == 11 ? 12 : static_cast<int>(i);
    return 11;
}

StreetNpcTalkTables loadTables() {
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
        ProfessionTalk &talk = result.professions[static_cast<NpcProfession>(id)];
        for (int weekday = 0; weekday < 7; ++weekday) {
            int column = 2 + 2 * ((weekday + 1) % 7); // The file starts the week on Sunday.
            talk.weekdayTopics[weekday] = unquote(cells[column]);
            talk.weekdayTexts[weekday] = unquote(cells[column + 1]);
        }
    }

    // 0x4C1034, personality of each npcBTB.txt column from the third one.
    static constexpr std::array<int, PERSONALITY_COUNT> btbColumnPersonality = {7, 12, 11, 3, 10, 9, 0, 8, 5, 2, 1, 6, 4};
    for (std::string_view line : split(resources->eventsData("npcBTB.txt").str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2 + PERSONALITY_COUNT> cells = split(line).by('\t');
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

const StreetNpcTalkTables &tables() {
    static StreetNpcTalkTables result = loadTables();
    return result;
}

const ProfessionTalk &professionTalk(const NPCData &npc) {
    static const ProfessionTalk none;
    if (npc.profession < NPC_PROFESSION_FIRST || npc.profession > NPC_PROFESSION_LAST)
        return none;
    return tables().professions[npc.profession];
}

std::string_view greeting(const NPCData &npc, GreetingRow row) {
    const auto &greetings = tables().greetings;
    size_t index = std::to_underlying(row) - 1;
    if (index >= greetings.size())
        return {};
    return greetings[index][professionTalk(npc).personality];
}

int npcState(const NPCData &npc) {
    return static_cast<int>(static_cast<NpcFlags::underlying_type>(npc.flags) & 0x7F);
}

bool reputationsAgree(int npcReputation, int reputation) {
    return npcReputation == 0 || (reputation > 0 && npcReputation > 0 && reputation > npcReputation) ||
           (reputation < 0 && npcReputation < 0 && reputation < npcReputation);
}

int mapNewsNumber(std::string_view mapFileName) {
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

} // namespace

mm6::StreetNpcTalkMode mm6::streetNpcTalkMode(const NPCData &npc, int fame, int reputation) {
    if ((npc.flags & NPC_HIRED) || std::to_underlying(npc.house) < 0)
        return STREET_NPC_TALK_TOPICS;
    if (fame <= npc.fame)
        return STREET_NPC_TALK_FAME_TOO_LOW;
    switch (npcState(npc)) {
    case 0:
    case 1:
        return reputationsAgree(npc.rep, reputation) ? STREET_NPC_TALK_TOPICS : STREET_NPC_TALK_REPUTATION;
    case 2:
    case 3:
        return STREET_NPC_TALK_REPUTATION;
    case 4:
        return STREET_NPC_TALK_TOPICS;
    default:
        return STREET_NPC_TALK_FAME_TOO_LOW;
    }
}

std::vector<DialogueId> mm6::streetNpcTopics(StreetNpcTalkMode mode) {
    switch (mode) {
    case STREET_NPC_TALK_TOPICS:
        return {DIALOGUE_WEEKDAY_TOPIC, DIALOGUE_13_hiring_related, DIALOGUE_NEWS};
    default:
        return {}; // TODO(_): MM6 offers Beg, Threat and Bribe to an NPC whose reputation doesn't agree with the party's.
    }
}

void mm6::pickStreetNpcNews(NPCData *npc, std::string_view mapFileName) {
    if (!npc->field_24 || npc->newsTopic != 0)
        return;

    const std::vector<NewsRow> &news = tables().news;
    auto rowsOf = [&](int map) { return static_cast<int>(std::ranges::count(news, map, &NewsRow::map)); };
    int map = mapNewsNumber(mapFileName);
    if (rowsOf(map) == 0)
        map = 1;
    int count = rowsOf(map);
    if (count == 0)
        return;

    int first = static_cast<int>(std::ranges::find(news, map, &NewsRow::map) - news.begin());
    npc->newsTopic = first + grng->random(count); // The first row of map 1 reads as no news yet, MM6 picks again next time.
}

std::string_view mm6::streetNpcWeekdayTopic(const NPCData &npc, int dayOfMonth) {
    return professionTalk(npc).weekdayTopics[dayOfMonth % 7];
}

std::string_view mm6::streetNpcWeekdayText(const NPCData &npc, int dayOfMonth) {
    return professionTalk(npc).weekdayTexts[dayOfMonth % 7];
}

std::string_view mm6::streetNpcNewsTopic(const NPCData &npc) {
    const std::vector<NewsRow> &news = tables().news;
    return npc.newsTopic >= 0 && static_cast<size_t>(npc.newsTopic) < news.size() ? std::string_view(news[npc.newsTopic].topic) : std::string_view();
}

std::string_view mm6::streetNpcNewsText(const NPCData &npc) {
    const std::vector<NewsRow> &news = tables().news;
    return npc.newsTopic >= 0 && static_cast<size_t>(npc.newsTopic) < news.size() ? std::string_view(news[npc.newsTopic].text) : std::string_view();
}

std::string_view mm6::streetNpcGreeting(const NPCData &npc, StreetNpcTalkMode mode, int reputation) {
    if (mode == STREET_NPC_TALK_FAME_TOO_LOW)
        return greeting(npc, GREETING_FAME_TOO_LOW);

    int state = npcState(npc);
    if (state == 2)
        return greeting(npc, GREETING_AFTER_BEG);
    if (state == 3)
        return greeting(npc, GREETING_AFTER_BRIBE);
    if (state == 4)
        return greeting(npc, GREETING_AFTER_THREAT);

    // Rows 11 to 14 have their second greeting four rows further down, rows 15 to 18.
    int secondOffset = state == 1 ? 4 : 0;
    int npcReputation = npc.rep;
    if (!reputationsAgree(npcReputation, reputation)) {
        if (reputation <= -1000 && npcReputation > 0)
            return greeting(npc, GREETING_NOTORIOUS_TO_GOOD);
        if (reputation >= 1000 && npcReputation < 0)
            return greeting(npc, GREETING_SAINTLY_TO_EVIL);
        GreetingRow row;
        if (npcReputation > 0) {
            row = reputation <= 0 ? GREETING_BAD_FIRST : GREETING_NOT_GOOD_ENOUGH_FIRST;
        } else {
            row = reputation >= 0 ? GREETING_GOOD_TO_EVIL_FIRST : GREETING_NOT_BAD_ENOUGH_FIRST;
        }
        return greeting(npc, static_cast<GreetingRow>(std::to_underlying(row) + secondOffset));
    }

    if (reputation <= -1000 && npcReputation < 0)
        return greeting(npc, GREETING_NOTORIOUS_TO_EVIL);
    if (reputation >= 1000 && npcReputation > 0)
        return greeting(npc, GREETING_SAINTLY_TO_GOOD);
    return greeting(npc, state == 1 ? GREETING_SECOND : GREETING_FIRST);
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
