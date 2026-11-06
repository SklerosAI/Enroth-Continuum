#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6NpcEventState.h"

UNIT_TEST(MM6NpcEventState, TopicAndLocationMutateOneNpc) {
    NPCStats stats;
    mm6::setNpcTopic(stats, GameTitle::MM6, 330, 0, 364);
    mm6::setNpcTopic(stats, GameTitle::MM6, 330, 2, 0);
    mm6::moveNpc(stats, GameTitle::MM6, 330, static_cast<HouseId>(486));

    EXPECT_EQ(stats.pNPCData[330].dialogue_1_evt_id, 364);
    EXPECT_EQ(stats.pNPCData[330].dialogue_2_evt_id, 0);
    EXPECT_EQ(stats.pNPCData[330].dialogue_3_evt_id, 0);
    EXPECT_EQ(stats.pNPCData[330].house, static_cast<HouseId>(486));
    EXPECT_EQ(stats.pNPCData[329].dialogue_1_evt_id, 0);
    EXPECT_EQ(stats.pNPCData[329].house, HOUSE_INVALID);

    mm6::moveNpc(stats, GameTitle::MM6, 330, HOUSE_INVALID);
    EXPECT_EQ(stats.pNPCData[330].house, HOUSE_INVALID);
}

UNIT_TEST(MM6NpcEventState, ForeignAndInvalidOperandsRejectBeforeMutation) {
    NPCStats stats;
    stats.pNPCData[1].dialogue_1_evt_id = 50;
    stats.pNPCData[1].house = static_cast<HouseId>(20);
    EXPECT_THROW(mm6::setNpcTopic(stats, GameTitle::MM7, 1, 0, 60), std::runtime_error);
    EXPECT_THROW(mm6::setNpcTopic(stats, GameTitle::MM6, 0, 0, 60), std::runtime_error);
    EXPECT_THROW(mm6::setNpcTopic(stats, GameTitle::MM6, 501, 0, 60), std::runtime_error);
    EXPECT_THROW(mm6::setNpcTopic(stats, GameTitle::MM6, 1, 3, 60), std::runtime_error);
    EXPECT_THROW(mm6::setNpcTopic(stats, GameTitle::MM6, 1, 0, 365), std::runtime_error);
    EXPECT_THROW(mm6::moveNpc(stats, GameTitle::MM7, 1, HOUSE_INVALID), std::runtime_error);
    EXPECT_THROW(mm6::moveNpc(stats, GameTitle::MM6, 1, static_cast<HouseId>(558)), std::runtime_error);
    EXPECT_EQ(stats.pNPCData[1].dialogue_1_evt_id, 50);
    EXPECT_EQ(stats.pNPCData[1].house, static_cast<HouseId>(20));
}

UNIT_TEST(MM6NpcEventState, GlobalScriptPrecedesMM7TeacherRange) {
    EXPECT_EQ(mm6::npcTopicRoute(GameTitle::MM6, 250, true), mm6::NpcTopicRoute::GlobalEvent);
    EXPECT_EQ(mm6::npcTopicRoute(GameTitle::MM6, 250, false), mm6::NpcTopicRoute::Guarded);
    EXPECT_EQ(mm6::npcTopicRoute(GameTitle::MM6, 0, true), mm6::NpcTopicRoute::Guarded);
    EXPECT_EQ(mm6::npcTopicRoute(GameTitle::MM7, 250, true), mm6::NpcTopicRoute::Legacy);
    EXPECT_EQ(mm6::npcTopicRoute(GameTitle::MM7, 399, false), mm6::NpcTopicRoute::Legacy);
}
