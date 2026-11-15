#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"

namespace {

constexpr int castleSetupEventId = 1;
constexpr int warBannerEventId = 451;
constexpr int warBannerLeftCog = 16;
constexpr int warBannerRightCog = 17;
constexpr int advisorBannerLeftCog = 19;
constexpr int advisorBannerRightCog = 20;

bool eventComparesQuestBitAt(const EvtProgram &program, int eventId, int step, int targetStep, QuestBit bit) {
    const EvtInstruction &instruction = program.instruction(eventId, step);
    return instruction.opcode == EVENT_Compare && instruction.target_step == targetStep &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventSetsTextureAt(const EvtProgram &program, int eventId, int step, int cog, std::string_view texture) {
    const EvtInstruction &instruction = program.instruction(eventId, step);
    return instruction.opcode == EVENT_SetTexture &&
           instruction.data.sprite_texture_descr.cog == cog && instruction.str == texture;
}

std::vector<std::string> texturesForCog(int cog) {
    std::vector<std::string> result;
    for (const BLVFace &face : pIndoor->faces) {
        if (face.cogNumber == cog && face.GetTexture())
            result.push_back(face.GetTexture()->name());
    }
    return result;
}

void expectCogTexture(int cog, std::string_view expected) {
    const std::vector<std::string> textures = texturesForCog(cog);
    ASSERT_FALSE(textures.empty());
    for (const std::string &texture : textures)
        EXPECT_EQ(texture, expected);
}

void expectNoWarBanners() {
    EXPECT_EQ(texturesForCog(warBannerLeftCog), std::vector<std::string>({"chb1"}));
    EXPECT_EQ(texturesForCog(warBannerRightCog),
              std::vector<std::string>({"chb1", "nechumb", "wizh-b", "elfhumb"}));
}

void expectNoAdvisorBanners() {
    EXPECT_EQ(texturesForCog(advisorBannerLeftCog), std::vector<std::string>({"chb1"}));
    EXPECT_EQ(texturesForCog(advisorBannerRightCog), std::vector<std::string>({"chb1"}));
}

void enterRebuiltCastle(EngineController &game, std::initializer_list<QuestBit> bits) {
    pParty->_questBits.reset();
    pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
    for (QuestBit bit : bits)
        pParty->_questBits.set(bit);
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
}

void expectWarBannersForScore(int score) {
    if (score < 0) {
        expectCogTexture(warBannerLeftCog, "chb1p6");
        expectCogTexture(warBannerRightCog, "chb1p7");
    } else if (score > 0) {
        expectCogTexture(warBannerLeftCog, "elfhuma");
        expectCogTexture(warBannerRightCog, "elfhumb");
    } else {
        expectCogTexture(warBannerLeftCog, "bannwc-a");
        expectCogTexture(warBannerRightCog, "bannwc-b");
    }
}

} // namespace

GAME_TEST(MM7CastleFlags, SourceContract) {
    game.startNewGame();
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    const EvtProgram &program = engine->_localEventMap;
    ASSERT_TRUE(program.hasEvent(castleSetupEventId));
    ASSERT_TRUE(program.hasEvent(warBannerEventId));
    EXPECT_EQ(program.instruction(castleSetupEventId, 0).opcode, EVENT_OnMapReload);
    EXPECT_TRUE(eventComparesQuestBitAt(program, castleSetupEventId, 25, 28,
                                        QBIT_WALLS_OF_MIST_PASSED));
    EXPECT_TRUE(eventComparesQuestBitAt(program, castleSetupEventId, 26, 31,
                                        QBIT_BREEDING_ZONE_FINISHED));
    EXPECT_TRUE(eventSetsTextureAt(program, castleSetupEventId, 28, advisorBannerLeftCog, "wizh-a"));
    EXPECT_TRUE(eventSetsTextureAt(program, castleSetupEventId, 29, advisorBannerRightCog, "wizh-b"));
    EXPECT_TRUE(eventSetsTextureAt(program, castleSetupEventId, 31, advisorBannerLeftCog, "nechuma"));
    EXPECT_TRUE(eventSetsTextureAt(program, castleSetupEventId, 32, advisorBannerRightCog, "nechumb"));
    EXPECT_FALSE(std::ranges::any_of(program.function(castleSetupEventId), [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               (instruction.data.variable_descr.value == std::to_underlying(QBIT_LIGHT_PATH) ||
                instruction.data.variable_descr.value == std::to_underlying(QBIT_DARK_PATH));
    }));

    EXPECT_EQ(program.instruction(warBannerEventId, 0).opcode, EVENT_OnMapReload);
    EXPECT_TRUE(eventComparesQuestBitAt(program, warBannerEventId, 1, 9,
                                        QBIT_RIVERSTRIDE_PLANS_GIVEN));
    EXPECT_TRUE(eventComparesQuestBitAt(program, warBannerEventId, 2, 7,
                                        QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN));
    EXPECT_TRUE(eventComparesQuestBitAt(program, warBannerEventId, 3, 5,
                                        QBIT_FALSE_LOREN_GIVEN));
    EXPECT_TRUE(eventComparesQuestBitAt(program, warBannerEventId, 9, 13,
                                        QBIT_LOREN_STEEL_RESCUED));
    EXPECT_TRUE(eventSetsTextureAt(program, warBannerEventId, 16, warBannerLeftCog, "chb1p6"));
    EXPECT_TRUE(eventSetsTextureAt(program, warBannerEventId, 20, warBannerLeftCog, "elfhuma"));
    EXPECT_TRUE(eventSetsTextureAt(program, warBannerEventId, 24, warBannerLeftCog, "bannwc-a"));
}

GAME_TEST(MM7CastleFlags, AdvisorBannersRequireMatchingAlignment) {
    enum class Expected { None, Wizard, Necromancer };
    struct Case {
        bool light;
        bool dark;
        bool walls;
        bool breeding;
        Expected expected;
    };
    constexpr std::array cases = {
        Case{true, false, true, false, Expected::Wizard},
        Case{false, true, false, true, Expected::Necromancer},
        Case{false, true, true, false, Expected::None},
        Case{true, false, false, true, Expected::None},
        Case{false, false, true, true, Expected::None},
    };

    for (const Case &testCase : cases) {
        SCOPED_TRACE(fmt::format("light={} dark={} walls={} breeding={}", testCase.light, testCase.dark,
                                 testCase.walls, testCase.breeding));
        test.prepareForNextTest();
        game.startNewGame();
        pParty->_questBits.reset();
        pParty->_questBits.set(QBIT_HARMONDALE_REBUILT);
        if (testCase.light)
            pParty->_questBits.set(QBIT_LIGHT_PATH);
        if (testCase.dark)
            pParty->_questBits.set(QBIT_DARK_PATH);
        if (testCase.walls)
            pParty->_questBits.set(QBIT_WALLS_OF_MIST_PASSED);
        if (testCase.breeding)
            pParty->_questBits.set(QBIT_BREEDING_ZONE_FINISHED);
        game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);

        if (testCase.expected == Expected::Wizard) {
            expectCogTexture(advisorBannerLeftCog, "wizh-a");
            expectCogTexture(advisorBannerRightCog, "wizh-b");
        } else if (testCase.expected == Expected::Necromancer) {
            expectCogTexture(advisorBannerLeftCog, "nechuma");
            expectCogTexture(advisorBannerRightCog, "nechumb");
        } else {
            expectNoAdvisorBanners();
        }
    }
}

