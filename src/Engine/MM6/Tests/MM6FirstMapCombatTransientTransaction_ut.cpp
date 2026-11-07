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
    std::vector<TurnBased_QueueElem> queue{queueEntry(Pid::actor(2), 12)};
    std::vector<Pid> targets{Pid::actor(3)};
    std::vector<unsigned int> ids{4};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &queue, &targets, &ids};

    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM7, owners, 2), std::exception);
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, -1), std::exception);
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, Pid::ID_MAX + 1), std::exception);
    owners.nearActorIds = nullptr;
    EXPECT_THROW(mm6::ScopedFirstMapCombatTransients(GameTitle::MM6, owners, 2), std::exception);
    EXPECT_EQ(attacks.size(), 1);
    EXPECT_EQ(queue[0].uPackedID, Pid::actor(2));
    EXPECT_EQ(queue[0].actor_initiative, 12);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(ids[0], 4u);
}

UNIT_TEST(MM6FirstMapCombatTransients, LateFaultRestoresAllFourOwners) {
    std::vector<AttackDescription> attacks(1);
    attacks[0].pid = Pid::actor(7);
    attacks[0].attackRange = 12;
    std::vector<TurnBased_QueueElem> queue{
        queueEntry(Pid::actor(2), 12), queueEntry(Pid::character(1), 23),
        queueEntry(Pid::actor(4), 34)};
    std::vector<Pid> targets{Pid::actor(3), Pid::character(2)};
    std::vector<unsigned int> ids{4, 5};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &queue, &targets, &ids};

    try {
        mm6::ScopedFirstMapCombatTransients scope(GameTitle::MM6, owners, 4);
        EXPECT_TRUE(attacks.empty());
        EXPECT_EQ(queue.size(), 1);
        EXPECT_EQ(queue[0].uPackedID, Pid::character(1));
        EXPECT_EQ(queue[0].actor_initiative, 23);
        EXPECT_EQ(targets.size(), 4);
        EXPECT_EQ(targets[0], Pid::actor(3));
        EXPECT_EQ(targets[1], Pid::character(2));
        EXPECT_EQ(targets[3], Pid());
        EXPECT_EQ(ids.size(), 4);
        EXPECT_EQ(ids[0], 4u);
        EXPECT_EQ(ids[1], 5u);

        attacks.emplace_back();
        queue.push_back(queueEntry(Pid::actor(8), 99));
        targets[0] = Pid::actor(8);
        ids[0] = 99;
        throw std::runtime_error("late outdoor failure");
    } catch (const std::runtime_error &) {
    }

    EXPECT_EQ(attacks.size(), 1);
    EXPECT_EQ(attacks[0].pid, Pid::actor(7));
    EXPECT_EQ(attacks[0].attackRange, 12);
    EXPECT_EQ(queue.size(), 3);
    EXPECT_EQ(queue[0].uPackedID, Pid::actor(2));
    EXPECT_EQ(queue[0].actor_initiative, 12);
    EXPECT_EQ(queue[1].uPackedID, Pid::character(1));
    EXPECT_EQ(queue[1].actor_initiative, 23);
    EXPECT_EQ(queue[2].uPackedID, Pid::actor(4));
    EXPECT_EQ(queue[2].actor_initiative, 34);
    EXPECT_EQ(targets.size(), 2);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(targets[1], Pid::character(2));
    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 4u);
    EXPECT_EQ(ids[1], 5u);
}

UNIT_TEST(MM6FirstMapCombatTransients, CommitKeepsPreparedReset) {
    std::vector<AttackDescription> attacks(1);
    std::vector<TurnBased_QueueElem> queue{
        queueEntry(Pid::actor(2), 12), queueEntry(Pid::character(0), 23)};
    std::vector<Pid> targets{Pid::actor(3), Pid::actor(4)};
    std::vector<unsigned int> ids{4, 5};
    mm6::FirstMapCombatTransientOwners owners{&attacks, &queue, &targets, &ids};
    {
        mm6::ScopedFirstMapCombatTransients scope(GameTitle::MM6, owners, 1);
        scope.commit();
    }
    EXPECT_TRUE(attacks.empty());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue[0].uPackedID, Pid::character(0));
    EXPECT_EQ(targets.size(), 1);
    EXPECT_EQ(targets[0], Pid::actor(3));
    EXPECT_EQ(ids.size(), 1);
    EXPECT_EQ(ids[0], 4u);
}

} // namespace
