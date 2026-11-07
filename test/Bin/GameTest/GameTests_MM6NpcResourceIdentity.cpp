#include <algorithm>
#include <memory>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6NpcResourceIdentity.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"

// Run alone in a fresh null-platform MM6 process. This reads the loaded title
// resources and globals but does not assign NPC rows, topics or RNG state.
GAME_TEST(MM6ZNpcResourceHeadless, DISABLED_LoadedResourcesMatchFreshNpcIdentity) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(engine->resources(), nullptr);
    ASSERT_NE(pNPCStats, nullptr);

    const auto baseline = std::make_unique<mm6::NpcStaticBaseline>(
        mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6));
    const auto saved = std::make_unique<mm6::NpcSaveFields>(
        mm6::captureNpcFields(*pNPCStats, GameTitle::MM6));
    auto staged = std::make_unique<NPCStats>(baseline->stats());
    staged->pNPCData = saved->npcData;
    staged->pGroups = saved->groups;
    staged->uNumNewNPCs = saved->activeCount;
    const auto plan = mm6::inspectNpcResourceIdentity(
        *engine->resources(), *baseline, *saved, *staged, *pNPCStats, pNPCTopics, GameTitle::MM6);
    EXPECT_FALSE(plan.canPublishNpcState());
    EXPECT_NE(std::find(plan.blockers.begin(), plan.blockers.end(),
                        mm6::NpcResourceIdentityBlocker::GeneratedNpcIdentityAfterMapLoad),
              plan.blockers.end());
    EXPECT_NE(std::find(plan.blockers.begin(), plan.blockers.end(),
                        mm6::NpcResourceIdentityBlocker::GameRngStateUnrepresented),
              plan.blockers.end());
    EXPECT_EQ(window->nativeHandle(), nullptr);
}