GAME_TEST(MM7CastleFlags, AllTwelveCompleteWarOutcomesUseCombinedScore) {
    struct ScoredBit {
        QuestBit bit;
        int score;
        std::string_view name;
    };
    constexpr std::array recipients = {
        ScoredBit{QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA, -1, "Erathia"},
        ScoredBit{QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE, 1, "Avlee"},
        ScoredBit{QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER, 0, "Arbiter"},
    };
    constexpr std::array plans = {
        ScoredBit{QBIT_RIVERSTRIDE_PLANS_GIVEN, 1, "real plans"},
        ScoredBit{QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN, -1, "false plans"},
    };
    constexpr std::array lorens = {
        ScoredBit{QBIT_LOREN_STEEL_RESCUED, -1, "real Loren"},
        ScoredBit{QBIT_FALSE_LOREN_GIVEN, 1, "false Loren"},
    };

    for (const ScoredBit &recipient : recipients) {
        for (const ScoredBit &plan : plans) {
            for (const ScoredBit &loren : lorens) {
                SCOPED_TRACE(fmt::format("{}; {}; {}", recipient.name, plan.name, loren.name));
                test.prepareForNextTest();
                game.startNewGame();
                enterRebuiltCastle(game, {recipient.bit, plan.bit, loren.bit});
                expectWarBannersForScore(recipient.score + plan.score + loren.score);
            }
        }
    }
}

GAME_TEST(MM7CastleFlags, IncompleteOrContradictoryWarStateKeepsBaseTextures) {
    const auto runCase = [&](std::initializer_list<QuestBit> bits) {
        test.prepareForNextTest();
        game.startNewGame();
        enterRebuiltCastle(game, bits);
        expectNoWarBanners();
    };

    runCase({});
    runCase({QBIT_FALSE_LOREN_GIVEN});
    EXPECT_TRUE(pParty->_questBits[QBIT_HARMONDALE_FACTION_BANNERS_HUNG]); // Preserve shipped quest-bit behavior.
    runCase({QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA, QBIT_RIVERSTRIDE_PLANS_GIVEN,
             QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN, QBIT_LOREN_STEEL_RESCUED});
    runCase({QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE,
             QBIT_RIVERSTRIDE_PLANS_GIVEN, QBIT_LOREN_STEEL_RESCUED});
    runCase({QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA, QBIT_RIVERSTRIDE_PLANS_GIVEN,
             QBIT_LOREN_STEEL_RESCUED, QBIT_FALSE_LOREN_GIVEN});
}
