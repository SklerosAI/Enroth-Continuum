#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "Utility/Memory/Blob.h"

namespace {

struct ScopedPartyAndNpcs {
    Party party = *pParty;
    NPCStats npcs = *pNPCStats;

    ~ScopedPartyAndNpcs() {
        *pParty = party;
        *pNPCStats = npcs;
    }
};

std::string variableWrite(char step, char opcode, unsigned char variable, char value) {
    return std::string("\x09\x01\x00", 3) + step + opcode + static_cast<char>(variable) + value + std::string(3, '\0');
}

std::string variableCompare(char step, unsigned char variable, char value, char target) {
    return std::string("\x0a\x01\x00", 3) + step + '\x0e' + static_cast<char>(variable) + value + std::string(3, '\0') +
           target;
}

std::string forPartyMember(char step, char who) {
    return std::string("\x05\x01\x00", 3) + step + '\x23' + who;
}

EvtProgram syntheticRewardEvent() {
    // Add 10 to fire resistance, Set magic resistance bonus to 20, Add 20 skill points, hire NPC 13 and check that
    // it's hired, then make the second character dead. Shipped MM6 scripts use each of these operands.
    return EvtProgram::load(Blob::fromString(variableWrite(0, '\x10', 46, 10) +
                                             variableWrite(1, '\x12', 55, 20) +
                                             variableWrite(2, '\x10', 225, 20) +
                                             variableWrite(3, '\x12', 214, 13) +
                                             variableCompare(4, 214, 13, 7) +
                                             forPartyMember(5, 1) +
                                             variableWrite(6, '\x12', 101, 0)),
                            GameTitle::MM6);
}

void startMM6NewGame(EngineController &game) {
    // MM6 has no new-game save yet, so the override seeds the map deltas and chest state from the game archives.
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        pMapDeltas.clear();
        for (const std::string &name : pGames_LOD->ls())
            if (name.ends_with(".dlv") || name.ends_with(".ddm"))
                pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
        mm6::resetChestTitleState(&mm6::chestTitleState());
        if (!mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(), 0x12345678u, {}))
            throw std::runtime_error("MM6 chest title state did not restore");
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    game.runGameRoutine([] {
        for (Character &character : pParty->pCharacters) {
            for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
                const Skill skill = character.GetSkillIdxByOrder(order);
                if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                    character.pActiveSkills[skill] = CombinedSkillValue::novice();
            }
        }
        for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
            for (Character &character : pParty->pCharacters)
                for (Attribute stat : allStatAttributes())
                    character.IncreaseAttribute(stat);
    });
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
}

} // namespace

GAME_TEST(MM6EventVariables, InterpreterUsesMM6ResistanceSkillPointHireAndConditionStorage) {
    // MM6 scripts that touch resistances, skill points, hired NPCs or Dead used to throw before mutating anything.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedPartyAndNpcs restore;
    EvtProgram program = syntheticRewardEvent();
    EvtInterpreter interpreter;

    for (Character &character : pParty->pCharacters) {
        character.sResFireBase = 5;
        character.sResMagicBonus = 0;
        character.uSkillPoints = 3;
        character.conditions.resetAll();
    }
    pNPCStats->pNPCData[13].flags &= ~NPC_HIRED;
    pParty->GetPlayingTime() = Time::fromHours(9); // A condition stamped at time zero reads as absent.

    for (int step = 0; step < 4; ++step)
        EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, step)).type, EVT_FLOW_NEXT);
    for (const Character &character : pParty->pCharacters) {
        EXPECT_EQ(character.sResFireBase, 15);
        EXPECT_EQ(character.sResAirBase, 0);
        EXPECT_EQ(character.sResMagicBonus, 20);
        EXPECT_EQ(character.sResPhysicalBonus, 0);
        EXPECT_EQ(character.uSkillPoints, 23);
    }
    EXPECT_TRUE(pNPCStats->pNPCData[13].Hired());

    EvtFlow hired = interpreter.executeInstruction(program.instruction(1, 4));
    EXPECT_EQ(hired.type, EVT_FLOW_JUMP);
    EXPECT_EQ(hired.target, 7);

    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 5)).type, EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(program.instruction(1, 6)).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pParty->pCharacters[0].conditions.has(CONDITION_DEAD));
    EXPECT_TRUE(pParty->pCharacters[1].conditions.has(CONDITION_DEAD));
    EXPECT_FALSE(pParty->pCharacters[2].conditions.has(CONDITION_DEAD));
}

GAME_TEST(MM6EventVariables, FirstMapStatBonusAward) {
    // Clicking a first-map face that sets a stat bonus used to crash on the award animation, which MM6 has no icon for.
    auto mightTape = charTapes.stats(ATTRIBUTE_MIGHT);
    auto soundsTape = tapes.sounds();
    auto screenTape = tapes.screen();
    startMM6NewGame(game);
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    int faceId = -1;
    for (int i = 0; i < pOutdoor->faces.size(); i++)
        if (pOutdoor->faces[i].eventId == 150) // Sets the active character's might bonus to 10.
            faceId = i;
    ASSERT_NE(faceId, -1);
    game.teleportTo(engine->_currentLoadedMapId, pOutdoor->faces[faceId].boundingBox.center() - Vec3f(300, 0, 0), 0);
    test.startTaping();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();
    game.pointMouseAtFace(faceId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    EXPECT_EQ(mightTape.delta(), tape(0, 10, 0, 0));
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_quest);
    EXPECT_EQ(screenTape, tape(SCREEN_GAME));
}

GAME_TEST(MM6EventVariables, ShippedReputationWritesLandOncePerTargetedCharacter) {
    // MM6 reputation scripts used to throw. A write lands once per targeted character, so four times for the party.
    struct Write {
        int eventId;
        int whoStep;
        EvtTargetCharacter who;
        int writeStep;
        int reputation;
    };
    // Frank's reward in the New Sorpigal Town Hall adds 50 for the active character. Global event 30 subtracts 1000
    // from the whole party, of which only the low byte 232 counts. Global event 380 adds 200 for the whole party.
    for (Write write : {Write{326, 7, CHOOSE_ACTIVE, 9, 50}, Write{30, 8, CHOOSE_PARTY, 9, -928}, Write{380, 0, CHOOSE_PARTY, 6, 800}}) {
        ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
        ScopedPartyAndNpcs restore;
        const EvtInstruction &who = engine->_globalEventMap.instruction(write.eventId, write.whoStep);
        const EvtInstruction &reputation = engine->_globalEventMap.instruction(write.eventId, write.writeStep);
        ASSERT_EQ(who.opcode, EVENT_ForPartyMember);
        ASSERT_EQ(who.who, write.who);
        ASSERT_EQ(std::to_underlying(reputation.data.variable_descr.type), 215);
        pParty->mm6Reputation = 0;

        EvtInterpreter interpreter;
        EXPECT_EQ(interpreter.executeInstruction(who).type, EVT_FLOW_NEXT);
        EXPECT_EQ(interpreter.executeInstruction(reputation).type, EVT_FLOW_NEXT);
        EXPECT_EQ(pParty->mm6Reputation, write.reputation) << write.eventId;
    }
}
