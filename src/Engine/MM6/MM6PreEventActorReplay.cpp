#include "MM6PreEventActorReplay.h"

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "MM6ActiveActorSection.h"
#include "MM6GameplayRngSaveFields.h"
#include "MM6PreEventGeneratedNpcReplay.h"

namespace mm6 {

static bool sameNativeAnchor(const Actor &rebuilt, const Actor &saved, std::size_t slot) {
    return rebuilt.id == slot && saved.id == slot &&
           rebuilt.monsterInfo.id == saved.monsterInfo.id &&
           rebuilt.monsterId == saved.monsterId &&
           rebuilt.initialPosition == saved.initialPosition;
}

void requirePreEventActorReplaySource(const V3OutdoorActorMapAgreement &agreement, GameTitle title) {
    if (title != GameTitle::MM6 ||
        agreement.activeMap.mapName.empty() || agreement.activeActors.empty() ||
        agreement.saved.raw.metadata.header.mapFileName != agreement.activeMap.mapName ||
        agreement.saved.generatedNpcs.mapName != agreement.activeMap.mapName ||
        agreement.activeMap.delta.actors.size() != agreement.activeSidecar.actors.size() ||
        encodeActiveActorSection(agreement.activeActors, title) !=
            agreement.saved.activeActorPayload)
        throw std::runtime_error("MM6 pre-event actor replay requires a sole staged map");
    (void) encodeGeneratedNpcSection(agreement.saved.generatedNpcs, title);
    (void) encodeGameplayRngSection(agreement.saved.gameplayRng, title);
}

PreEventActorReplayPlan stagePreEventActorReplay(
    const std::vector<Actor> &savedActors,
    const GeneratedNpcSaveFields &savedGenerated,
    const GameplayRandomState &savedRng,
    const std::deque<Actor> &rebuiltActors,
    const NPCStats &freshNpcs,
    std::string_view loadedMapName,
    std::size_t nativeActorCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    GameTitle title) {
    if (title != GameTitle::MM6 || loadedMapName.empty() ||
        rebuiltActors.size() != nativeActorCount ||
        savedActors.size() < nativeActorCount ||
        savedActors.size() > runtimeActorCap ||
        freshNpcs.uNewlNPCBufPos != 0)
        throw std::runtime_error("MM6 v3 pre-event replay lacks an exact fresh actor context");
    for (std::size_t slot = 0; slot < rebuiltActors.size(); ++slot)
        if (!sameNativeAnchor(rebuiltActors[slot], savedActors[slot], slot))
            throw std::runtime_error("MM6 v3 rebuilt actor anchor differs from ACTR");

    PreEventActorReplayPlan plan;
    plan.actors.assign(savedActors.begin(), savedActors.end());
    const auto generated = stagePreEventGeneratedNpcReplay(
        savedGenerated, savedRng, plan.actors, freshNpcs,
        loadedMapName, selectedRngType, title);
    requireGeneratedNpcActorAgreement(savedGenerated, plan.actors,
                                      loadedMapName, title);
    plan.generatedRows = freshNpcs.pAdditionalNPC;
    for (std::size_t row = 0; row < generated.generatedCount; ++row)
        plan.generatedRows[row] = generated.generated.rows[row].npc;
    plan.generatedCount = static_cast<int>(generated.generatedCount);
    plan.gameplayRng = generated.gameplayRng;
    return plan;
}

void publishPreEventActorReplay(PreEventActorReplayPlan plan, std::deque<Actor> *liveActors,
                                NPCStats *liveNpcs, GameTitle title) {
    if (title != GameTitle::MM6 || !liveActors || !liveNpcs)
        throw std::runtime_error("MM6 v3 pre-event replay lacks a publication owner");
    static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
    static_assert(std::is_nothrow_swappable_v<std::array<NPCData, 100>>);
    liveActors->swap(plan.actors);
    liveNpcs->pAdditionalNPC.swap(plan.generatedRows);
    liveNpcs->uNewlNPCBufPos = plan.generatedCount;
}

} // namespace mm6
