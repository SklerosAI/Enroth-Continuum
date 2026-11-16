#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/TurnEngine/TurnEngine.h"
#include "Engine/mm7_data.h"

GAME_TEST(MM7TransitionCombat, StalePreviousMapActorIsIgnoredAtTurnBasedStart) {
    game.startNewGame();

    ASSERT_FALSE(ai_near_actors_ids.empty());
    ai_near_actors_ids[0] = pActors.size() + 100;
    ai_arrays_size = 1;

    // Input can reach this path immediately after an indoor transition, before
    // the new map has rebuilt its nearby-actor list. This used to abort in
    // std::deque::operator[] when the old map had a larger actor id.
    pTurnEngine->Start();
    pParty->bTurnBasedModeOn = true;

    EXPECT_TRUE(std::ranges::all_of(pTurnEngine->pQueue, [](const TurnBased_QueueElem &element) {
        return element.uPackedID.type() != OBJECT_Actor || element.uPackedID.id() < pActors.size();
    }));

    pParty->bTurnBasedModeOn = false;
    pTurnEngine->End(false);
}
