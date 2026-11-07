#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6NpcResourceIdentity.h"

namespace {

Blob table(std::string_view text) { return Blob::copy(text.data(), text.size()); }

mm6::NpcResourceInputs inputs() {
    return {table("Header\r\nHeader\r\n1\tStatic Ada\t123\t0\t0\t0\t1\t2\t1\t7\t11\t12\t13\r\n"),
            table("0\tNews\tunused\tTown news\r\n"),
            table("Header\r\nAdam\tBeth\r\n"),
            table("1\tProfession\t2\t100\tunused\tAction\tBenefit\tJoin\r\n"),
            table("1\tHello\r\n"),
            table("1\tAsk\r\n")};
}

std::array<NPCTopic, 789> topics() {
    std::array<NPCTopic, 789> result{};
    result[0].pText = "Hello";
    result[1].pTopic = "Ask";
    return result;
}

bool hasBlocker(const mm6::NpcResourceIdentityPlan &plan,
                mm6::NpcResourceIdentityBlocker blocker) {
    return std::find(plan.blockers.begin(), plan.blockers.end(), blocker) != plan.blockers.end();
}

} // namespace

UNIT_TEST(MM6NpcResourceIdentity, MatchesStaticResourcesButBlocksGeneratedIdentityAndRng) {
    const auto source = inputs();
    const auto baseline = mm6::loadNpcStaticBaseline(
        source.npcData, source.npcNews, source.npcNames, source.npcProfessions, GameTitle::MM6);
    auto staged = std::make_unique<NPCStats>(baseline.stats());
    auto live = std::make_unique<NPCStats>(baseline.stats());
    staged->pNPCData[1].name = "Saved Ada";
    const auto saved = mm6::captureNpcFields(*staged, GameTitle::MM6);
    const auto liveTopics = topics();
    const auto plan = mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM6);
    EXPECT_FALSE(plan.canPublishNpcState());
    EXPECT_TRUE(hasBlocker(plan, mm6::NpcResourceIdentityBlocker::GeneratedNpcIdentityAfterMapLoad));
    EXPECT_TRUE(hasBlocker(plan, mm6::NpcResourceIdentityBlocker::GameRngStateUnrepresented));
    EXPECT_FALSE(hasBlocker(plan, mm6::NpcResourceIdentityBlocker::PriorGeneratedNpcStatePresent));
    EXPECT_EQ(staged->pNPCData[1].name, "Saved Ada");
    EXPECT_EQ(live->pOriginalNPCData[1].name, "Static Ada");

    live->uNewlNPCBufPos = 1;
    const auto stale = mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM6);
    EXPECT_TRUE(hasBlocker(stale, mm6::NpcResourceIdentityBlocker::PriorGeneratedNpcStatePresent));
}

UNIT_TEST(MM6NpcResourceIdentity, RejectsStaticTopicAndMutableMismatchBeforeAssignment) {
    const auto source = inputs();
    const auto baseline = mm6::loadNpcStaticBaseline(
        source.npcData, source.npcNews, source.npcNames, source.npcProfessions, GameTitle::MM6);
    auto staged = std::make_unique<NPCStats>(baseline.stats());
    auto live = std::make_unique<NPCStats>(baseline.stats());
    staged->pNPCData[1].name = "Saved Ada";
    const auto saved = mm6::captureNpcFields(*staged, GameTitle::MM6);
    auto liveTopics = topics();
    liveTopics[0].pText = "Stale";
    EXPECT_THROW((void) mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM6), std::exception);
    liveTopics = topics();
    live->pProfessions[static_cast<NpcProfession>(1)].uHirePrice++;
    EXPECT_THROW((void) mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM6), std::exception);
    live = std::make_unique<NPCStats>(baseline.stats());
    staged->pNPCData[1].name = "Different";
    EXPECT_THROW((void) mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::inspectNpcResourceIdentity(
        source, baseline, saved, *staged, *live, liveTopics, GameTitle::MM7), std::exception);
    EXPECT_EQ(live->pOriginalNPCData[1].name, "Static Ada");
}
