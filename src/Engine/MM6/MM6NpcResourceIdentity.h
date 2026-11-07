#pragma once

#include <array>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Tables/NPCTable.h"
#include "Utility/Memory/Blob.h"

#include "MM6NpcSaveFields.h"
#include "MM6NpcStaticBaseline.h"

class ResourceManager;

namespace mm6 {

struct NpcResourceInputs {
    Blob npcData;
    Blob npcNews;
    Blob npcNames;
    Blob npcProfessions;
    Blob npcText;
    Blob npcTopics;
};

enum class NpcResourceIdentityBlocker {
    GeneratedNpcIdentityAfterMapLoad,
    GameRngStateUnrepresented,
    PriorGeneratedNpcStatePresent,
    MispronouncedNameCacheNotReset,
};

struct NpcResourceIdentityPlan {
    std::vector<NpcResourceIdentityBlocker> blockers;

    bool canPublishNpcState() const { return blockers.empty(); }
};

// Reparse the loaded MM6 NPC resources on detached storage and compare static
// tables, global topics and the staged mutable RESC overlay before assignment.
// Does not create generated NPCs, consume RNG or write any process owner.
NpcResourceIdentityPlan inspectNpcResourceIdentity(
    const NpcResourceInputs &inputs, const NpcStaticBaseline &baseline,
    const NpcSaveFields &saved, const NPCStats &staged, const NPCStats &live,
    std::span<const NPCTopic> liveTopics, GameTitle title);

NpcResourceIdentityPlan inspectNpcResourceIdentity(
    ResourceManager &resources, const NpcStaticBaseline &baseline,
    const NpcSaveFields &saved, const NPCStats &staged, const NPCStats &live,
    std::span<const NPCTopic> liveTopics, GameTitle title);

} // namespace mm6
