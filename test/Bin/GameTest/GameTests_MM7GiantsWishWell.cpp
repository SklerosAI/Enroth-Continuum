#include <array>
#include <cmath>
#include <numbers>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Spells/SpellBuff.h"
#include "Utility/Exception.h"

namespace {

constexpr int wishWellEventId = 205;

bool eventHasVariableInstruction(const EvtProgram &program, int step, EvtOpcode opcode,
                                 EvtVariable variable, int value) {
    return std::ranges::any_of(program.function(wishWellEventId),
                               [=](const EvtInstruction &instruction) {
        return instruction.step == step && instruction.opcode == opcode &&
               instruction.data.variable_descr.type == variable &&
               instruction.data.variable_descr.value == value;
    });
}

bool eventHasFiveThousandGoldCharge(const EvtProgram &program) {
    return std::ranges::any_of(program.function(wishWellEventId), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_FixedGold &&
               instruction.data.variable_descr.value == 5000;
    });
}

void prepareWell(EngineController &game, int gold) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();
    game.teleportTo(MAP_LAND_OF_THE_GIANTS, Vec3f(), 0);
    game.goToGame(); // A new party's first arrival starts Archibald's scripted greeting and gives a Blaster.
    pParty->takeHoldingItem();
    pParty->SetGold(gold, true);
    pParty->pPartyBuffs[PARTY_BUFF_PROTECTION_FROM_MAGIC].Apply(
        pParty->GetPlayingTime() + Duration::fromDays(1), MASTERY_GRANDMASTER, 100, 0, 0);
}

int pointAtWishWell(EngineController &game) {
    for (int faceId = 0; faceId < std::ssize(pOutdoor->faces); ++faceId) {
        const BLVFace &face = pOutdoor->faces[faceId];
        if (face.eventId != wishWellEventId || !face.Clickable())
            continue;

        for (float distance : {150.0f, 250.0f, 350.0f, 500.0f}) {
            for (const Vec3f &direction : std::array{
                     Vec3f(1, 0, 0), Vec3f(-1, 0, 0), Vec3f(0, 1, 0), Vec3f(0, -1, 0)}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                bool onWater = false;
                int floorFaceId = -1;
                position.z = ODM_GetFloorLevel(position, &onWater, &floorFaceId);
                const int yaw = std::lround(std::atan2(-direction.y, -direction.x) * 180 /
                                            std::numbers::pi);
                game.teleportTo(MAP_LAND_OF_THE_GIANTS, position, yaw, -30);
                try {
                    game.pointMouseAtFace(faceId);
                    return faceId;
                } catch (const Exception &) {
                }
            }
        }
    }
    return -1;
}

void clickWishWell(EngineController &game) {
    ASSERT_NE(pointAtWishWell(game), -1);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

} // namespace

GAME_TEST(MM7GiantsWishWell, SourceContractAndRuntimeIdempotence) {
    prepareWell(game, 20000);
    const EvtProgram &program = engine->_localEventMap;
    ASSERT_TRUE(program.hasEvent(wishWellEventId));
    EXPECT_TRUE(eventHasVariableInstruction(program, 0, EVENT_Compare, VAR_FixedGold, 5000));
    EXPECT_EQ(program.instruction(wishWellEventId, 0).target_step, 3);
    EXPECT_TRUE(eventHasVariableInstruction(program, 1, EVENT_Subtract, VAR_FixedGold, 4999));
    EXPECT_FALSE(eventHasFiveThousandGoldCharge(program));

    clickWishWell(game);
    clickWishWell(game);

    EXPECT_EQ(program.instruction(wishWellEventId, 0).target_step, 3);
    EXPECT_FALSE(eventHasFiveThousandGoldCharge(program));
}

GAME_TEST(MM7GiantsWishWell, QualifyingClicksPayEveryTime) {
    prepareWell(game, 20000);
    auto goldTape = tapes.gold();
    auto statusTape = tapes.statusBar();
    test.startTaping();

    clickWishWell(game);
    const int afterFirstWish = pParty->GetGold();
    EXPECT_TRUE(afterFirstWish == 15000 || afterFirstWish == 25000);
    clickWishWell(game);
    const int afterSecondWish = pParty->GetGold();
    EXPECT_TRUE(afterSecondWish == afterFirstWish - 5000 || afterSecondWish == afterFirstWish + 5000);

    EXPECT_CONTAINS(statusTape, "You make a wish");
    EXPECT_EQ(goldTape.back(), afterSecondWish);
}

GAME_TEST(MM7GiantsWishWell, PreservesOriginalGoldBoundaries) {
    for (int gold : {4998, 4999, 5000}) {
        SCOPED_TRACE(fmt::format("gold={}", gold));
        test.prepareForNextTest();
        prepareWell(game, gold);
        auto statusTape = tapes.statusBar();
        test.startTaping();

        clickWishWell(game);

        if (gold == 4998) {
            EXPECT_EQ(pParty->GetGold(), 4998);
            EXPECT_MISSES(statusTape, "You make a wish");
        } else if (gold == 4999) {
            EXPECT_EQ(pParty->GetGold(), 0);
            EXPECT_CONTAINS(statusTape, "You make a wish");
        } else {
            EXPECT_TRUE(pParty->GetGold() == 0 || pParty->GetGold() == 10000);
            EXPECT_CONTAINS(statusTape, "You make a wish");
        }
    }
}
