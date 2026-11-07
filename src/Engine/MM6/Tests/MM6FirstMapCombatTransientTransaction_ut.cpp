#include <stdexcept>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6FirstMapCombatTransientTransaction.h"

namespace {

TurnBased_QueueElem queueEntry(Pid pid, int initiative) {
    TurnBased_QueueElem result;
    result.uPackedID = pid;
    result.actor_initiative = initiative;
    return result;
}

UNIT_TEST(MM6FirstMapCombatTransients, RejectsInvalidScopeBeforeMutation) {
    std::vector<AttackDescription> attacks(1);
    stru262_TurnBased turnEngine;
    turnEngine.pQueue = {queueEntry(Pid::actor(2), 12)};
    turnEngine.turns_count = 5;
    std::vector<Pid> targets{Pid::actor(3)};
    std::vector<unsigned int> ids{4};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &turnEngine, &targets, &ids};

    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM7, owners, 2), std::exception);
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, -1), std::exception);
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, Pid::ID_MAX + 1), std::exception);
    owners.turnEngine = nullptr;
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, 2), std::exception);
    owners.turnEngine = &turnEngine;
    owners.nearActorIds = nullptr;
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, 2), std::exception);
    EXPECT_EQ(attacks.size(), 1);
    EXPECT_EQ(turnEngine.pQueue[0].uPackedID, Pid::actor(2));
    EXPECT_EQ(turnEngine.pQueue[0].actor_initiative, 12);
    EXPECT_EQ(turnEngine.turns_count, 5);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(ids[0], 4u);
}

UNIT_TEST(MM6FirstMapCombatTransients, LateFaultRestoresContainersAndTurnScalars) {
    std::vector<AttackDescription> attacks(1);
    attacks[0].pid = Pid::actor(7);
    attacks[0].attackRange = 12;
    stru262_TurnBased turnEngine;
    turnEngine.pQueue = {
        queueEntry(Pid::actor(2), 12), queueEntry(Pid::character(1), 23),
        queueEntry(Pid::actor(4), 34)};
    turnEngine.turns_count = 5;
    turnEngine.turn_stage = TE_WAIT;
    turnEngine.ai_turn_timer = Duration::fromTicks(41);
    turnEngine.turn_initiative = 7;
    turnEngine.uActionPointsLeft = 9;
    turnEngine.flags |= TE_HAVE_PENDING_ACTIONS;
    turnEngine.pending_actions = 2;
    std::vector<Pid> targets{Pid::actor(3), Pid::character(2)};
    std::vector<unsigned int> ids{4, 5};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &turnEngine, &targets, &ids};

    try {
        mm6::ScopedFirstMapCombatTransients scope(GameTitle::MM6, owners, 4);
        EXPECT_TRUE(attacks.empty());
        EXPECT_EQ(turnEngine.pQueue.size(), 1);
        EXPECT_EQ(turnEngine.pQueue[0].uPackedID, Pid::character(1));
        EXPECT_EQ(turnEngine.pQueue[0].actor_initiative, 23);
        EXPECT_EQ(targets.size(), 4);
        EXPECT_EQ(targets[0], Pid::actor(3));
        EXPECT_EQ(targets[1], Pid::character(2));
        EXPECT_EQ(targets[3], Pid());
        EXPECT_EQ(ids.size(), 4);
        EXPECT_EQ(ids[0], 4u);
        EXPECT_EQ(ids[1], 5u);

        attacks.emplace_back();
        turnEngine.pQueue.push_back(queueEntry(Pid::actor(8), 99));
        turnEngine.turns_count = 50;
        turnEngine.turn_stage = TE_ATTACK;
        turnEngine.ai_turn_timer = Duration::fromTicks(99);
        turnEngine.turn_initiative = 70;
        turnEngine.uActionPointsLeft = 90;
        turnEngine.flags &= ~TE_HAVE_PENDING_ACTIONS;
        turnEngine.pending_actions = 20;
        targets[0] = Pid::actor(8);
        ids[0] = 99;
        throw std::runtime_error("late outdoor failure");
    } catch (const std::runtime_error &) {
    }

    EXPECT_EQ(attacks.size(), 1);
    EXPECT_EQ(attacks[0].pid, Pid::actor(7));
    EXPECT_EQ(attacks[0].attackRange, 12);
    EXPECT_EQ(turnEngine.pQueue.size(), 3);
    EXPECT_EQ(turnEngine.pQueue[0].uPackedID, Pid::actor(2));
    EXPECT_EQ(turnEngine.pQueue[0].actor_initiative, 12);
    EXPECT_EQ(turnEngine.pQueue[1].uPackedID, Pid::character(1));
    EXPECT_EQ(turnEngine.pQueue[1].actor_initiative, 23);
    EXPECT_EQ(turnEngine.pQueue[2].uPackedID, Pid::actor(4));
    EXPECT_EQ(turnEngine.pQueue[2].actor_initiative, 34);
    EXPECT_EQ(turnEngine.turns_count, 5);
    EXPECT_EQ(turnEngine.turn_stage, TE_WAIT);
    EXPECT_EQ(turnEngine.ai_turn_timer, Duration::fromTicks(41));
    EXPECT_EQ(turnEngine.turn_initiative, 7);
    EXPECT_EQ(turnEngine.uActionPointsLeft, 9);
    EXPECT_TRUE(bool(turnEngine.flags & TE_HAVE_PENDING_ACTIONS));
    EXPECT_EQ(turnEngine.pending_actions, 2);
    EXPECT_EQ(targets.size(), 2);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(targets[1], Pid::character(2));
    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 4u);
    EXPECT_EQ(ids[1], 5u);
}

UNIT_TEST(MM6FirstMapCombatTransients, CommitKeepsPreparedReset) {
    std::vector<AttackDescription> attacks(1);
    stru262_TurnBased turnEngine;
    turnEngine.pQueue = {
        queueEntry(Pid::actor(2), 12), queueEntry(Pid::character(0), 23)};
    std::vector<Pid> targets{Pid::actor(3), Pid::actor(4)};
    std::vector<unsigned int> ids{4, 5};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &turnEngine, &targets, &ids};
    {
        mm6::ScopedFirstMapCombatTransients scope(GameTitle::MM6, owners, 1);
        turnEngine.turns_count = 12;
        turnEngine.turn_stage = TE_MOVEMENT;
        turnEngine.ai_turn_timer = Duration::fromTicks(10);
        turnEngine.turn_initiative = 8;
        turnEngine.uActionPointsLeft = 3;
        turnEngine.flags |= TE_PLAYER_TURN;
        turnEngine.pending_actions = 1;
        scope.commit();
    }
    EXPECT_TRUE(attacks.empty());
    EXPECT_EQ(turnEngine.pQueue.size(), 1);
    EXPECT_EQ(turnEngine.pQueue[0].uPackedID, Pid::character(0));
    EXPECT_EQ(turnEngine.turns_count, 12);
    EXPECT_EQ(turnEngine.turn_stage, TE_MOVEMENT);
    EXPECT_EQ(turnEngine.ai_turn_timer, Duration::fromTicks(10));
    EXPECT_EQ(turnEngine.turn_initiative, 8);
    EXPECT_EQ(turnEngine.uActionPointsLeft, 3);
    EXPECT_TRUE(bool(turnEngine.flags & TE_PLAYER_TURN));
    EXPECT_EQ(turnEngine.pending_actions, 1);
    EXPECT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(ids.size(), 1);
    EXPECT_EQ(ids[0], 4u);
}

} // namespace
