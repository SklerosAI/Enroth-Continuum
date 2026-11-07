#include "NPCTable.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Engine/MapEnumFunctions.h"
#include "Engine/Engine.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/Objects/NPCEnumFunctions.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Random/Random.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/Exception.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

std::array<NPCTopic, 789> pNPCTopics;
NPCStats *pNPCStats = nullptr;

int NPCStats::dword_AE336C_LastMispronouncedNameFirstLetter = -1;
int NPCStats::dword_AE3370_LastMispronouncedNameResult = -1;

//----- (00476977) --------------------------------------------------------
void NPCStats::InitializeNPCText(const Blob &npcText) {
    // npctext.txt table structure: index | text (localized) | dev notes | npc name (localized, not used).
    for (std::string_view line : split(npcText.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        int i = fromString<int>(tokens[0]) - 1; // File indices are 1-based, array is 0-based.
        pNPCTopics[i].pText = unquote(tokens[1]);
    }
}

void NPCStats::InitializeNPCTopics(const Blob &npcTopics) {
    // npctopic.txt table structure: index | topic (localized) | ??? (not used) | dev notes | text index (not used) |
    //                               npc name (not localized, not used) | npc index (not used).
    for (std::string_view line : split(npcTopics.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        int i = fromString<int>(tokens[0]);
        pNPCTopics[i].pTopic = unquote(tokens[1]);
    }
}

void NPCStats::InitializeNPCDist(const Blob &npcDist) {
    // npcdist.txt table structure: profession (localized, not used) | area profession chance values...
    for (auto [line, prof] : split(npcDist.str()).by("\r\n").drop(2).skip("").zip(allNpcProfessions()))
        for (auto [token, map] : split(line).by('\t').drop(1).zip(allMaps()))
            pProfessionChance[map].chanceByProfession[prof] = fromString<int>(token);

    for (MapId map : allMaps())
        for (NpcProfession prof : allNpcProfessions())
            pProfessionChance[map].total += pProfessionChance[map].chanceByProfession[prof];
}

NPCProfessionChance parseMm6ProfessionChance(const Blob &npcProfs) {
    NPCProfessionChance result;
    std::array<bool, std::to_underlying(NPC_PROFESSION_MM6_LAST)> seen{};
    for (std::string_view line : split(npcProfs.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 3> cells = split(line).by('\t');
        int id = 0;
        if (!tryDeserialize(cells[0], &id))
            continue; // Headers and blank rows.
        if (id < 1 || id > std::to_underlying(NPC_PROFESSION_MM6_LAST))
            throw Exception("MM6 npcprof.txt contains an invalid profession id");
        if (id == std::to_underlying(NPC_PROFESSION_MM6_LAST))
            continue; // The terminal row has no numeric random chance.
        if (seen[id])
            throw Exception("MM6 npcprof.txt contains a duplicate profession id");
        seen[id] = true;

        int weight = 0;
        if (!tryDeserialize(cells[2], &weight) || weight < 0 ||
            weight > std::numeric_limits<int>::max() - result.total)
            throw Exception("MM6 npcprof.txt contains an invalid random chance");
        result.chanceByProfession[static_cast<NpcProfession>(id)] = weight;
        result.total += weight;
    }
    if (result.total <= 0)
        throw Exception("MM6 npcprof.txt has no positive random profession chance");
    return result;
}

NpcProfession selectAdditionalNpcProfession(const NPCProfessionChance &chance, GameTitle title, int draw) {
    if (draw < 0 || draw >= chance.total)
        throw Exception("Additional NPC profession draw is outside the loaded distribution");
    int cumulative = 0;
    auto professions = title == GameTitle::MM6
        ? allMm6RandomNpcProfessions()
        : Segment<NpcProfession>{NPC_PROFESSION_FIRST, NPC_PROFESSION_HUNTER};
    for (NpcProfession profession : professions) {
        cumulative += chance.chanceByProfession[profession];
        if (cumulative > draw)
            return profession;
    }
    throw Exception("Additional NPC profession distribution has no matching row");
}

//----- (00476CB5) --------------------------------------------------------
void NPCStats::InitializeNPCData(const Blob &npcData) {
    // npcdata.txt table structure: index | name (localized) | portrait id | groups (not used) | house | profession |
    //                              greeting index | can join (y/n) | event ids 1-6 | dev notes |
    //                              map id (optional, not used).
    for (std::string_view line : split(npcData.str()).by("\r\n").drop(2).skip("").take(500)) {
        std::array<std::string_view, 16> tokens = split(line).by('\t');
        int i = fromString<int>(tokens[0]); // File indices are 1-based.
        pNPCUnicNames[i] = unquote(tokens[1]);
        pOriginalNPCData[i].name = pNPCUnicNames[i];
        pOriginalNPCData[i].portraitId = fromString<int>(tokens[2]);
        pOriginalNPCData[i].house = static_cast<HouseId>(fromString<int>(tokens[6]));
        pOriginalNPCData[i].profession = static_cast<NpcProfession>(fromString<int>(tokens[7]));
        pOriginalNPCData[i].greetingIndex = fromString<int>(tokens[8]);
        pOriginalNPCData[i].canJoin = tokens[9][0] == 'y' ? 1 : 0;
        pOriginalNPCData[i].dialogue_1_evt_id = fromString<int>(tokens[10]);
        pOriginalNPCData[i].dialogue_2_evt_id = fromString<int>(tokens[11]);
        pOriginalNPCData[i].dialogue_3_evt_id = fromString<int>(tokens[12]);
        pOriginalNPCData[i].dialogue_4_evt_id = fromString<int>(tokens[13]);
        pOriginalNPCData[i].dialogue_5_evt_id = fromString<int>(tokens[14]);
        pOriginalNPCData[i].dialogue_6_evt_id = fromString<int>(tokens[15]);
    }
    uNumNewNPCs = 501;
}

void NPCStats::InitializeNPCGreets(const Blob &npcGreets) {
    // npcgreet.txt table structure: index | greeting 1 (localized) | greeting 2 (localized) |
    //                               notes (not localized, not used) | owner (not localized, not used).
    for (std::string_view line : split(npcGreets.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 3> tokens = split(line).by('\t');
        if (tokens[0].empty())
            continue; // Trailing orphan row with no index column.

        int i = fromString<int>(tokens[0]); // File indices are 1-based.
        pNPCGreetings[i].pGreeting1 = unquote(tokens[1]);
        pNPCGreetings[i].pGreeting2 = unquote(tokens[2]);
    }
}

void NPCStats::InitializeNPCGroups(const Blob &npcGroups) {
    // npcgroup.txt table structure: group index | news index | dev notes.
    for (std::string_view line : split(npcGroups.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        int i = fromString<int>(tokens[0]); // File indices are 0-based.
        pOriginalGroups[i] = fromString<int>(tokens[1]);
    }
}

void NPCStats::InitializeNPCNews(const Blob &npcNews) {
    // npcnews.txt table structure: index | text (localized) | dev notes.
    for (std::string_view line : split(npcNews.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        int i = fromString<int>(tokens[0]); // File indices are 0-based.
        pCatchPhrases[i] = unquote(tokens[1]);
    }
}

//----- (0047702F) --------------------------------------------------------
void NPCStats::Initialize(ResourceManager *resourceManager) {
    pOriginalNPCData.fill(NPCData());
    InitializeNPCData(resourceManager->eventsData("npcdata.txt"));
    InitializeNPCGreets(resourceManager->eventsData("npcgreet.txt"));
    InitializeNPCGroups(resourceManager->eventsData("npcgroup.txt"));
    InitializeNPCNews(resourceManager->eventsData("npcnews.txt"));
    InitializeNPCText(resourceManager->eventsData("npctext.txt"));
    InitializeNPCTopics(resourceManager->eventsData("npctopic.txt"));
    InitializeNPCDist(resourceManager->eventsData("npcdist.txt"));
    InitializeNPCNames(resourceManager->eventsData("npcnames.txt"));
    InitializeNPCProfs(resourceManager->eventsData("npcprof.txt"));
}

void NPCStats::InitializeMm6(ResourceManager *resourceManager) {
    InitializeMm6Static(resourceManager->eventsData("npcdata.txt"),
                        resourceManager->eventsData("npcnews.txt"),
                        resourceManager->eventsData("npcnames.txt"),
                        resourceManager->eventsData("npcprof.txt"));

    Blob npcText = resourceManager->eventsData("npctext.txt");
    for (std::string_view line : split(npcText.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= 1 && id <= pNPCTopics.size())
            pNPCTopics[id - 1].pText = unquote(cells[1]);
    }

    Blob npcTopics = resourceManager->eventsData("npctopic.txt");
    for (std::string_view line : split(npcTopics.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= 1 && id < pNPCTopics.size())
            pNPCTopics[id].pTopic = unquote(cells[1]);
    }
}

void NPCStats::InitializeMm6Static(const Blob &npcData, const Blob &npcNews,
                                   const Blob &npcNames, const Blob &npcProfs) {
    pOriginalNPCData.fill(NPCData());
    pProfessionChance.fill({});
    for (std::string_view line : split(npcData.str()).by("\r\n").drop(2).skip("")) {
        std::array<std::string_view, 14> cells = split(line).by('\t');
        int id = 0;
        if (!tryDeserialize(cells[0], &id) || id < 1 || id >= pOriginalNPCData.size())
            continue;
        NPCData &npc = pOriginalNPCData[id];
        npc.name = unquote(cells[1]);
        pNPCUnicNames[id] = npc.name;
        npc.portraitId = fromString<int>(cells[2]);
        npc.house = static_cast<HouseId>(fromString<int>(cells[6]));
        npc.profession = static_cast<NpcProfession>(fromString<int>(cells[7]));
        npc.canJoin = cells[8] == "1";
        npc.newsTopic = fromString<int>(cells[9]);
        npc.dialogue_1_evt_id = fromString<int>(cells[10]);
        npc.dialogue_2_evt_id = fromString<int>(cells[11]);
        npc.dialogue_3_evt_id = fromString<int>(cells[12]);
        uNumNewNPCs = std::max(uNumNewNPCs, id + 1);
    }

    for (std::string_view line : split(npcNews.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 4> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= 0 && id < pCatchPhrases.size())
            pCatchPhrases[id] = unquote(cells[3]);
    }

    InitializeNPCNames(npcNames);

    for (std::string_view line : split(npcProfs.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 8> cells = split(line).by('\t');
        int id = 0;
        if (!tryDeserialize(cells[0], &id) || id < 1 || id > std::to_underlying(NPC_PROFESSION_LAST))
            continue;
        NPCProfession &profession = pProfessions[static_cast<NpcProfession>(id)];
        profession.uHirePrice = fromString<int>(cells[3]);
        profession.pActionText = unquote(cells[5]);
        profession.pBenefits = unquote(cells[6]);
        profession.pJoinText = unquote(cells[7]);
        uNumNPCProfessions = std::max(uNumNPCProfessions, id);
    }
    // This table has a global Chance field, unlike MM7's per-map npcdist.txt.
    // Sharing it across maps is a bounded inference until original generation
    // behavior is observed in game.
    NPCProfessionChance mm6ProfessionChance = parseMm6ProfessionChance(npcProfs);
    for (MapId map : allMaps())
        pProfessionChance[map] = mm6ProfessionChance;
}

void NPCStats::InitializeNPCNames(const Blob &npcNames) {
    // npcnames.txt table structure: male name (localized) | female name (localized).
    // Female column runs out before the male column.
    uNewlNPCBufPos = 0;
    pNPCNames.fill({});
    for (std::string_view line : split(npcNames.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        if (!tokens[0].empty())
            pNPCNames[SEX_MALE].emplace_back(unquote(tokens[0]));
        if (!tokens[1].empty())
            pNPCNames[SEX_FEMALE].emplace_back(unquote(tokens[1]));
    }
}

void NPCStats::InitializeNPCProfs(const Blob &npcProfs) {
    // npcprof.txt table structure: profession id | profession name (localized, not used) | hire price |
    //                              action text (localized) | benefit description (localized) |
    //                              join text (localized) | dismiss text (localized).
    for (std::string_view line : split(npcProfs.str()).by("\r\n").drop(4)) {
        std::array<std::string_view, 7> tokens = split(line).by('\t');
        if (tokens[0].empty())
            continue; // Trailing pure-tab orphan rows past the last entry.

        NpcProfession prof = static_cast<NpcProfession>(fromString<int>(tokens[0]));
        pProfessions[prof].uHirePrice = fromString<int>(tokens[2]);
        pProfessions[prof].pActionText = unquote(tokens[3]);
        // TODO(captainurist): MM7 says Hunter increases the identify monster skill "by four", but vanilla MM7 and OE
        //                     give +6. Fix the text as a data patch.
        pProfessions[prof].pBenefits = unquote(tokens[4]);
        pProfessions[prof].pJoinText = unquote(tokens[5]);
        pProfessions[prof].pDismissText = unquote(tokens[6]);
    }
    uNumNPCProfessions = 59;
}

//----- (0047732C) --------------------------------------------------------
void NPCStats::InitializeAdditionalNPCs(NPCData *pNPCDataBuff, MonsterId npc_uid,
                                        HouseId uLocation2D, MapId uMapId) {
    int rep_gen;
    int uGeneratedPortret;    // ecx@23
                              // signed int result; // eax@39
    Race uRace;                // [sp+Ch] [bp-Ch]@1
    bool break_gen;           // [sp+10h] [bp-8h]@1
    signed int gen_attempts;  // [sp+14h] [bp-4h]@1
    int uPortretMin;          // [sp+24h] [bp+Ch]@1
    int uPortretMax;

    MonsterType monsterType = monsterTypeForMonsterId(npc_uid);
    Sex uNPCSex = sexForMonsterType(monsterType);
    uRace = raceForMonsterType(monsterType);
    pNPCDataBuff->sex = uNPCSex;
    pNPCDataBuff->name = grng->randomSample(pNPCNames[uNPCSex]);

    gen_attempts = 0;
    break_gen = false;

    do {
        switch (uRace) {
            case RACE_HUMAN:
                if (uNPCSex == SEX_MALE) {
                    uPortretMin = 2;
                    uPortretMax = 100;
                } else {
                    uPortretMin = 201;
                    uPortretMax = 250;
                }
            case RACE_ELF:
                if (uNPCSex == SEX_MALE) {
                    uPortretMin = 400;
                    uPortretMax = 430;
                } else {
                    uPortretMin = 460;
                    uPortretMax = 490;
                }
                break;
            case RACE_GOBLIN:
                if (uNPCSex == SEX_MALE) {
                    uPortretMin = 500;
                    uPortretMax = 520;
                } else {
                    uPortretMin = 530;
                    uPortretMax = 550;
                }
                break;
            case RACE_DWARF:
                if (uNPCSex == SEX_MALE) {
                    uPortretMin = 300;
                    uPortretMax = 330;
                } else {
                    uPortretMin = 360;
                    uPortretMax = 387;
                }

                break;
        }

        uGeneratedPortret =
            uPortretMin + grng->random(uPortretMax - uPortretMin + 1);
        if (CheckPortretAgainstSex(uGeneratedPortret, uNPCSex))
            break_gen = true;
        ++gen_attempts;
        if (gen_attempts >= 4) {
            uGeneratedPortret = uPortretMin;
            break_gen = true;
        }
    } while (!break_gen);

    pNPCDataBuff->portraitId = uGeneratedPortret;
    pNPCDataBuff->flags = 0;
    pNPCDataBuff->fame = 0;
    // generate reputation
    rep_gen = grng->random(100) + 1;

    if (rep_gen >= 60) {
        if (rep_gen >= 90) {
            if (rep_gen >= 95) {
                if (rep_gen >= 98) {
                    pNPCDataBuff->rep = -600;
                } else {
                    pNPCDataBuff->rep = 400;
                }
            } else {
                pNPCDataBuff->rep = -300;
            }
        } else {
            pNPCDataBuff->rep = 200;
        }
    } else {
        pNPCDataBuff->rep = 0;
    }

    const NPCProfessionChance &chance = pProfessionChance[uMapId];
    if (engine->gameTitle() == GameTitle::MM6 && chance.total <= 0)
        throw Exception("MM6 additional NPC profession distribution is empty");
    pNPCDataBuff->profession = selectAdditionalNpcProfession(
        chance, engine->gameTitle(), grng->random(chance.total));
    pNPCDataBuff->house = uLocation2D;
    pNPCDataBuff->field_24 = 1;
    pNPCDataBuff->canJoin = 1;
    pNPCDataBuff->dialogue_1_evt_id = 0;
    pNPCDataBuff->dialogue_2_evt_id = 0;
    pNPCDataBuff->dialogue_3_evt_id = 0;
    pNPCDataBuff->dialogue_4_evt_id = 0;
    pNPCDataBuff->dialogue_5_evt_id = 0;
    pNPCDataBuff->dialogue_6_evt_id = 0;
}

//----- (00495366) --------------------------------------------------------
const std::string &NPCStats::sub_495366_MispronounceName(char firstLetter, Sex gender) {
    int pickedName;

    // TODO(captainurist): Caching in kinda wrong? Revisit when working on #mm6.
    //                     See "O Ho! %13! Er, %13. I think. Whatever..."

    if (firstLetter == dword_AE336C_LastMispronouncedNameFirstLetter) {
        pickedName = dword_AE3370_LastMispronouncedNameResult;
    } else {
        dword_AE336C_LastMispronouncedNameFirstLetter = firstLetter;
        const std::vector<std::string> &names = this->pNPCNames[gender];

        std::vector<int> matches;
        for (int i = 0; i < static_cast<int>(names.size()); ++i)
            if (tolower(names[i][0]) == tolower(firstLetter))
                matches.push_back(i);

        if (!matches.empty()) {
            pickedName = vrng->randomSample(matches);
        } else {
            pickedName = vrng->random(names.size()); // No name with this letter - pick any.
        }
    }
    dword_AE3370_LastMispronouncedNameResult = pickedName;
    return this->pNPCNames[gender][pickedName];
}
