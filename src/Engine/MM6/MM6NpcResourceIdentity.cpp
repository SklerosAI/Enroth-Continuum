#include "MM6NpcResourceIdentity.h"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "Engine/MapEnumFunctions.h"
#include "Engine/Resources/ResourceManager.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

namespace mm6 {
namespace {

void requireSameStaticTables(const NPCStats &candidate, const NPCStats &source) {
    if (candidate.uNumNewNPCs != source.uNumNewNPCs ||
        candidate.uNumNPCProfessions != source.uNumNPCProfessions ||
        candidate.pNPCUnicNames != source.pNPCUnicNames ||
        candidate.pNPCNames != source.pNPCNames ||
        candidate.pCatchPhrases != source.pCatchPhrases ||
        candidate.pOriginalGroups != source.pOriginalGroups)
        throw std::runtime_error("MM6 NPC static source tables differ");
    for (std::size_t index = 0; index < candidate.pOriginalNPCData.size(); ++index)
        if (encodeNpcRecord(candidate.pOriginalNPCData[index]) !=
            encodeNpcRecord(source.pOriginalNPCData[index]))
            throw std::runtime_error("MM6 original NPC rows differ from resources");
    for (int raw = std::to_underlying(NPC_PROFESSION_FIRST);
         raw <= std::to_underlying(NPC_PROFESSION_MM6_LAST); ++raw) {
        const auto id = static_cast<NpcProfession>(raw);
        const NPCProfession &a = candidate.pProfessions[id];
        const NPCProfession &b = source.pProfessions[id];
        if (a.uHirePrice != b.uHirePrice || a.pBenefits != b.pBenefits ||
            a.pActionText != b.pActionText || a.pJoinText != b.pJoinText ||
            a.pDismissText != b.pDismissText)
            throw std::runtime_error("MM6 NPC profession table differs from resources");
    }
    for (MapId map : allMaps()) {
        const NPCProfessionChance &a = candidate.pProfessionChance[map];
        const NPCProfessionChance &b = source.pProfessionChance[map];
        if (a.total != b.total || a.chanceByProfession != b.chanceByProfession)
            throw std::runtime_error("MM6 NPC profession weights differ from resources");
    }
}

std::array<NPCTopic, 789> expectedTopics(const Blob &text, const Blob &topics) {
    std::array<NPCTopic, 789> expected{};
    for (std::string_view line : split(text.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= 1 && id <= expected.size())
            expected[id - 1].pText = unquote(cells[1]);
    }
    for (std::string_view line : split(topics.str()).by("\r\n").skip("")) {
        std::array<std::string_view, 2> cells = split(line).by('\t');
        int id = 0;
        if (tryDeserialize(cells[0], &id) && id >= 1 && id < expected.size())
            expected[id].pTopic = unquote(cells[1]);
    }
    return expected;
}

} // namespace

NpcResourceIdentityPlan inspectNpcResourceIdentity(
    const NpcResourceInputs &inputs, const NpcStaticBaseline &baseline,
    const NpcSaveFields &saved, const NPCStats &staged, const NPCStats &live,
    std::span<const NPCTopic> liveTopics, GameTitle title) {
    if (title != GameTitle::MM6 || liveTopics.size() != 789)
        throw std::runtime_error("MM6 NPC resource identity requires title-local topics");
    const NpcStaticBaseline fresh = loadNpcStaticBaseline(
        inputs.npcData, inputs.npcNews, inputs.npcNames, inputs.npcProfessions, title);
    requireSameStaticTables(baseline.stats(), fresh.stats());
    requireSameStaticTables(staged, fresh.stats());
    requireSameStaticTables(live, fresh.stats());
    if (encodeNpcSection(captureNpcFields(staged, title)).payload !=
        encodeNpcSection(saved).payload)
        throw std::runtime_error("MM6 staged NPC mutable state differs from RESC");
    const auto expected = expectedTopics(inputs.npcText, inputs.npcTopics);
    for (std::size_t index = 0; index < expected.size(); ++index)
        if (liveTopics[index].pText != expected[index].pText ||
            liveTopics[index].pTopic != expected[index].pTopic)
            throw std::runtime_error("MM6 NPC topic table differs from loaded resources");

    NpcResourceIdentityPlan result;
    result.blockers.push_back(NpcResourceIdentityBlocker::GeneratedNpcIdentityAfterMapLoad);
    result.blockers.push_back(NpcResourceIdentityBlocker::GameRngStateUnrepresented);
    if (live.uNewlNPCBufPos != 0 || staged.uNewlNPCBufPos != 0)
        result.blockers.push_back(NpcResourceIdentityBlocker::PriorGeneratedNpcStatePresent);
    if (NPCStats::dword_AE336C_LastMispronouncedNameFirstLetter != -1 ||
        NPCStats::dword_AE3370_LastMispronouncedNameResult != -1)
        result.blockers.push_back(NpcResourceIdentityBlocker::MispronouncedNameCacheNotReset);
    return result;
}

NpcResourceIdentityPlan inspectNpcResourceIdentity(
    ResourceManager &resources, const NpcStaticBaseline &baseline,
    const NpcSaveFields &saved, const NPCStats &staged, const NPCStats &live,
    std::span<const NPCTopic> liveTopics, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 NPC resource identity requires MM6 title");
    NpcResourceInputs inputs{resources.eventsData("npcdata.txt"),
                             resources.eventsData("npcnews.txt"),
                             resources.eventsData("npcnames.txt"),
                             resources.eventsData("npcprof.txt"),
                             resources.eventsData("npctext.txt"),
                             resources.eventsData("npctopic.txt")};
    return inspectNpcResourceIdentity(inputs, baseline, saved, staged, live, liveTopics, title);
}

} // namespace mm6
