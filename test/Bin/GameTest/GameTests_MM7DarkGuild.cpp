#include <algorithm>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"

#include "MM7CampaignChain.h"

static std::vector<int> breedingZoneCompletionEvents() {
    std::vector<int> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (!face.Clickable() || !engine->_localEventMap.hasEvent(face.eventId))
            continue;
        const auto &instructions = engine->_localEventMap.function(face.eventId);
        bool completesQuest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(QBIT_BREEDING_ZONE_FINISHED);
        });
        if (completesQuest && std::ranges::find(result, face.eventId) == result.end())
            result.push_back(face.eventId);
    }
    return result;
}

GAME_TEST(MM7DarkGuild, BreedingZoneMarkerDoesNotGrantDarkGuild) {
    // The normal exit only returns the party to the Pit. The actual in-dungeon completion event must be found from
    // the loaded map program and clicked, so the test cannot silently substitute a generic exit for this objective.
    for (bool assigned : {false, true}) {
        SCOPED_TRACE(fmt::format("assigned={}", assigned));
        test.prepareForNextTest();
        auto activeTape = tapes.questBit(QBIT_BREEDING_ZONE_ACTIVE);
        auto finishedTape = tapes.questBit(QBIT_BREEDING_ZONE_FINISHED);
        auto guildTape = tapes.questBit(QBIT_DARK_GUILD_JOINED);
        engine->config->debug.NoActors.setValue(true);
        game.startNewGame();
        pParty->_questBits.set(QBIT_DARK_PATH);
        if (assigned)
            pParty->_questBits.set(QBIT_BREEDING_ZONE_ACTIVE);
        test.startTaping();

        game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
        const std::vector<int> completionEvents = breedingZoneCompletionEvents();
        ASSERT_EQ(completionEvents.size(), 1u);
        ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_BREEDING_ZONE, completionEvents.front(), 1, [] {
            return pParty->_questBits[QBIT_BREEDING_ZONE_FINISHED];
        }));

        // The dungeon's marker can be set before Archibald has assigned the task, but it is not the actual quest
        // completion: only the later Archibald hand-in may grant Dark-guild membership. Keep this distinction so a
        // future chain cannot mistake the map bit for an eligible adviser progression.
        EXPECT_EQ(activeTape, assigned ? tape(true) : tape(false));
        EXPECT_EQ(finishedTape, tape(false, true));
        EXPECT_EQ(guildTape, tape(false));
    }
}
