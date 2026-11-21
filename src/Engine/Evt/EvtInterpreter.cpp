#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <tl/generator.hpp>

#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtEnumFunctions.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Evt/EvtVariables.h"
#include "Engine/Evt/MM6EventRules.h"
#include "Engine/Evt/MM6EventVariables.h"
#include "Engine/Evt/MM6AutonoteState.h"
#include "Engine/Evt/MM6SkillEvent.h"
#include "Engine/Evt/MM6MapByteState.h"
#include "Engine/Evt/MM6QuestBitState.h"
#include "Engine/Evt/MM6DayOfYear.h"
#include "Engine/Evt/MM6MonthOfYear.h"
#include "Engine/Evt/MM6PartyFlying.h"
#include "Engine/Evt/MM6CharacterEventBit.h"
#include "Engine/Evt/MM6GoldEvent.h"
#include "Engine/Evt/MM6NpcEventState.h"
#include "Engine/Evt/MM6SpriteEvent.h"
#include "Engine/Evt/MM6ItemEventCompare.h"
#include "Engine/Evt/MM6ItemEventWrite.h"
#include "Engine/Evt/MM6GiveItemEvent.h"
#include "Engine/Evt/MM6EventVariableDispatch.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Party.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Weather.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/MonsterEnumFunctions.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Random/Random.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Spells/Spells.h"
#include "Engine/Engine.h"
#include "Engine/Tables/MapTable.h"

#include "Io/Mouse.h"

#include "Media/Audio/AudioPlayer.h"
#include "Media/MediaPlayer.h"

#include "Utility/Math/TrigLut.h"
#include "Utility/String/Transformations.h"
#include "Utility/Exception.h"

#include "GUI/GUIProgressBar.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/UI/Houses/MM6House.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIDialogue.h"
#include "GUI/UI/UIBranchlessDialogue.h"
#include "GUI/UI/UITransition.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/UIMonsterLoot.h"

#include "Library/Logger/Logger.h"

static bool isOriginalOreSmithReward(const EvtContext &context, std::span<const EvtInstruction> instructions,
                                     const EvtInstruction &reward) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_GLOBAL ||
        context.eventId < 501 || context.eventId > 503 || reward.opcode != EVENT_GiveItem ||
        reward.data.give_item_descr.item_id != ITEM_NULL)
        return false;
    const int oreIndex = (reward.step - 9) / 4;
    if (reward.step < 9 || reward.step > 29 || reward.step != 9 + 4 * oreIndex ||
        std::to_underlying(reward.data.give_item_descr.treasure_level) != 6 - oreIndex ||
        std::to_underlying(reward.data.give_item_descr.treasure_type) != context.eventId - 481)
        return false;

    // The six original branches each gate a held ore, consume it once, then give one random item.
    // A patched script with different reward semantics must use its own event behavior.
    for (int i = 0; i < 6; ++i) {
        const int ore = std::to_underlying(ITEM_STALT_LACED_ORE) - i;
        const auto has = [&](int step, EvtOpcode opcode, bool isReward) {
            return std::ranges::any_of(instructions, [&](const EvtInstruction &ir) {
                if (ir.step != step || ir.opcode != opcode)
                    return false;
                if (isReward)
                    return ir.data.give_item_descr.item_id == ITEM_NULL &&
                           std::to_underlying(ir.data.give_item_descr.treasure_level) == 6 - i &&
                           std::to_underlying(ir.data.give_item_descr.treasure_type) == context.eventId - 481;
                return ir.data.variable_descr.type == VAR_PlayerItemInHands &&
                       ir.data.variable_descr.value == ore;
            });
        };
        if (!has(i, EVENT_Compare, false) || !has(8 + 4 * i, EVENT_Subtract, false) ||
            !has(9 + 4 * i, EVENT_GiveItem, true))
            return false;
    }
    return true;
}

/**
 * @offset 0x4465DF
 */
static bool checkSeason(Season season) {
    int monthPlusOne = pParty->uCurrentMonth + 1;
    int daysPlusOne = pParty->uCurrentDayOfMonth + 1;

    switch (season) {
        case SEASON_WINTER:  // winter 12.21 -> 3.20
            return (monthPlusOne == 12 && daysPlusOne >= 21 ||
                    monthPlusOne == 1 || monthPlusOne == 2 ||
                    monthPlusOne == 3 && daysPlusOne <= 20);

        case SEASON_AUTUMN:  // autumn/fall 9.21 -> 12.20
            return (monthPlusOne == 9 && daysPlusOne >= 21 ||
                    monthPlusOne == 10 || monthPlusOne == 11 ||
                    monthPlusOne == 12 && daysPlusOne <= 20);

        case SEASON_SUMMER:  // summer 6.21 -> 9.20
            return (monthPlusOne == 6 && daysPlusOne >= 21 ||
                    monthPlusOne == 7 || monthPlusOne == 8 ||
                    monthPlusOne == 9 && daysPlusOne <= 20);

        case SEASON_SPRING:  // spring 3.21 -> 6.20
            return (monthPlusOne == 3 && daysPlusOne >= 21 ||
                    monthPlusOne == 4 || monthPlusOne == 5 ||
                    monthPlusOne == 6 && daysPlusOne <= 20);

        default:
            assert(false);
            return false;
    }
}

constexpr int breedingZoneBookEventId = 195;
constexpr int breedingZoneBookChestId = 0;
constexpr int breedingZoneFallbackChestId = 19;
constexpr int tidewaterMapEventId = 176;
constexpr int tidewaterMapChestId = 1;
constexpr int tidewaterFallbackChestId = 0;
constexpr int tidewaterMapVariable = 2;
constexpr int wallsLichJarEventId = 176;
constexpr int wallsLichJarChestId = 0;
constexpr int wallsLichJarFallbackChestId = 1;
constexpr int wallsLichJarDecorVariable = 23;
constexpr int titansPerfectBowEventId = 176;
constexpr int titansPerfectBowChestId = 1;
constexpr int titansPerfectBowFallbackChestId = 0;
constexpr int titansPerfectBowMapVariable = 2;
constexpr int mazeHaldarsRemainsEventId = 176;
constexpr int mazeHaldarsRemainsChestId = 1;
constexpr int mazeHaldarsRemainsFallbackChestId = 0;
constexpr int mazeHaldarsRemainsMapVariable = 0;
constexpr int lightEndingCompletionEventId = 170;
constexpr int darkEndingCompletionEventId = 172;
constexpr int giantsWishWellEventId = 205;
constexpr int deyjaGuardSetupEventId = 1;
constexpr int deyjaAlliedGuardGroup = 4;
constexpr int deyjaAngerCounterIndex = 9;
constexpr int deyjaGuardStateMapVariable = 4;
constexpr int castleHarmondaleSetupEventId = 1;
constexpr int castleHarmondaleWarBannerEventId = 451;
constexpr int castleHarmondaleWarBannerLeftCog = 16;
constexpr int castleHarmondaleWarBannerRightCog = 17;
constexpr int castleHarmondaleAdvisorBannerLeftCog = 19;
constexpr int castleHarmondaleAdvisorBannerRightCog = 20;
constexpr int castleHarmondaleGolemEventId = 376;
constexpr int celesteLadyCarmineMapLeaveEventId = 378;
constexpr int wineCellarKillCheckEventId = 2;
constexpr int colonyZodKillMovieEventId = 2;
constexpr int kastoreAltarPiecesEventId = 158;
constexpr int kastoreHasDarkPieceTextId = 328;
constexpr int kastoreNeedsDarkPieceTextId = 329;

static bool isOriginalBreedingZoneBookEvent(const EvtContext &context,
                                            std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_BREEDING_ZONE || context.eventId != breedingZoneBookEventId)
        return false;

    const bool hasOriginalGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_DIVINE_INTERVENTION_RETRIEVED);
    });
    const bool hasMapVariableGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_MapPersistentVariable_0;
    });
    const bool hasBookChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == breedingZoneBookChestId;
    });
    const bool hasFallbackChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == breedingZoneFallbackChestId;
    });
    return hasOriginalGate && !hasMapVariableGate && hasBookChest && hasFallbackChest;
}

static bool isOriginalTidewaterMapEvent(const EvtContext &context,
                                        std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_TIDEWATER_CAVERNS || context.eventId != tidewaterMapEventId)
        return false;

    const auto changesVariableAt = [instructions](int step, int targetStep, EvtOpcode opcode,
                                                   EvtVariable variable, int value) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.target_step == targetStep &&
                   instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                   instruction.data.variable_descr.value == value;
        });
    };
    const auto opensChestAt = [instructions](int step, int chestId) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_OpenChest &&
                   instruction.data.chest_id == chestId;
        });
    };
    const bool hasHint = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == -1 && instruction.opcode == EVENT_MouseOver;
    });
    const bool hasTerminalJump = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 4 && instruction.target_step == 6 && instruction.opcode == EVENT_Jmp;
    });
    const bool hasExit = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 6 && instruction.opcode == EVENT_Exit;
    });
    return instructions.size() == 8 && hasHint &&
           changesVariableAt(0, 5, EVENT_Compare, VAR_Award,
                             std::to_underlying(AWARD_PROMOTION_PRIEST)) &&
           changesVariableAt(1, 5, EVENT_Compare, VAR_Award,
                             std::to_underlying(AWARD_PROMOTION_PRIEST_HONORARY)) &&
           opensChestAt(2, tidewaterMapChestId) &&
           changesVariableAt(3, 0, EVENT_Add, VAR_QBits_QuestsDone,
                             std::to_underlying(QBIT_ORACLE_RECOVERS_EVENMORN_MAP)) &&
           hasTerminalJump && opensChestAt(5, tidewaterFallbackChestId) && hasExit;
}

static bool isOriginalColonyZodKillMovieEvent(const EvtContext &context,
                                              std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_COLONY_ZOD || context.eventId != colonyZodKillMovieEventId ||
        instructions.size() != 6)
        return false;

    const auto atStep = [instructions](int step) {
        return std::ranges::find(instructions, step, &EvtInstruction::step);
    };
    const auto trigger = atStep(0);
    const auto killed = atStep(1);
    const auto exit = atStep(2);
    const auto setKilled = atStep(3);
    const auto movie = atStep(4);
    const auto history = atStep(5);
    if (trigger == instructions.end() || killed == instructions.end() || exit == instructions.end() ||
        setKilled == instructions.end() || movie == instructions.end() || history == instructions.end())
        return false;
    const EvtVariable history24 = static_cast<EvtVariable>(std::to_underlying(VAR_History_0) + 24);
    return trigger->opcode == EVENT_OnMapLeave && killed->opcode == EVENT_IsActorKilled &&
           killed->target_step == 3 && killed->data.actor_descr.policy == KILL_CHECK_ACTORID &&
           killed->data.actor_descr.param == 0 && killed->data.actor_descr.num == 0 &&
           exit->opcode == EVENT_Exit && setKilled->opcode == EVENT_Set &&
           setKilled->data.variable_descr.type == VAR_QBits_QuestsDone &&
           setKilled->data.variable_descr.value == std::to_underlying(QBIT_XENOFEX_KILLED) &&
           movie->opcode == EVENT_ShowMovie && movie->data.movie_unknown_field == 1 &&
           trimRemoveQuotes(movie->str) == "family reunion" && history->opcode == EVENT_Add &&
           history->data.variable_descr.type == history24 && history->data.variable_descr.value == 0;
}

static bool isOriginalKastoreAltarPiecesEvent(const EvtContext &context,
                                               std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_GLOBAL ||
        context.eventId != kastoreAltarPiecesEventId || instructions.size() != 22)
        return false;

    const auto atStep = [instructions](int step) {
        return std::ranges::find(instructions, step, &EvtInstruction::step);
    };
    std::array<std::span<const EvtInstruction>::iterator, 22> steps;
    for (int step = 0; step < std::ssize(steps); ++step) {
        steps[step] = atStep(step);
        if (steps[step] == instructions.end())
            return false;
    }
    const auto comparesItem = [&steps](int step, ItemId item, int targetStep) {
        return steps[step]->opcode == EVENT_Compare && steps[step]->target_step == targetStep &&
               steps[step]->data.variable_descr.type == VAR_PlayerItemInHands &&
               steps[step]->data.variable_descr.value == std::to_underlying(item);
    };
    const auto showsText = [&steps](int step, int textId) {
        return steps[step]->opcode == EVENT_ShowMessage && steps[step]->data.text_id == textId;
    };
    const auto removesItem = [&steps](int step, ItemId item) {
        return steps[step]->opcode == EVENT_Subtract &&
               steps[step]->data.variable_descr.type == VAR_PlayerItemInHands &&
               steps[step]->data.variable_descr.value == std::to_underlying(item);
    };
    const auto changesVariable = [&steps](int step, EvtOpcode opcode, EvtVariable variable, int value) {
        return steps[step]->opcode == opcode && steps[step]->data.variable_descr.type == variable &&
               steps[step]->data.variable_descr.value == value;
    };
    const EvtVariable history19 =
        static_cast<EvtVariable>(std::to_underlying(VAR_History_0) + 19);

    return steps[0]->opcode == EVENT_ForPartyMember && steps[0]->who == CHOOSE_PARTY &&
           comparesItem(1, ITEM_QUEST_ALTAR_PIECE_LIGHT, 7) &&
           comparesItem(2, ITEM_QUEST_ALTAR_PIECE_DARK, 5) &&
           showsText(3, 327) && steps[4]->opcode == EVENT_Exit &&
           showsText(5, kastoreNeedsDarkPieceTextId) && steps[6]->opcode == EVENT_Exit &&
           comparesItem(7, ITEM_QUEST_ALTAR_PIECE_DARK, 10) &&
           showsText(8, kastoreHasDarkPieceTextId) && steps[9]->opcode == EVENT_Exit &&
           showsText(10, 330) &&
           removesItem(11, ITEM_QUEST_ALTAR_PIECE_LIGHT) &&
           removesItem(12, ITEM_QUEST_ALTAR_PIECE_DARK) &&
           changesVariable(13, EVENT_Subtract, VAR_QBits_QuestsDone,
                           std::to_underlying(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_LIGHT)) &&
           changesVariable(14, EVENT_Subtract, VAR_QBits_QuestsDone,
                           std::to_underlying(QBIT_ORACLE_RECOVERS_ALTAR_PIECE_DARK)) &&
           changesVariable(15, EVENT_Add, history19, 0) &&
           changesVariable(16, EVENT_Add, VAR_Experience, 50000) &&
           steps[17]->opcode == EVENT_SetNPCTopic &&
           steps[17]->data.npc_topic_descr.npc_id == 84 &&
           steps[17]->data.npc_topic_descr.index == 0 &&
           steps[17]->data.npc_topic_descr.event_id == 0 &&
           steps[18]->opcode == EVENT_Subtract &&
           steps[18]->data.variable_descr.type == VAR_QBits_QuestsDone &&
           steps[18]->data.variable_descr.value == std::to_underlying(QBIT_ALTAR_PIECES_DARK_ACTIVE) &&
           steps[19]->opcode == EVENT_Set &&
           steps[19]->data.variable_descr.type == VAR_QBits_QuestsDone &&
           steps[19]->data.variable_descr.value == std::to_underlying(QBIT_ALTAR_PIECES_DARK_FINISHED) &&
           changesVariable(20, EVENT_Set, VAR_Award, std::to_underlying(AWARD_47)) &&
           steps[21]->opcode == EVENT_SetNPCGreeting &&
           steps[21]->data.npc_descr.npc_id == 84 && steps[21]->data.npc_descr.greeting == 112;
}

static bool isOriginalWallsLichJarEvent(const EvtContext &context,
                                        std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_WALLS_OF_MIST || context.eventId != wallsLichJarEventId)
        return false;

    const bool hasOriginalAwardGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.target_step == 1 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_THE_PIT_RETRIEVED_SOUL_JARS);
    });
    const bool hasActiveQuestGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_LICH_JARS_ACTIVE);
    });
    const bool hasDecorVariableGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentDecorVariable_0) +
                                            wallsLichJarDecorVariable);
    });
    const bool hasJarChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 4 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == wallsLichJarChestId;
    });
    const bool hasFallbackChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == wallsLichJarFallbackChestId;
    });
    const bool hasTakenBitWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 5 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_WALLS_OF_MIST_LICH_JARS_TAKEN);
    });
    const bool hasDecorVariableWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 6 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentDecorVariable_0) +
                                            wallsLichJarDecorVariable) &&
               instruction.data.variable_descr.value == 1;
    });
    return hasOriginalAwardGate && hasActiveQuestGate && !hasDecorVariableGate && hasJarChest &&
           hasFallbackChest && hasTakenBitWrite && hasDecorVariableWrite;
}

static bool isOriginalTitansPerfectBowEvent(const EvtContext &context,
                                            std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_TITANS_STRONGHOLD || context.eventId != titansPerfectBowEventId)
        return false;

    const bool hasPromotionAwardGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 6 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_PROMOTION_MASTER_ARCHER);
    });
    const bool hasLightQuestGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 4 && instruction.target_step == 8 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_PERFECT_BOW_LIGHT_ACTIVE);
    });
    const bool hasDarkQuestGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 5 && instruction.target_step == 8 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_PERFECT_BOW_DARK_ACTIVE);
    });
    const bool hasFallbackChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 6 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == titansPerfectBowFallbackChestId;
    });
    const bool hasPrematureTakenWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 8 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_PERFECT_BOW_TAKEN);
    });
    const bool hasBowChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 9 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == titansPerfectBowChestId;
    });
    const bool hasMapVariableInstruction = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Set ||
                instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentVariable_0) +
                                            titansPerfectBowMapVariable);
    });
    return hasPromotionAwardGate && hasLightQuestGate && hasDarkQuestGate && hasFallbackChest &&
           hasPrematureTakenWrite && hasBowChest && !hasMapVariableInstruction;
}

static bool isOriginalMazeHaldarsRemainsEvent(const EvtContext &context,
                                              std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_MAZE || context.eventId != mazeHaldarsRemainsEventId)
        return false;

    const bool hasAwardGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 5 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value == std::to_underlying(AWARD_NIGHON_RETRIEVED_HALDARS_REMAINS);
    });
    const bool hasTakenGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 5 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HALDARS_REMAINS_TAKEN);
    });
    const bool hasRemainsChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == mazeHaldarsRemainsChestId;
    });
    const bool hasTakenWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HALDARS_REMAINS_TAKEN);
    });
    const bool hasFallbackChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 5 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == mazeHaldarsRemainsFallbackChestId;
    });
    const bool hasMapVariableInstruction = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Set ||
                instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_MapPersistentVariable_0;
    });
    return hasAwardGate && hasTakenGate && hasRemainsChest && hasTakenWrite && hasFallbackChest &&
           !hasMapVariableInstruction;
}

static std::optional<QuestBit> originalEndingActiveBitCleanup(
    const EvtContext &context, std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_GLOBAL ||
        (context.eventId != lightEndingCompletionEventId && context.eventId != darkEndingCompletionEventId))
        return std::nullopt;

    const bool light = context.eventId == lightEndingCompletionEventId;
    const QuestBit activeBit = light ? QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE
                                     : QBIT_OSCILLATION_OVERTHRUSTER_DARK_ACTIVE;
    const int npcId = light ? 80 : 84;
    const bool hasItemGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 4 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value ==
                   std::to_underlying(ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    });
    const bool hasItemRemoval = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 4 && instruction.opcode == EVENT_Subtract &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value ==
                   std::to_underlying(ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    });
    const auto removesQuestBitAt = [instructions](int step, QuestBit bit) {
        return std::ranges::any_of(instructions, [step, bit](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_Subtract &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(bit);
        });
    };
    const bool hasTopicClear = std::ranges::any_of(instructions, [npcId](const EvtInstruction &instruction) {
        return instruction.step == 7 && instruction.opcode == EVENT_SetNPCTopic &&
               instruction.data.npc_topic_descr.npc_id == npcId &&
               instruction.data.npc_topic_descr.index == 1 &&
               instruction.data.npc_topic_descr.event_id == 0;
    });
    const bool alreadyCleansActiveBit = std::ranges::any_of(instructions, [activeBit](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(activeBit);
    });
    if (!hasItemGate || !hasItemRemoval || !removesQuestBitAt(5, QBIT_ORACLE_RECOVERS_WETSUIT) ||
        !removesQuestBitAt(6, QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER) || !hasTopicClear ||
        alreadyCleansActiveBit)
        return std::nullopt;

    return activeBit;
}

static bool isOriginalGiantsWishWellEvent(const EvtContext &context,
                                          std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_LAND_OF_THE_GIANTS || context.eventId != giantsWishWellEventId)
        return false;

    const bool hasOriginalGoldGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_FixedGold &&
               instruction.data.variable_descr.value == 5000;
    });
    const bool hasInsufficientGoldWithdrawal = std::ranges::any_of(
        instructions, [](const EvtInstruction &instruction) {
            return instruction.step == 1 && instruction.opcode == EVENT_Subtract &&
                   instruction.data.variable_descr.type == VAR_FixedGold &&
                   instruction.data.variable_descr.value == 4999;
        });
    const bool hasInsufficientGoldExit = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.target_step == 30 && instruction.opcode == EVENT_Jmp;
    });
    const bool hasWishRandomizer = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_RandomGoTo;
    });
    const bool hasWishStatus = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 30 && instruction.opcode == EVENT_StatusText;
    });
    const bool alreadyChargesFiveThousand = std::ranges::any_of(
        instructions, [](const EvtInstruction &instruction) {
            return instruction.opcode == EVENT_Subtract &&
                   instruction.data.variable_descr.type == VAR_FixedGold &&
                   instruction.data.variable_descr.value == 5000;
        });
    return hasOriginalGoldGate && hasInsufficientGoldWithdrawal && hasInsufficientGoldExit &&
           hasWishRandomizer && hasWishStatus && !alreadyChargesFiveThousand;
}

static bool isOriginalDeyjaGuardSetupEvent(const EvtContext &context,
                                           std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_DEYJA || context.eventId != deyjaGuardSetupEventId ||
        !context.duringMapLoad)
        return false;

    const auto mapVariable = [](int index) {
        return static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentVariable_0) + index);
    };
    const auto hasVariableInstruction = [instructions](int step, int targetStep, EvtOpcode opcode,
                                                        EvtVariable variable, int value) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.target_step == targetStep &&
                   instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
                   instruction.data.variable_descr.value == value;
        });
    };
    const auto togglesGroup = [instructions](int step, int group, bool set) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_ToggleActorGroupFlag &&
                   instruction.data.actor_flag_descr.id == group &&
                   instruction.data.actor_flag_descr.attr == ACTOR_AGGRESSOR &&
                   instruction.data.actor_flag_descr.is_set == set;
        });
    };
    const bool hasMapLoadTrigger = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_OnMapReload;
    });
    const bool hasPatchedFactionWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_ChangeGroupAlly;
    });
    return hasMapLoadTrigger &&
           hasVariableInstruction(1, 0, EVENT_Set, mapVariable(29), 5) &&
           hasVariableInstruction(2, 6, EVENT_Compare, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_LIGHT_PATH)) &&
           hasVariableInstruction(3, 10, EVENT_Compare, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_ALLIED_GUARDS_ANGERED)) &&
           hasVariableInstruction(4, 7, EVENT_Compare, mapVariable(deyjaGuardStateMapVariable), 2) &&
           hasVariableInstruction(6, 0, EVENT_Set, mapVariable(deyjaGuardStateMapVariable), 2) &&
           togglesGroup(7, deyjaAlliedGuardGroup, true) && togglesGroup(8, 5, true) &&
           hasVariableInstruction(10, 12, EVENT_Compare, VAR_Counter10, 720) &&
           hasVariableInstruction(12, 0, EVENT_Subtract, VAR_QBits_QuestsDone,
                                  std::to_underlying(QBIT_ALLIED_GUARDS_ANGERED)) &&
           hasVariableInstruction(13, 0, EVENT_Set, mapVariable(deyjaGuardStateMapVariable), 0) &&
           togglesGroup(14, deyjaAlliedGuardGroup, false) && !hasPatchedFactionWrite;
}

static void applyDeyjaGuardFaction() {
    const bool angered = pParty->_questBits[QBIT_ALLIED_GUARDS_ANGERED];
    const Time angerStartedAt = pParty->PartyTimes.CounterEventValues[deyjaAngerCounterIndex];
    const bool angerExpired = angered && angerStartedAt.isValid() &&
                              angerStartedAt + Duration::fromHours(720) <= pParty->GetPlayingTime();
    const bool hostile = pParty->_questBits[QBIT_LIGHT_PATH] ||
                         (!angerExpired &&
                          (angered || engine->_persistentVariables.mapVars[deyjaGuardStateMapVariable] >= 2));
    for (Actor &actor : pActors) {
        if (actor.group != deyjaAlliedGuardGroup)
            continue;
        actor.hostilityGroup = hostile ? monsterTypeForMonsterId(actor.monsterInfo.id)
                                       : MONSTER_TYPE_PEASANT_GOBLIN_FEMALE_A;
        if (hostile) {
            actor.attributes |= ACTOR_AGGRESSOR;
        } else {
            actor.ResetAggressor();
            actor.ResetHostile();
        }
    }
}

static bool isOriginalCastleHarmondaleAdvisorBannerEvent(
    const EvtContext &context, std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_CASTLE_HARMONDALE ||
        context.eventId != castleHarmondaleSetupEventId || !context.duringMapLoad)
        return false;

    const auto comparesQuestBitAt = [instructions](int step, int targetStep, QuestBit bit) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.target_step == targetStep &&
                   instruction.opcode == EVENT_Compare &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(bit);
        });
    };
    const auto setsTextureAt = [instructions](int step, int cog, std::string_view texture) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_SetTexture &&
                   instruction.data.sprite_texture_descr.cog == cog && instruction.str == texture;
        });
    };
    const bool hasAlignmentGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               (instruction.data.variable_descr.value == std::to_underlying(QBIT_LIGHT_PATH) ||
                instruction.data.variable_descr.value == std::to_underlying(QBIT_DARK_PATH));
    });
    return comparesQuestBitAt(25, 28, QBIT_WALLS_OF_MIST_PASSED) &&
           comparesQuestBitAt(26, 31, QBIT_BREEDING_ZONE_FINISHED) &&
           setsTextureAt(28, castleHarmondaleAdvisorBannerLeftCog, "wizh-a") &&
           setsTextureAt(29, castleHarmondaleAdvisorBannerRightCog, "wizh-b") &&
           setsTextureAt(31, castleHarmondaleAdvisorBannerLeftCog, "nechuma") &&
           setsTextureAt(32, castleHarmondaleAdvisorBannerRightCog, "nechumb") &&
           !hasAlignmentGate;
}

static bool isOriginalCastleHarmondaleWarBannerEvent(
    const EvtContext &context, std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_CASTLE_HARMONDALE ||
        context.eventId != castleHarmondaleWarBannerEventId || !context.duringMapLoad)
        return false;

    const auto comparesQuestBitAt = [instructions](int step, int targetStep, QuestBit bit) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.target_step == targetStep &&
                   instruction.opcode == EVENT_Compare &&
                   instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
                   instruction.data.variable_descr.value == std::to_underlying(bit);
        });
    };
    const auto setsTextureAt = [instructions](int step, int cog, std::string_view texture) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_SetTexture &&
                   instruction.data.sprite_texture_descr.cog == cog && instruction.str == texture;
        });
    };
    return instructions.size() == 29 &&
           comparesQuestBitAt(1, 9, QBIT_RIVERSTRIDE_PLANS_GIVEN) &&
           comparesQuestBitAt(2, 7, QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN) &&
           comparesQuestBitAt(3, 5, QBIT_FALSE_LOREN_GIVEN) &&
           comparesQuestBitAt(9, 13, QBIT_LOREN_STEEL_RESCUED) &&
           comparesQuestBitAt(13, 24, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER) &&
           comparesQuestBitAt(14, 16, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA) &&
           comparesQuestBitAt(15, 20, QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE) &&
           setsTextureAt(16, castleHarmondaleWarBannerLeftCog, "chb1p6") &&
           setsTextureAt(17, castleHarmondaleWarBannerRightCog, "chb1p7") &&
           setsTextureAt(20, castleHarmondaleWarBannerLeftCog, "elfhuma") &&
           setsTextureAt(21, castleHarmondaleWarBannerRightCog, "elfhumb") &&
           setsTextureAt(24, castleHarmondaleWarBannerLeftCog, "bannwc-a") &&
           setsTextureAt(25, castleHarmondaleWarBannerRightCog, "bannwc-b");
}

static std::optional<int> castleHarmondaleWarBannerScore() {
    const int recipientCount = pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA] +
                               pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE] +
                               pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ARBITER];
    const int plansCount = pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN] +
                           pParty->_questBits[QBIT_FALSE_RIVERSTRIDE_PLANS_GIVEN];
    const int lorenCount = pParty->_questBits[QBIT_LOREN_STEEL_RESCUED] +
                           pParty->_questBits[QBIT_FALSE_LOREN_GIVEN];
    if (recipientCount != 1 || plansCount != 1 || lorenCount != 1)
        return std::nullopt;

    int score = 0;
    score += pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_ERATHIA] ? -1 : 0;
    score += pParty->_questBits[QBIT_GRYPHONHEART_TRUMPET_GIVEN_TO_AVLEE] ? 1 : 0;
    score += pParty->_questBits[QBIT_RIVERSTRIDE_PLANS_GIVEN] ? 1 : -1;
    score += pParty->_questBits[QBIT_LOREN_STEEL_RESCUED] ? -1 : 1;
    return score;
}

static void applyCastleHarmondaleAdvisorBanners() {
    if (pParty->_questBits[QBIT_LIGHT_PATH] && pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]) {
        setTexture(castleHarmondaleAdvisorBannerLeftCog, "wizh-a");
        setTexture(castleHarmondaleAdvisorBannerRightCog, "wizh-b");
    } else if (pParty->_questBits[QBIT_DARK_PATH] &&
               pParty->_questBits[QBIT_BREEDING_ZONE_FINISHED]) {
        setTexture(castleHarmondaleAdvisorBannerLeftCog, "nechuma");
        setTexture(castleHarmondaleAdvisorBannerRightCog, "nechumb");
    }
}

static void applyCastleHarmondaleWarBanners() {
    const std::optional<int> score = castleHarmondaleWarBannerScore();
    if (!score)
        return;

    if (*score < 0) {
        setTexture(castleHarmondaleWarBannerLeftCog, "chb1p6");
        setTexture(castleHarmondaleWarBannerRightCog, "chb1p7");
    } else if (*score > 0) {
        setTexture(castleHarmondaleWarBannerLeftCog, "elfhuma");
        setTexture(castleHarmondaleWarBannerRightCog, "elfhumb");
    } else {
        setTexture(castleHarmondaleWarBannerLeftCog, "bannwc-a");
        setTexture(castleHarmondaleWarBannerRightCog, "bannwc-b");
    }
}

static bool isOriginalCastleGolemEvent(const EvtContext &context,
                                       std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_CASTLE_HARMONDALE ||
        context.eventId != castleHarmondaleGolemEventId)
        return false;

    const auto changesVariableAt = [instructions](int step, int targetStep, EvtOpcode opcode,
                                                   EvtVariable variable, int value) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.target_step == targetStep &&
                   instruction.opcode == opcode &&
                   instruction.data.variable_descr.type == variable &&
                   instruction.data.variable_descr.value == value;
        });
    };
    const auto togglesGroupAt = [instructions](int step, ActorAttribute attribute, bool set) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == EVENT_ToggleActorGroupFlag &&
                   instruction.data.actor_flag_descr.id == 6 &&
                   instruction.data.actor_flag_descr.attr == attribute &&
                   instruction.data.actor_flag_descr.is_set == set;
        });
    };
    const bool hasAnimationGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_GOLEM_ANIMATED);
    });
    const bool hasReloadTrigger = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_OnMapReload;
    });
    const bool hasOriginalJump = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.target_step == 7 && instruction.opcode == EVENT_Jmp;
    });
    const bool hasExit = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 7 && instruction.opcode == EVENT_Exit;
    });
    return instructions.size() == 9 && hasReloadTrigger &&
           changesVariableAt(1, 4, EVENT_Compare, VAR_QBits_QuestsDone,
                             std::to_underlying(QBIT_ABBEY_NORMAL_GOLEM_ASSEMBLED)) &&
           changesVariableAt(2, 5, EVENT_Compare, VAR_QBits_QuestsDone,
                             std::to_underlying(QBIT_GOLEM_ASSEMBLED)) &&
           hasOriginalJump && togglesGroupAt(4, ACTOR_AGGRESSOR, true) &&
           changesVariableAt(5, 0, EVENT_Subtract, VAR_NPCs2, 56) &&
           togglesGroupAt(6, ACTOR_UNKNOW11, false) && hasExit && !hasAnimationGate;
}

static bool isCelesteLadyCarmineKillCheck(const EvtContext &context, const EvtInstruction &instruction) {
    return engine->gameTitle() == GameTitle::MM7 && context.source == EVT_SOURCE_MAP &&
           engine->_currentLoadedMapId == MAP_CELESTE &&
           context.eventId == celesteLadyCarmineMapLeaveEventId &&
           instruction.data.actor_descr.policy == KILL_CHECK_GROUPID &&
           instruction.data.actor_descr.param == 1 && instruction.data.actor_descr.num == 0 &&
           !pActors.empty() && pActors[0].npcId == 92 &&
           pActors[0].monsterInfo.id == MONSTER_PEASANT_HUMAN2_FEMALE_A_C && pActors[0].group == 22;
}

static bool isWineCellarVampireKillCheck(const EvtContext &context, const EvtInstruction &instruction) {
    return engine->gameTitle() == GameTitle::MM7 && context.source == EVT_SOURCE_MAP &&
           engine->_currentLoadedMapId == MAP_WINE_CELLAR &&
           context.eventId == wineCellarKillCheckEventId &&
           instruction.data.actor_descr.policy == KILL_CHECK_ACTORID &&
           instruction.data.actor_descr.param == 0 && instruction.data.actor_descr.num == 0 &&
           !pActors.empty() && pActors[0].monsterInfo.id == MONSTER_VAMPIRE_C;
}

/**
 * @offset 0x448CF4
 */
void spawnMonsters(int16_t typeindex, int16_t level, int count,
                          Vec3f pos, int group, int uUniqueName) {
    if (engine->_currentLoadedMapId == MAP_INVALID || engine->config->debug.NoActors.value())
        return;

    SpawnPoint pSpawnPoint;

    pSpawnPoint.position = pos;
    pSpawnPoint.group = group;
    pSpawnPoint.radius = 32;
    pSpawnPoint.type = OBJECT_Actor;
    pSpawnPoint.monsterIndex = typeindex + 2 * level + level;

    AIDirection direction;
    int oldNumActors = pActors.size();
    SpawnEncounter(&pMapTable->pInfos[engine->_currentLoadedMapId], &pSpawnPoint, 0, count, 0);
    Actor::GetDirectionInfo(pos, pParty->pos + Vec3f(0, 0, pParty->eyeLevel), &direction);
    for (int i = oldNumActors; i < pActors.size(); ++i) {
        pActors[i].PrepareSprites(0);
        pActors[i].yawAngle = direction.uYawAngle;
        pActors[i].uniqueNameIndex = uUniqueName;
    }
}

static tl::generator<Character &> iterateCharacters(EvtTargetCharacter who, RandomEngine *rng) {
    if (who >= CHOOSE_PLAYER1 && who <= CHOOSE_PLAYER4) {
        co_yield pParty->pCharacters[std::to_underlying(who)];
    } else if (who == CHOOSE_ACTIVE) {
        if (pParty->hasActiveCharacter()) {
            co_yield pParty->activeCharacter();
        } else {
            co_yield pParty->pCharacters[rng->random(4)]; // Nobody is active while all four characters recover.
        }
    } else if (who == CHOOSE_PARTY) {
        for (Character &player : pParty->pCharacters)
            co_yield player;
    } else {
        assert(who == CHOOSE_RANDOM);
        co_yield pParty->pCharacters[rng->random(4)];
    }
}

/**
 * @param who                           Characters that a variable command targets.
 * @param variable                      Variable of the command.
 * @param rng                           Random engine for `CHOOSE_RANDOM`.
 * @return                              Characters to run the command for. For a party variable this is just the first
 *                                      targeted character, since the whole party shares one value.
 */
static tl::generator<Character &> iterateCharacters(EvtTargetCharacter who, EvtVariable variable, RandomEngine *rng) {
    for (Character &character : iterateCharacters(who, rng)) {
        co_yield character;
        if (isPartyVariable(variable))
            co_return;
    }
}

/**
 * @param ir                            MoveToMap instruction.
 * @return                              Where it sends the party. An all-zero position means the script isn't placing
 *                                      the party itself - on the current map it stays put, which is how the MM6
 *                                      castle doors open their throne rooms without a teleport, and on another map
 *                                      it arrives at Party Start. Yaw, pitch and speed are ignored in that case, and
 *                                      every shipped record with a zero position has them at zero anyway.
 */
static MapDestination moveToMapDestination(const EvtInstruction &ir) {
    const auto &descr = ir.data.move_map_descr;

    MapId map = !ir.str.starts_with('0') && !ir.str.empty() ? pMapTable->GetMapInfo(ir.str) : MAP_INVALID;
    if (descr.x || descr.y || descr.z)
        return MapDestination(map, PartyPlacement(Vec3f(descr.x, descr.y, descr.z),
                                                  descr.yaw != -1 ? (descr.yaw & TrigLUT.uDoublePiMask) : -1, descr.pitch, descr.zspeed));
    if (map != MAP_INVALID)
        return MapDestination(map, MAP_START_POINT_PARTY); // Shipped MM6 doors name the map and nothing else.
    return {};
}

static BLVFace &mm6OutdoorEventFace(int modelIndex, int facetIndex) {
    if (uCurrentlyLoadedLevelType != LEVEL_OUTDOOR || !pOutdoor ||
        modelIndex < 0 || static_cast<size_t>(modelIndex) >= pOutdoor->pBModels.size())
        throw Exception("MM6 outdoor event has no model {}", modelIndex);
    const auto &modelFaces = pOutdoor->pBModels[modelIndex].faces;
    if (facetIndex < 0 || static_cast<size_t>(facetIndex) >= modelFaces.size())
        throw Exception("MM6 outdoor event model {} has no facet {}", modelIndex, facetIndex);
    const int faceIndex = modelFaces[facetIndex];
    if (faceIndex < 0 || static_cast<size_t>(faceIndex) >= pOutdoor->faces.size())
        throw Exception("MM6 outdoor event has invalid reconstructed face {}", faceIndex);
    return pOutdoor->faces[faceIndex];
}

static BLVFace *mm6IndoorEventFace(uint32_t faceIndex) {
    if (uCurrentlyLoadedLevelType != LEVEL_INDOOR || !pIndoor || faceIndex >= pIndoor->faces.size())
        return nullptr; // The original MM6 handler advances without mutation for invalid targets.
    return &pIndoor->faces[faceIndex];
}

static bool isMM6MapByteVariable(EvtVariable variable) {
    return engine->gameTitle() == GameTitle::MM6 &&
           describeMM6Variable(std::to_underlying(variable)).family == MM6VariableFamily::MapByte;
}

static bool isMM6AutonoteVariable(EvtVariable variable) {
    return engine->gameTitle() == GameTitle::MM6 &&
           describeMM6Variable(std::to_underlying(variable)).family == MM6VariableFamily::Autonote;
}

static bool isMM6SkillVariable(EvtVariable variable) {
    return engine->gameTitle() == GameTitle::MM6 &&
           mm6::isSkillVariable(std::to_underlying(variable));
}

static MM6AutonoteState mm6AutonoteState() {
    return MM6AutonoteState(pParty->_autonoteBits);
}

static MM6MapByteState mm6CurrentMapByteState() {
    if (uCurrentlyLoadedLevelType == LEVEL_INDOOR && pIndoor)
        return MM6MapByteState(pIndoor->mm6DeltaBytes);
    if (uCurrentlyLoadedLevelType == LEVEL_OUTDOOR && pOutdoor)
        return MM6MapByteState(pOutdoor->mm6DeltaBytes);
    throw Exception("MM6 map variable command has no loaded MM6 map delta");
}

constexpr int barrowLanternEventId = 177;
constexpr int barrowLanternChestId = 2;
constexpr int barrowLanternFallbackChestId = 0;
constexpr int barrowLanternMapVariable = 2;

constexpr int hiddenTombRevealEventId = 406;
constexpr int hiddenTombPuzzleMapVariable = 9;
constexpr int hiddenTombWallCog = 16;
constexpr int hiddenTombEntranceCog = 17;

static bool isOriginalBarrowLanternEvent(const EvtContext &context,
                                         std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_BARROW_II || context.eventId != barrowLanternEventId)
        return false;

    const bool hasHint = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == -1 && instruction.opcode == EVENT_MouseOver &&
               instruction.data.text_id == 3;
    });
    const bool hasAwardGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_Award &&
               instruction.data.variable_descr.value ==
                   std::to_underlying(AWARD_HARMONDALE_RETURNED_LANTERN_OF_LIGHT);
    });
    const bool hasLanternChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == barrowLanternChestId;
    });
    const bool hasExit = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.opcode == EVENT_Exit;
    });
    const bool hasFallbackChest = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_OpenChest &&
               instruction.data.chest_id == barrowLanternFallbackChestId;
    });
    const bool hasMapVariableInstruction = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Compare || instruction.opcode == EVENT_Set ||
                instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type ==
                   static_cast<EvtVariable>(std::to_underlying(VAR_MapPersistentVariable_0) +
                                            barrowLanternMapVariable);
    });
    return instructions.size() == 5 && hasHint && hasAwardGate && hasLanternChest && hasExit &&
           hasFallbackChest && !hasMapVariableInstruction;
}

static bool isOriginalHiddenTombReloadEvent(const EvtContext &context,
                                            std::span<const EvtInstruction> instructions) {
    if (engine->gameTitle() != GameTitle::MM7 || context.source != EVT_SOURCE_MAP ||
        engine->_currentLoadedMapId != MAP_ERATHIA || context.eventId != 1 || !context.duringMapLoad ||
        instructions.size() != 6 || !engine->_localEventMap.hasEvent(hiddenTombRevealEventId))
        return false;

    const bool hasReloadTrigger = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.opcode == EVENT_OnMapReload;
    });
    const bool hasErathiaPortalBit = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_TOWN_PORTAL_TO_ERATHIA);
    });

    const auto &reveal = engine->_localEventMap.function(hiddenTombRevealEventId);
    const EvtVariable mapVariable = static_cast<EvtVariable>(
        std::to_underlying(VAR_MapPersistentVariable_0) + hiddenTombPuzzleMapVariable);
    const bool hasSolvedGate = std::ranges::any_of(reveal, [mapVariable](const EvtInstruction &instruction) {
        return instruction.step == 0 && instruction.target_step == 13 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == mapVariable && instruction.data.variable_descr.value == 5;
    });
    const bool setsCodeBit = std::ranges::any_of(reveal, [](const EvtInstruction &instruction) {
        return instruction.step == 6 && instruction.opcode == EVENT_Set &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_HIDDEN_TOMB_CODE_ENTERED);
    });
    const bool hidesWall = std::ranges::any_of(reveal, [](const EvtInstruction &instruction) {
        return instruction.step == 9 && instruction.opcode == EVENT_SetFacesBit &&
               instruction.data.faces_bit_descr.cog == hiddenTombWallCog &&
               instruction.data.faces_bit_descr.face_bit == FACE_IsInvisible && instruction.data.faces_bit_descr.is_on;
    });
    const bool opensWall = std::ranges::any_of(reveal, [](const EvtInstruction &instruction) {
        return instruction.step == 7 && instruction.opcode == EVENT_SetFacesBit &&
               instruction.data.faces_bit_descr.cog == hiddenTombWallCog &&
               instruction.data.faces_bit_descr.face_bit == FACE_ETHEREAL && instruction.data.faces_bit_descr.is_on;
    });
    const bool showsEntrance = std::ranges::any_of(reveal, [](const EvtInstruction &instruction) {
        return instruction.step == 8 && instruction.opcode == EVENT_SetFacesBit &&
               instruction.data.faces_bit_descr.cog == hiddenTombEntranceCog &&
               instruction.data.faces_bit_descr.face_bit == FACE_IsInvisible && !instruction.data.faces_bit_descr.is_on;
    });
    return hasReloadTrigger && hasErathiaPortalBit && reveal.size() == 15 && hasSolvedGate && setsCodeBit &&
           hidesWall && opensWall && showsEntrance;
}

void EvtInterpreter::prepare(EvtContext context) {
    _context = context;

    const EvtProgram &program = context.source == EVT_SOURCE_GLOBAL ? engine->_globalEventMap : engine->_localEventMap;
    program.dump(context.eventId, context.source);

    _events.clear();
    if (program.hasEvent(context.eventId))
        _events = program.function(context.eventId);
}

EvtTransition EvtInterpreter::execute(int startStep) {
    assert(startStep >= 0);

    if (!_context.eventId || !_events.size())
        return EVT_TRANSITION_NONE;

    // Castle Harmondale's golem reload event omits the animation quest-bit gate. Without it, merely assembling the
    // golem changes the dormant map actor and dismisses the hired golem before Thomas Grey animates it.
    if (isOriginalCastleGolemEvent(_context, _events) &&
        !pParty->_questBits[QBIT_GOLEM_ANIMATED])
        return EVT_TRANSITION_NONE;

    const bool correctCastleAdvisorBanners =
        isOriginalCastleHarmondaleAdvisorBannerEvent(_context, _events);
    const bool correctCastleWarBanners =
        isOriginalCastleHarmondaleWarBannerEvent(_context, _events);

    // The original game preserves the entrance revealed by Erathia's five-part Hidden Tomb puzzle. OpenEnroth
    // reloads the base face flags while retaining the solved quest bit, so the entrance is hidden again.
    // Reapply only the exact original reveal records; patched or custom event data remains authoritative.
    if (pParty->_questBits[QBIT_HIDDEN_TOMB_CODE_ENTERED] && isOriginalHiddenTombReloadEvent(_context, _events)) {
        setFacesBit(hiddenTombWallCog, FACE_ETHEREAL, true);
        setFacesBit(hiddenTombEntranceCog, FACE_IsInvisible, false);
        setFacesBit(hiddenTombWallCog, FACE_IsInvisible, true);
    }

    int step = startStep;

    _who = !pParty->hasActiveCharacter() ? CHOOSE_RANDOM : CHOOSE_ACTIVE;

    while (step != -1)
        step = executeStep(step);

    if (correctCastleAdvisorBanners)
        applyCastleHarmondaleAdvisorBanners();
    if (correctCastleWarBanners)
        applyCastleHarmondaleWarBanners();

    return _transition;
}

bool EvtInterpreter::canShowTopic() {
    if (!_context.eventId)
        return false;

    int step = 0;
    while (step != -1)
        step = checkTopicStep(step);

    return _canShowTopic;
}

bool EvtInterpreter::isValid() {
    return _events.size() > 0;
}

// TODO(captainurist): take a const reference once the MoveToMap data hacks below stop patching `ir`.
EvtFlow EvtInterpreter::executeInstruction(EvtInstruction ir) {
    switch (ir.opcode) {
        case EVENT_Exit:
            return {EVT_FLOW_STOP};
        case EVENT_SpeakInHouse:
            if (enterHouse(ir.data.house_id)) {
                if (engine->gameTitle() != GameTitle::MM6)
                    pAudioPlayer->playHouseSound(SOUND_enter, false);
                HouseId houseId = HOUSE_JAIL;
                if (uCurrentHouse_Animation != 167) { // TODO: magic number
                    houseId = ir.data.house_id;
                }
                createHouseUI(houseId);
            }
            break;
        case EVENT_PlaySound:
            // TODO(captainurist): ir.data.sound_descr.x, ir.data.sound_descr.y used to be passed in.
            pAudioPlayer->playSound(ir.data.sound_descr.sound_id, SOUND_MODE_UI);
            break;
        case EVENT_MouseOver:
        case EVENT_LocationName:
            assert(false); // EvtInstruction::parse gives hints step -1, so the lookup by step above never returns one.
            break;
        case EVENT_MoveToMap:
        {
            if (ir.data.move_map_descr.house_id != HOUSE_INVALID || ir.data.move_map_descr.exit_pic_id) {
                // TODO(pskelton): Fix #1890 this should be a data mod
                if (engine->_indoor->filename == "d20.blv" && _context.eventId == 501)
                    ir.data.move_map_descr.z = 3088;

                pDialogueWindow = std::make_unique<GUIWindow_IndoorEntryExit>(ir.data.move_map_descr.house_id, ir.data.move_map_descr.exit_pic_id,
                                                                             moveToMapDestination(ir), ir.str, continuation(ir.step + 1));
                return {EVT_FLOW_YIELD};
            }

            // TODO(pskelton): Fix #2117 this should be a data mod
            if (engine->_indoor->filename == "d25.blv" && _context.eventId == 451 && ir.step == 1)
                ir.str = "out06.odm";

            // TODO(pskelton): Fix #2117 this should be a data mod - the RandomGoTo targets fall through into each
            //                 other, only the first one should run.
            if (engine->_indoor->filename == "d25.blv" && _context.eventId == 451 && engine->_pendingTransition)
                break;

            MapDestination destination = moveToMapDestination(ir);
            if (destination.map() == MAP_INVALID) { // teleport within map
                if (std::optional<PartyPlacement> placement = destination.resolvePlacement()) {
                    placeParty(*placement);
                    pAudioPlayer->playUISound(SOUND_teleport);
                }
                break;
            }

            pGameLoadingUI_ProgressBar->Initialize(_context.source == EVT_SOURCE_GLOBAL ? GUIProgressBar::TYPE_Fullscreen : GUIProgressBar::TYPE_Box);
            startMapTransition(destination);

            // TODO(captainurist): this should be a data mod. The Emerald Island ship sails to Harmondale for a week, and
            //                     the week passes after the autosave that startMapTransition writes.
            if (engine->gameTitle() == GameTitle::MM7 && engine->_currentLoadedMapId == MAP_EMERALD_ISLAND &&
                destination.map() == MAP_HARMONDALE) {
                Rest(Duration::fromDays(7));
                pParty->restAndHeal();
                pParty->days_played_without_rest = 0;
            }

            _transition = EVT_TRANSITION_STARTED;
            if (current_screen_type == SCREEN_HOUSE) {
                if (uGameState == GAME_STATE_CHANGE_LOCATION) {
                    houseDialogueGoBackToTop();
                    pMediaPlayer->Unload();
                    window_SpeakInHouse = nullptr;
                    engine->_messageQueue->clear();
                    current_screen_type = SCREEN_GAME;
                    pDialogueWindow = nullptr;
                }
                return {EVT_FLOW_STOP};
            }
            break;
        }
        case EVENT_OpenChest:
            if (!Chest::open(ir.data.chest_id, _context.objectPid))
                return {EVT_FLOW_STOP};
            if (isOriginalBreedingZoneBookEvent(_context, _events) &&
                ir.data.chest_id == breedingZoneBookChestId)
                engine->_persistentVariables.mapVars[0] = 1;
            if (isOriginalTidewaterMapEvent(_context, _events) &&
                ir.data.chest_id == tidewaterMapChestId)
                engine->_persistentVariables.mapVars[tidewaterMapVariable] = 1;
            if (isOriginalTitansPerfectBowEvent(_context, _events) &&
                ir.data.chest_id == titansPerfectBowChestId) {
                pParty->_questBits.set(QBIT_PERFECT_BOW_TAKEN);
                engine->_persistentVariables.mapVars[titansPerfectBowMapVariable] = 1;
            }
            if (isOriginalMazeHaldarsRemainsEvent(_context, _events) &&
                ir.data.chest_id == mazeHaldarsRemainsChestId)
                engine->_persistentVariables.mapVars[mazeHaldarsRemainsMapVariable] = 1;
            if (isOriginalBarrowLanternEvent(_context, _events) &&
                ir.data.chest_id == barrowLanternChestId)
                engine->_persistentVariables.mapVars[barrowLanternMapVariable] = 1;
            break;
        case EVENT_ShowFace:
            for (Character &character : iterateCharacters(ir.who, vrng))
                character.playEmotion(ir.data.portrait_id, 0_ticks);
            break;
        case EVENT_ReceiveDamage:
            for (Character &character : iterateCharacters(ir.who, grng))
                character.receiveDamage(ir.data.damage_descr.damage, ir.data.damage_descr.damage_type);
            break;
        case EVENT_SetSnow:
            if (!ir.data.snow_descr.is_nop) {
                pWeather->bRenderSnow = ir.data.snow_descr.is_enable;
            }
            break;
        case EVENT_SetTexture:
            // The original Castle Harmondale map-load trees choose these banners with incomplete conditions. Suppress
            // their texture writes, then apply the complete alignment/war decision after their other side effects run.
            if (isOriginalCastleHarmondaleAdvisorBannerEvent(_context, _events) &&
                ir.step >= 28 && ir.step <= 32)
                break;
            if (isOriginalCastleHarmondaleWarBannerEvent(_context, _events))
                break;
            if (engine->gameTitle() == GameTitle::MM6) {
                if (BLVFace *face = mm6IndoorEventFace(ir.data.sprite_texture_descr.cog))
                    face->SetTexture(ir.str);
            } else {
                setTexture(ir.data.sprite_texture_descr.cog, ir.str);
            }
            break;
        case EVENT_ShowMovie:
        {
            if (engine->gameTitle() == GameTitle::MM6) {
                mm6OutdoorEventFace(ir.data.mm6_outdoor_texture_descr.model,
                                    ir.data.mm6_outdoor_texture_descr.facet).SetTexture(ir.str);
                break;
            }
            std::string movieName = trimRemoveQuotes(ir.str);
            if (movieName.length() == 0) {
                break;
            }
            if (pMediaPlayer->IsMoviePlaying()) {
                pMediaPlayer->Unload();
            }

            // Restore the screen type after the movie has played in case we are already in a house
            auto saveScreenType = current_screen_type;
            pMediaPlayer->PlayFullscreenMovie(movieName);
            current_screen_type = saveScreenType;

            if (!movieName.compare("arbiter good")) { // change alignment to good
                pParty->alignment = PartyAlignment::PartyAlignment_Good;
                SetUserInterface(pParty->alignment);
            } else if (!movieName.compare("arbiter evil")) { // change alignment to evil
                pParty->alignment = PartyAlignment::PartyAlignment_Evil;
                SetUserInterface(pParty->alignment);
            }

            // is this block is needed anymore?
            if (!ir.data.movie_unknown_field || current_screen_type == SCREEN_BOOKS) {
                if (current_screen_type == SCREEN_BOOKS) {
                    pGameLoadingUI_ProgressBar->Initialize(GUIProgressBar::TYPE_Fullscreen);
                }

                if (current_screen_type == SCREEN_HOUSE) {
                    pMediaPlayer->OpenHouseMovie(pAnimatedRooms[uCurrentHouse_Animation].video_name, 1);
                }
            }
            break;
        }
        case EVENT_SetSprite:
            if (engine->gameTitle() == GameTitle::MM6) {
                std::optional<DecorationId> replacement;
                if (!ir.str.empty() && ir.str != "0")
                    replacement = pDecorationTable->decorationId(ir.str);
                mm6::applySetSprite(engine->gameTitle(), pLevelDecorations,
                                    ir.data.sprite_texture_descr.cog,
                                    ir.data.sprite_texture_descr.hide, replacement);
            } else {
                setDecorationSprite(ir.data.sprite_texture_descr.cog, ir.data.sprite_texture_descr.hide, ir.str);
            }
            break;
        case EVENT_Compare:
            if (isOriginalTidewaterMapEvent(_context, _events) && ir.step == 0) {
                int chestId = -1;
                if (engine->_persistentVariables.mapVars[tidewaterMapVariable] >= 1) {
                    chestId = tidewaterMapChestId;
                } else if (pParty->_questBits[QBIT_ORACLE_RECOVERS_EVENMORN_MAP]) {
                    chestId = tidewaterFallbackChestId;
                }
                if (chestId >= 0) {
                    auto chest = std::ranges::find_if(_events, [chestId](const EvtInstruction &instruction) {
                        return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
                    });
                    return {EVT_FLOW_JUMP, chest->step};
                }
            }
            if (isOriginalGiantsWishWellEvent(_context, _events) && ir.step == 0 &&
                pParty->GetGold() >= 5000) {
                pParty->TakeGold(5000);
                return {EVT_FLOW_JUMP, ir.target_step};
            }
            if (isOriginalMazeHaldarsRemainsEvent(_context, _events) && ir.step == 0 &&
                engine->_persistentVariables.mapVars[mazeHaldarsRemainsMapVariable] >= 1) {
                auto chest = std::ranges::find_if(_events, [](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest &&
                           instruction.data.chest_id == mazeHaldarsRemainsChestId;
                });
                return {EVT_FLOW_JUMP, chest->step};
            }
            if (isOriginalTitansPerfectBowEvent(_context, _events) && ir.step == 0) {
                const bool useBowChest = engine->_persistentVariables.mapVars[titansPerfectBowMapVariable] != 0 ||
                                         !pParty->_questBits[QBIT_PERFECT_BOW_TAKEN] &&
                                             (pParty->_questBits[QBIT_PERFECT_BOW_LIGHT_ACTIVE] ||
                                              pParty->_questBits[QBIT_PERFECT_BOW_DARK_ACTIVE]);
                const int chestId = useBowChest ? titansPerfectBowChestId : titansPerfectBowFallbackChestId;
                auto chest = std::ranges::find_if(_events, [chestId](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
                });
                return {EVT_FLOW_JUMP, chest->step};
            }
            if (isOriginalWallsLichJarEvent(_context, _events) &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone &&
                ir.data.variable_descr.value == std::to_underlying(QBIT_LICH_JARS_ACTIVE) &&
                engine->_persistentVariables.decorVars[wallsLichJarDecorVariable] == 1) {
                auto chest = std::ranges::find_if(_events, [](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest &&
                           instruction.data.chest_id == wallsLichJarChestId;
                });
                return {EVT_FLOW_JUMP, chest->step};
            }
            if (isOriginalWallsLichJarEvent(_context, _events) &&
                ir.data.variable_descr.type == VAR_Award &&
                ir.data.variable_descr.value == std::to_underlying(AWARD_THE_PIT_RETRIEVED_SOUL_JARS)) {
                const int chestId = engine->_persistentVariables.decorVars[wallsLichJarDecorVariable] == 1 ||
                                            pParty->_questBits[QBIT_LICH_JARS_ACTIVE] ?
                                        wallsLichJarChestId : wallsLichJarFallbackChestId;
                auto chest = std::ranges::find_if(_events, [chestId](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest && instruction.data.chest_id == chestId;
                });
                return {EVT_FLOW_JUMP, chest->step};
            }
            if (isOriginalBreedingZoneBookEvent(_context, _events) && ir.step == 0 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone &&
                ir.data.variable_descr.value == std::to_underlying(QBIT_DIVINE_INTERVENTION_RETRIEVED) &&
                pParty->_questBits[QBIT_DIVINE_INTERVENTION_RETRIEVED] &&
                engine->_persistentVariables.mapVars[0] >= 1) {
                auto sourceChest = std::ranges::find_if(_events, [](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest &&
                           instruction.data.chest_id == breedingZoneBookChestId;
                });
                return {EVT_FLOW_JUMP, sourceChest->step};
            }
            if (isOriginalBarrowLanternEvent(_context, _events) && ir.step == 0 &&
                engine->_persistentVariables.mapVars[barrowLanternMapVariable] >= 1) {
                auto chest = std::ranges::find_if(_events, [](const EvtInstruction &instruction) {
                    return instruction.opcode == EVENT_OpenChest &&
                           instruction.data.chest_id == barrowLanternChestId;
                });
                return {EVT_FLOW_JUMP, chest->step};
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_PlayerItemInHands) {
                for (Character &character : iterateCharacters(_who, grng))
                    if (mm6::hasEventItem(character, pParty->pPickedItem, 17,
                                          ir.data.variable_descr.value))
                        return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_FixedGold) {
                if (mm6::hasEventGold(pParty->GetGold(), ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_DayOfYear) {
                if (mm6::compareDayOfYear(pParty->GetPlayingTime(), 19, ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                std::to_underlying(ir.data.variable_descr.type) == 226) {
                if (mm6::compareMonthOfYear(pParty->uCurrentMonth, 226, ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                std::to_underlying(ir.data.variable_descr.type) == 222) {
                if (mm6::comparePartyFlying(222, ir.data.variable_descr.value, pParty->bFlying,
                                            pParty->pPartyBuffs[PARTY_BUFF_FLY].Active()))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                std::to_underlying(ir.data.variable_descr.type) == 213) {
                for (Character &character : iterateCharacters(_who, grng))
                    if (mm6::CharacterEventBitState(character._characterEventBits).compare(
                            213, ir.data.variable_descr.value))
                        return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone) {
                if (MM6QuestBitState(pParty->_questBits).compare(16, ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 && mm6::isPartyEventVariable(std::to_underlying(ir.data.variable_descr.type))) {
                if (mm6::comparePartyEventVariable(std::to_underlying(ir.data.variable_descr.type), ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                mm6::isCharacterEventVariable(EVENT_Compare, std::to_underlying(ir.data.variable_descr.type))) {
                for (Character &character : iterateCharacters(_who, grng))
                    if (mm6::compareCharacterEventVariable(character, std::to_underlying(ir.data.variable_descr.type),
                                                           ir.data.variable_descr.value))
                        return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (isMM6SkillVariable(ir.data.variable_descr.type)) {
                for (Character &character : iterateCharacters(_who, grng))
                    if (mm6::compareEventSkill(character, std::to_underlying(ir.data.variable_descr.type),
                                               ir.data.variable_descr.value, GameTitle::MM6))
                        return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (isMM6AutonoteVariable(ir.data.variable_descr.type)) {
                if (mm6AutonoteState().compare(std::to_underlying(ir.data.variable_descr.type),
                                               ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (isMM6MapByteVariable(ir.data.variable_descr.type)) {
                if (mm6CurrentMapByteState().compare(std::to_underlying(ir.data.variable_descr.type),
                                                     ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            // Leda Rowan's Champion return script checks only Knight wins, but the original compatibility behavior
            // accepts five combined Knight and Lord victories. Keep this correction scoped to that exact comparison.
            if (engine->gameTitle() == GameTitle::MM7 && _context.source == EVT_SOURCE_GLOBAL &&
                _context.eventId == 72 && ir.step == 0 &&
                ir.data.variable_descr.type == VAR_ArenaWinsKnight && ir.data.variable_descr.value == 5) {
                if (pParty->uNumArenaWins[ARENA_LEVEL_KNIGHT] +
                    pParty->uNumArenaWins[ARENA_LEVEL_LORD] >= ir.data.variable_descr.value)
                    return {EVT_FLOW_JUMP, ir.target_step};
                break;
            }
            if (!validateVariableValue(ir))
                break;
            for (Character &character : iterateCharacters(_who, ir.data.variable_descr.type, grng))
                if (compareEvtVariable(character, ir.data.variable_descr.type, ir.data.variable_descr.value))
                    return {EVT_FLOW_JUMP, ir.target_step};
            break;
        case EVENT_ChangeDoorState:
            switchDoorAnimation(ir.data.door_descr.door_id, ir.data.door_descr.door_action);
            break;
        case EVENT_Add:
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_PlayerItemInHands) {
                if (_who == CHOOSE_PARTY)
                    throw Exception("MM6 party-selected item reward needs repeated-grant preflight");
                Item item = mm6::makeEventItem(17, ir.data.variable_descr.value);
                if (pParty->pPickedItem.itemId == ITEM_NULL) {
                    pParty->setHoldingItem(item);
                } else {
                    std::array<Character *, 4> party;
                    for (int i = 0; i < 4; ++i)
                        party[i] = &pParty->pCharacters[i];
                    Character *recipient = mm6::planEventItemRelocation(
                        party, pParty->hasActiveCharacter() ? pParty->activeCharacterIndex() : -1,
                        pParty->pPickedItem);
                    Item previous = pParty->pPickedItem;
                    Pointi previousOffset = mouse->pickedItemOffset;
                    InventoryEntry stored = mm6::relocateEventItemToInventory(*recipient, pParty->pPickedItem);
                    try {
                        pParty->setHoldingItem(item);
                    } catch (...) {
                        pParty->pPickedItem = previous;
                        recipient->inventory.take(stored);
                        // Icon loading may fail after setHoldingItem has changed its name.
                        // Restoring the already displayed item is best effort only.
                        try {
                            mouse->SetCursorBitmapFromItemID(previous.itemId);
                        } catch (...) {
                        }
                        mouse->pickedItemOffset = previousOffset;
                        throw;
                    }
                }
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_FixedGold) {
                const int balance = mm6::goldAfterEventAdd(pParty->GetGold(), ir.data.variable_descr.value);
                pParty->SetGold(balance, true);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone) {
                MM6QuestBitState(pParty->_questBits).add(16, ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 && mm6::isPartyEventVariable(std::to_underlying(ir.data.variable_descr.type))) {
                mm6::writePartyEventVariable(EVENT_Add, std::to_underlying(ir.data.variable_descr.type), ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                mm6::isCharacterEventVariable(EVENT_Add, std::to_underlying(ir.data.variable_descr.type))) {
                for (Character &character : iterateCharacters(_who, grng))
                    mm6::writeCharacterEventVariable(character, EVENT_Add, std::to_underlying(ir.data.variable_descr.type),
                                                     ir.data.variable_descr.value);
                break;
            }
            if (isMM6AutonoteVariable(ir.data.variable_descr.type)) {
                mm6AutonoteState().add(std::to_underlying(ir.data.variable_descr.type),
                                       ir.data.variable_descr.value);
                break;
            }
            if (isMM6MapByteVariable(ir.data.variable_descr.type)) {
                mm6CurrentMapByteState().add(std::to_underlying(ir.data.variable_descr.type),
                                             ir.data.variable_descr.value);
                break;
            }
            if (!validateVariableValue(ir))
                break;
            // TODO(captainurist): move this workaround into patched event data, and add the OnMapReload step from
            //                     GrayFace's d27.evt that re-applies the empty cage sprite once the quest bit is set.
            //                     The sprite isn't saved, so after a reload the cage shows Roland until the next click.
            if (engine->gameTitle() == GameTitle::MM7 && engine->_currentLoadedMapId == MAP_COLONY_ZOD && _context.eventId == 376 &&
                ir.data.variable_descr.type == VAR_PlayerItemInHands && pParty->_questBits[QBIT_TALKED_TO_ROLAND])
                break; // Roland's cage script adds the key on every click, it never checks the quest bit.
            for (Character &character : iterateCharacters(_who, ir.data.variable_descr.type, grng))
                addEvtVariable(character, ir.data.variable_descr.type, ir.data.variable_descr.value);
            break;
        case EVENT_Subtract: {
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_PlayerItemInHands) {
                std::vector<Character *> selected;
                for (Character &character : iterateCharacters(_who, grng))
                    selected.push_back(&character);
                std::vector<mm6::ItemRemovalStep> plan = mm6::planEventItemRemoval(
                    selected, pParty->pPickedItem, 17, ir.data.variable_descr.value);
                for (const mm6::ItemRemovalStep &step : plan) {
                    if (step.inventoryEntry)
                        step.character->inventory.take(step.inventoryEntry);
                    else if (step.held)
                        pParty->takeHoldingItem();
                }
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_FixedGold) {
                std::optional<int> balance =
                    mm6::goldAfterEventSubtract(pParty->GetGold(), ir.data.variable_descr.value);
                if (!balance)
                    return {EVT_FLOW_STOP};
                pParty->SetGold(*balance, true);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone) {
                MM6QuestBitState(pParty->_questBits).subtract(16, ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 && mm6::isPartyEventVariable(std::to_underlying(ir.data.variable_descr.type))) {
                mm6::writePartyEventVariable(EVENT_Subtract, std::to_underlying(ir.data.variable_descr.type), ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                mm6::isCharacterEventVariable(EVENT_Subtract, std::to_underlying(ir.data.variable_descr.type))) {
                for (Character &character : iterateCharacters(_who, grng))
                    mm6::writeCharacterEventVariable(character, EVENT_Subtract, std::to_underlying(ir.data.variable_descr.type),
                                                     ir.data.variable_descr.value);
                break;
            }
            if (isMM6AutonoteVariable(ir.data.variable_descr.type)) {
                mm6AutonoteState().subtract(std::to_underlying(ir.data.variable_descr.type),
                                            ir.data.variable_descr.value);
                break;
            }
            if (isMM6MapByteVariable(ir.data.variable_descr.type)) {
                mm6CurrentMapByteState().subtract(std::to_underlying(ir.data.variable_descr.type),
                                                  ir.data.variable_descr.value);
                break;
            }
            if (!validateVariableValue(ir))
                break;
            bool isShort = false; // The party had less than the script asked for, e.g. gold.
            // We had a couple issues with quest items not being removed from inventory, and the reason was that the
            // character target wasn't properly set in the script. Thus, we don't even check `_who` here and just try
            // to take the item from all characters. See issues #1808 and #1912.
            if (ir.data.variable_descr.type == VAR_PlayerItemInHands/* && (_who == CHOOSE_PARTY || _who == CHOOSE_ACTIVE)*/) {
                ItemId itemId = static_cast<ItemId>(ir.data.variable_descr.value);
                for (Character &character : pParty->pCharacters) {
                    if (pParty->pPickedItem.itemId == itemId || character.inventory.find(itemId)) {
                        if (!subtractEvtVariable(character, ir.data.variable_descr.type, ir.data.variable_descr.value))
                            isShort = true;
                        break;  // Only take one item.
                    }
                }
            } else {
                for (Character &character : iterateCharacters(_who, ir.data.variable_descr.type, grng))
                    if (!subtractEvtVariable(character, ir.data.variable_descr.type, ir.data.variable_descr.value))
                        isShort = true;
            }
            if (isShort)
                return {EVT_FLOW_STOP};
            break;
        }
        case EVENT_Set:
            if (ir.step == 1 && isOriginalDeyjaGuardSetupEvent(_context, _events))
                applyDeyjaGuardFaction();
            if (engine->gameTitle() == GameTitle::MM6 &&
                std::to_underlying(ir.data.variable_descr.type) == 213) {
                if (ir.data.variable_descr.value < 0 || ir.data.variable_descr.value >= 512)
                    throw Exception("MM6 character event bit operand is invalid");
                for (Character &character : iterateCharacters(_who, grng))
                    mm6::CharacterEventBitState(character._characterEventBits).set(
                        213, ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_PlayerItemInHands)
                throw Exception("MM6 item Set is absent from retained scripts");
            if (engine->gameTitle() == GameTitle::MM6 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone) {
                MM6QuestBitState(pParty->_questBits).set(16, ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 && mm6::isPartyEventVariable(std::to_underlying(ir.data.variable_descr.type))) {
                mm6::writePartyEventVariable(EVENT_Set, std::to_underlying(ir.data.variable_descr.type), ir.data.variable_descr.value);
                break;
            }
            if (engine->gameTitle() == GameTitle::MM6 &&
                mm6::isCharacterEventVariable(EVENT_Set, std::to_underlying(ir.data.variable_descr.type))) {
                for (Character &character : iterateCharacters(_who, grng))
                    mm6::writeCharacterEventVariable(character, EVENT_Set, std::to_underlying(ir.data.variable_descr.type),
                                                     ir.data.variable_descr.value);
                break;
            }
            if (isMM6SkillVariable(ir.data.variable_descr.type)) {
                for (Character &character : iterateCharacters(_who, grng))
                    mm6::writeEventSkill(character, std::to_underlying(ir.data.variable_descr.type), ir.data.variable_descr.value);
                break;
            }
            if (isMM6AutonoteVariable(ir.data.variable_descr.type)) {
                mm6AutonoteState().set(std::to_underlying(ir.data.variable_descr.type),
                                       ir.data.variable_descr.value);
                break;
            }
            if (isMM6MapByteVariable(ir.data.variable_descr.type)) {
                mm6CurrentMapByteState().set(std::to_underlying(ir.data.variable_descr.type),
                                             ir.data.variable_descr.value);
                break;
            }
            // Wine Cellar event 501 writes the quest's vampire flag unconditionally. The map-leave actor check
            // below is the genuine completion predicate, so retain the exit but do not turn it into a free kill.
            if (engine->_indoor->filename == "d16.blv" && _context.eventId == 501 &&
                ir.data.variable_descr.type == VAR_QBits_QuestsDone &&
                ir.data.variable_descr.value == std::to_underlying(QBIT_WINE_CELLAR_VAMPIRE_KILLED))
                break;
            if (!validateVariableValue(ir))
                break;
            for (Character &character : iterateCharacters(_who, ir.data.variable_descr.type, grng))
                setEvtVariable(character, ir.data.variable_descr.type, ir.data.variable_descr.value);
            break;
        case EVENT_SummonMonsters:
            if (engine->gameTitle() == GameTitle::MM6) {
                // MM6 carries only the map encounter slot, A/B/C tier, count
                // and position. Group and unique name are MM7-only operands.
                if (ir.data.monster_descr.type < 1 || ir.data.monster_descr.type > 3 ||
                    ir.data.monster_descr.level < 1 || ir.data.monster_descr.level > 3)
                    throw Exception("MM6 monster spawn has invalid encounter slot or tier");
                spawnMonsters(ir.data.monster_descr.type, ir.data.monster_descr.level,
                              ir.data.monster_descr.count,
                              Vec3f(ir.data.monster_descr.x, ir.data.monster_descr.y, ir.data.monster_descr.z),
                              0, 0);
                break;
            }
            spawnMonsters(ir.data.monster_descr.type, ir.data.monster_descr.level, ir.data.monster_descr.count,
                          Vec3f(ir.data.monster_descr.x, ir.data.monster_descr.y, ir.data.monster_descr.z),
                          ir.data.monster_descr.group, ir.data.monster_descr.name_id);
            break;
        case EVENT_CastSpell:
            eventCastSpell(ir.data.spell_descr.spell_id, ir.data.spell_descr.spell_mastery, ir.data.spell_descr.spell_level,
                         Vec3f(ir.data.spell_descr.fromx, ir.data.spell_descr.fromy, ir.data.spell_descr.fromz),
                         Vec3f(ir.data.spell_descr.tox, ir.data.spell_descr.toy, ir.data.spell_descr.toz));
            break;
        case EVENT_SpeakNPC:
            if (!_context.duringMapLoad) {
                // TODO(pskeltonm): Fix #2223 stop tutorial message spam - should be data mod
                if (engine->gameTitle() == GameTitle::MM7 && engine->_currentLoadedMapId == MAP_EMERALD_ISLAND &&
                    _context.eventId >= 200 && _context.eventId <= 218) {
                    if (engine->_OE_transientVariables[_context.eventId - 200])
                        break;
                    engine->_OE_transientVariables[_context.eventId - 200] = 1;
                }

                initializeNPCDialogue(ir.data.npc_descr.npc_id, false);
            } else {
                bDialogueUI_InitializeActor_NPC_ID = ir.data.npc_descr.npc_id;
            }
            break;
        case EVENT_SetFacesBit:
            if (engine->gameTitle() == GameTitle::MM6) {
                if (BLVFace *face = mm6IndoorEventFace(ir.data.faces_bit_descr.cog)) {
                    if (ir.data.faces_bit_descr.is_on)
                        face->attributes |= ir.data.faces_bit_descr.face_bit;
                    else
                        face->attributes &= ~ir.data.faces_bit_descr.face_bit;
                }
            } else {
                setFacesBit(ir.data.faces_bit_descr.cog, ir.data.faces_bit_descr.face_bit, ir.data.faces_bit_descr.is_on);
            }
            break;
        case EVENT_ToggleActorFlag:
            if (engine->gameTitle() == GameTitle::MM6) {
                const auto &target = ir.data.mm6_outdoor_facet_descr;
                auto apply = [&](int facetIndex) {
                    BLVFace &face = mm6OutdoorEventFace(target.model, facetIndex);
                    const FaceAttribute bit = static_cast<FaceAttribute>(static_cast<uint32_t>(target.bit));
                    if (target.is_on)
                        face.attributes |= bit;
                    else
                        face.attributes &= ~bit;
                };
                if (target.facet == -1) {
                    if (uCurrentlyLoadedLevelType != LEVEL_OUTDOOR || !pOutdoor ||
                        target.model < 0 || static_cast<size_t>(target.model) >= pOutdoor->pBModels.size())
                        throw Exception("MM6 outdoor event has no model {}", target.model);
                    for (size_t i = 0; i < pOutdoor->pBModels[target.model].faces.size(); ++i)
                        apply(i);
                } else {
                    apply(target.facet);
                }
                break;
            }
            Actor::toggleFlag(ir.data.actor_flag_descr.id, ir.data.actor_flag_descr.attr, ir.data.actor_flag_descr.is_set);
            break;
        case EVENT_RandomGoTo:
            return {EVT_FLOW_JUMP, ir.data.random_goto_descr.random_goto[grng->random(ir.data.random_goto_descr.random_goto_len)]};
        case EVENT_InputString:
            if (engine->gameTitle() == GameTitle::MM6) {
                const auto &question = ir.data.mm6_question_descr;
                auto stringAt = [&](int id) -> std::string {
                    if (id < 0 || static_cast<size_t>(id) >= engine->_levelStrings.size())
                        throw Exception("MM6 question string {} is outside the current map", id);
                    return engine->_levelStrings[id];
                };
                startMM6QuestionDialogue({stringAt(question.question), stringAt(question.answer1),
                                          stringAt(question.answer2), continuation(ir.target_step),
                                          continuation(ir.step + 1)});
                return {EVT_FLOW_YIELD};
            }
            // Originally starting step was checked to ensure skipping this command when returning from dialogue.
            // Changed to using "step + 1" to go to next event
            //
            // TODO(Nik-RE-dev): this event is not used in MM7. In GrayFace's data it's called "Question" and must have additional arguments
            // that control where events executions must be continued on correct/incorrect input.
            assert(false);
#if 0
            game_ui_status_bar_event_string = (ir.data.text_id < engine->_levelStrings.size()) ? engine->_levelStrings[ir.data.text_id] : "";
            startBranchlessDialogue(EVENT_InputString, continuation(ir.step + 1));
#endif
            return {EVT_FLOW_STOP};
        case EVENT_MM6_Opcode20:
        case EVENT_MM6_Opcode27:
        case EVENT_MM6_Opcode28:
            if (engine->gameTitle() != GameTitle::MM6 || ir.gameTitle != GameTitle::MM6)
                throw Exception("MM6-only evt opcode {} in another title", std::to_underlying(ir.opcode));
            // The original MM6 dispatcher routes all three opcodes directly to its
            // ordinary next-record path. Their operands remain preserved by the parser.
            break;
        case EVENT_StatusText:
            if (_context.source == EVT_SOURCE_GLOBAL) {
                if (!_context.objectPid) // An NPC topic.
                    current_npc_text = pNPCTopics[ir.data.text_id - 1].pText;
                if (!_context.duringMapLoad)
                    engine->_statusBar->setEvent(pNPCTopics[ir.data.text_id - 1].pText);
            } else {
                // TODO(captainurist): log an error on an out of range string id here, in ShowMessage, in the pNPCTopics
                //                     lookups of both, which don't check at all, and in EvtProgram::hint.
                if (!_context.duringMapLoad)
                    engine->_statusBar->setEvent((ir.data.text_id < engine->_levelStrings.size()) ? engine->_levelStrings[ir.data.text_id] : "");
            }
            break;
        case EVENT_ShowMessage:
            branchless_dialogue_str.clear();
            if (_context.source == EVT_SOURCE_GLOBAL) {
                int textId = ir.data.text_id;
                // Kastore's shipped one-piece branches point at each other's responses. Match the complete original
                // event signature before applying GrayFace's two-reference correction, leaving patched or custom
                // global data authoritative.
                if (isOriginalKastoreAltarPiecesEvent(_context, _events)) {
                    if (ir.step == 5)
                        textId = kastoreHasDarkPieceTextId;
                    else if (ir.step == 8)
                        textId = kastoreNeedsDarkPieceTextId;
                }
                current_npc_text = pNPCTopics[textId - 1].pText;
            } else if (ir.data.text_id < engine->_levelStrings.size()) {
                branchless_dialogue_str = engine->_levelStrings[ir.data.text_id];
            }
            break;
        case EVENT_OnTimer:
            // Trigger, must be skipped but can be encountered in vanilla
            return {EVT_FLOW_STOP};
        case EVENT_ToggleIndoorLight:
            pIndoor->toggleLight(ir.data.light_descr.light_id, ir.data.light_descr.is_enable);
            break;
        case EVENT_PressAnyKey:
            startBranchlessDialogue(EVENT_PressAnyKey, continuation(ir.step + 1));
            return {EVT_FLOW_YIELD};
        case EVENT_SummonItem:
            SpriteObject::dropItemAt(ir.data.summon_item_descr.sprite, Vec3f(ir.data.summon_item_descr.x, ir.data.summon_item_descr.y, ir.data.summon_item_descr.z),
                                     ir.data.summon_item_descr.speed, ir.data.summon_item_descr.count, ir.data.summon_item_descr.random_rotate);
            break;
        case EVENT_ForPartyMember:
            _who = ir.who;
            break;
        case EVENT_Jmp:
            return {EVT_FLOW_JUMP, ir.target_step};
        case EVENT_OnMapReload:
            // Trigger, must be skipped but can be encountered in vanilla
            return {EVT_FLOW_STOP};
        case EVENT_OnLongTimer:
            // Trigger, must be skipped but can be encountered in vanilla
            return {EVT_FLOW_STOP};
        case EVENT_SetNPCTopic:
        {
            if (engine->gameTitle() == GameTitle::MM6) {
                mm6::setNpcTopic(*pNPCStats, GameTitle::MM6, ir.data.npc_topic_descr.npc_id,
                                 ir.data.npc_topic_descr.index, ir.data.npc_topic_descr.event_id);
                break;
            }
            if (ir.step == 7) {
                if (std::optional<QuestBit> activeBit = originalEndingActiveBitCleanup(_context, _events))
                    pParty->_questBits.reset(*activeBit);
            }
            NPCData *npc = &pNPCStats->pNPCData[ir.data.npc_topic_descr.npc_id];
            if (ir.data.npc_topic_descr.index == 0) npc->dialogue_1_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.index == 1) npc->dialogue_2_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.index == 2) npc->dialogue_3_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.index == 3) npc->dialogue_4_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.index == 4) npc->dialogue_5_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.index == 5) npc->dialogue_6_evt_id = ir.data.npc_topic_descr.event_id;
            if (ir.data.npc_topic_descr.npc_id == 8) {
                if (ir.data.npc_topic_descr.event_id == 78) {
                    houseDialogueGoBack();
                    window_SpeakInHouse = nullptr;
                    if (enterHouse(HOUSE_DARK_GUILD_PIT)) {
                        createHouseUI(HOUSE_DARK_GUILD_PIT);
                        current_npc_text = pNPCTopics[90].pText;
                    }
                }
            }
            break;
        }
        case EVENT_MoveNPC:
            if (engine->gameTitle() == GameTitle::MM6) {
                mm6::moveNpc(*pNPCStats, GameTitle::MM6, ir.data.npc_move_descr.npc_id,
                             ir.data.npc_move_descr.location_id);
                mm6OnNpcMoved();
                break;
            }
            pNPCStats->pNPCData[ir.data.npc_move_descr.npc_id].house = ir.data.npc_move_descr.location_id;
            // TODO(Nik-RE-dev): Looks like it's artifact of MM6
#if 0
            if (window_SpeakInHouse) {
                if (window_SpeakInHouse->houseId() == HOUSE_BODY_GUILD_MASTER_ERATHIA) {
                    houseDialogueGoBack();
                    pMediaPlayer->Unload();
                    window_SpeakInHouse->Release();
                    if (enterHouse(HOUSE_BODY_GUILD_MASTER_ERATHIA)) {
                        pAudioPlayer->playUISound(SOUND_Invalid);
                        window_SpeakInHouse = new GUIWindow_House({0, 0}, render->GetRenderDimensions(), HOUSE_BODY_GUILD_MASTER_ERATHIA, "");
                        window_SpeakInHouse->DeleteButtons();
                    }
                }
            }
#endif
            break;
        case EVENT_GiveItem:
        {
            if (engine->gameTitle() == GameTitle::MM6) {
                mm6::giveEventItem(ir.data.give_item_descr.treasure_level, ir.data.give_item_descr.treasure_type,
                                   ir.data.give_item_descr.item_id);
                break;
            }
            if (isOriginalOreSmithReward(_context, _events, ir) && fortuneRerollActive() &&
                (current_screen_type == SCREEN_HOUSE || current_screen_type == SCREEN_NPC_DIALOGUE)) {
                openOreRerollPopup(ir.data.give_item_descr.treasure_level, ir.data.give_item_descr.treasure_type);
                break;
            }
            if (ir.data.give_item_descr.item_id == ITEM_NULL &&
                captureClickedRandomLoot(ir.data.give_item_descr.treasure_level,
                                         ir.data.give_item_descr.treasure_type))
                break;
            Item item;
            item.Reset();
            pItemTable->generateItem(ir.data.give_item_descr.treasure_level, ir.data.give_item_descr.treasure_type, &item);
            if (ir.data.give_item_descr.item_id != ITEM_NULL) {
                item.itemId = ir.data.give_item_descr.item_id;
            }
            pParty->setHoldingItem(item);
            break;
        }
        case EVENT_ChangeEvent:
        {
            // The operand is the absolute id of the global event to run on the next click, and the byte in decorVars
            // stores it relative to decorationEventBase() through a mod-256 store, so the round trip only works for
            // ids in [base, base + 255]. All shipped records fit - every non-zero operand in MM7's evt files is in
            // [383, 415], and in MM6's in [410, 443]. MM8 data doesn't fit. Its global.evt uses ids 268-289 and
            // 531-570, and its engine maps them piecewise - states 0-21 are events 268-289, states 23-62 are events
            // 531-570, everything else passes through unchanged. Read out of the PS2 SLPS_250.31 binary at 0xa33e0
            // and 0xa3418, the PC MM8.exe is presumed to match.
            // TODO(captainurist): make the state<->event mapping piecewise for MM8 before feeding MM8 data through here.
            if (_context.objectPid.type() != OBJECT_Decoration) {
                MM_ERROR("Skipping step {} of evt event {}, ChangeEvent ran without a decoration", ir.step, _context.eventId);
                break;
            }
            LevelDecoration &decoration = pLevelDecorations[_context.objectPid.id()];
            if (ir.data.event_id) {
                engine->_persistentVariables.decorVars[decoration.eventVarId] = ir.data.event_id - decorationEventBase();
            } else {
                engine->_persistentVariables.decorVars[decoration.eventVarId] = 0;
                decoration.uFlags |= LEVEL_DECORATION_INVISIBLE;
            }
            break;
        }
        case EVENT_CheckSkill:
            assert(_who != CHOOSE_PARTY); // TODO(Nik-RE-dev): original code for this option is dubious
            for (Character &character : iterateCharacters(_who, grng)) {
                CombinedSkillValue val = character.getSkillValue(ir.data.check_skill_descr.skill_type);
                bool satisfied = engine->gameTitle() == GameTitle::MM6
                    ? mm6CheckSkillSatisfied(val, ir.data.check_skill_descr.skill_mastery,
                                             ir.data.check_skill_descr.skill_level)
                    : val.level() >= ir.data.check_skill_descr.skill_level &&
                          val.mastery() == ir.data.check_skill_descr.skill_mastery;
                if (satisfied)
                    return {EVT_FLOW_JUMP, ir.target_step};
            }
            break;
        case EVENT_SetNPCGroupNews:
            pNPCStats->pGroups[ir.data.npc_groups_descr.groups_id] = ir.data.npc_groups_descr.group;
            break;
        case EVENT_SetActorGroup:
            // TODO: enconunter and process
            assert(false);
#if 0
            *(&pActors[0].uGroup + 0x11000000 * _evt->v8 +
              209 * (_evt->v5 +
                     ((_evt->v6 + ((unsigned)_evt->v7 << 8)) << 8))) =
                EVT_DWORD(_evt->v9);
#endif
            break;
        case EVENT_NPCSetItem:
            npcSetItem(ir.data.npc_item_descr.id, ir.data.npc_item_descr.item, ir.data.npc_item_descr.is_give);
            break;
        case EVENT_SetNPCGreeting: {
            int greetingNpcId = ir.data.npc_descr.npc_id;
            // Tor Anwyn owns the Warlock promotion and greeting 83, but its return script targets Anthony Green.
            if (engine->gameTitle() == GameTitle::MM7 && _context.source == EVT_SOURCE_GLOBAL &&
                _context.eventId == 103 && greetingNpcId == 50 && ir.data.npc_descr.greeting == 83)
                greetingNpcId = 51;
            pNPCStats->pNPCData[greetingNpcId].flags &= ~NPC_GREETED_FIRST;
            pNPCStats->pNPCData[greetingNpcId].flags &= ~NPC_GREETED_SECOND;
            pNPCStats->pNPCData[greetingNpcId].greetingIndex = ir.data.npc_descr.greeting;
            break;
        }
        case EVENT_IsActorKilled: {
            if (isOriginalColonyZodKillMovieEvent(_context, _events) && ir.step == 1 &&
                pParty->_questBits[QBIT_XENOFEX_KILLED])
                return {EVT_FLOW_JUMP, 2};
            bool actorKilled;
            // These two quest scripts target actor zero exactly. The generic predicate filters exact actors through
            // map alert state and can therefore report a living mismatched actor as vacuously dead.
            if (isCelesteLadyCarmineKillCheck(_context, ir)) {
                actorKilled = pActors[0].IsNotAlive();
            } else if (isWineCellarVampireKillCheck(_context, ir)) {
                // The Elder Vampire starts disabled in the map data; that spawn state is not the required quest kill.
                actorKilled = pActors[0].aiState != Disabled && pActors[0].IsNotAlive();
            } else {
                actorKilled = Actor::isActorKilled(ir.data.actor_descr.policy, ir.data.actor_descr.param,
                                                   ir.data.actor_descr.num);
            }
            if (actorKilled) {
                return {EVT_FLOW_JUMP, ir.target_step};
            }
            break;
        }
        case EVENT_OnMapLeave:
            // Trigger, must be skipped but can be encountered in vanilla
            return {EVT_FLOW_STOP};
        case EVENT_ChangeGroup:
            // TODO: enconunter and process
            assert(false);
#if 0
            v38 = EVT_DWORD(_evt->v5);
            v39 = EVT_DWORD(_evt->v9);
            for (unsigned actor_id = 0; actor_id < pActors.size(); actor_id++) {
                if (pActors[actor_id].uGroup == v38)
                    pActors[actor_id].uGroup = v39;
            }
#endif
            break;
        case EVENT_ChangeGroupAlly:
            // TODO: enconunter and process
            assert(false);
#if 0
            v42 = EVT_DWORD(_evt->v5);
            v43 = EVT_DWORD(_evt->v9);
            for (unsigned actor_id = 0; actor_id < pActors.size(); actor_id++) {
                if (pActors[actor_id].uGroup == v42)
                    pActors[actor_id].uAlly = v43;
            }
#endif
            break;
        case EVENT_CheckSeason:
            if (checkSeason(ir.data.season)) {
                return {EVT_FLOW_JUMP, ir.target_step};
            }
            break;
        case EVENT_ToggleActorGroupFlag:
            toggleActorGroupFlag(ir.data.actor_flag_descr.id, ir.data.actor_flag_descr.attr, ir.data.actor_flag_descr.is_set);
            // The Wine Cellar's first actor is the uniquely named "Super Vampire (fix)" that event 2 checks for the
            // Light quest. The shipped DLV stores this quest target disabled and event 1 only makes group 5
            // aggressive, so the target otherwise cannot be fought. Activate only that authored actor while its
            // quest is active; the existing disabled-spawn guard still applies before the party accepts the quest.
            if (engine->gameTitle() == GameTitle::MM7 && engine->_indoor->filename == "d16.blv" &&
                _context.eventId == 1 && ir.data.actor_flag_descr.id == 5 &&
                ir.data.actor_flag_descr.attr == ACTOR_AGGRESSOR && ir.data.actor_flag_descr.is_set &&
                pParty->_questBits[QBIT_WINE_CELLAR_ACTIVE] &&
                !pParty->_questBits[QBIT_WINE_CELLAR_VAMPIRE_KILLED] &&
                !pParty->_questBits[QBIT_WINE_CELLAR_FINISHED] && !pActors.empty() &&
                pActors[0].monsterInfo.id == MONSTER_VAMPIRE_C && pActors[0].group == 5 &&
                pActors[0].uniqueNameIndex == 5 && pActors[0].aiState == Disabled) {
                Actor::toggleFlag(0, ACTOR_UNKNOW11, false);
                pActors[0].UpdateAnimation();
            }
            break;
        case EVENT_ToggleChestFlag:
            Chest::toggleFlag(ir.data.chest_flag_descr.chest_id, ir.data.chest_flag_descr.flag, ir.data.chest_flag_descr.is_set);
            break;
        case EVENT_CharacterAnimation:
            for (Character &character : iterateCharacters(ir.who, vrng))
                character.playReaction(ir.data.speech_id);
            break;
        case EVENT_SetActorItem:
            Actor::giveItem(ir.data.npc_item_descr.id, ir.data.npc_item_descr.item, ir.data.npc_item_descr.is_give);
            break;
        // None of these appear in MM7 data.
        case EVENT_OnDateTimer:
        case EVENT_EnableDateTimer:
        case EVENT_StopAnimation:
        case EVENT_CheckItemsCount:
        case EVENT_RemoveItems:
        case EVENT_SpecialJump:
        case EVENT_IsTotalBountyHuntingAwardInRange:
        case EVENT_IsNPCInParty:
            assert(false);
            break;
        default:
            break;
    }

    return {EVT_FLOW_NEXT};
}

int EvtInterpreter::executeStep(int step) {
    const EvtInstruction *ir = instructionAt(step);
    if (!ir)
        return -1;

    EvtFlow flow = executeInstruction(*ir);
    // MM7's two timed war assignments omit an Exit after their deadline cleanup blocks.
    // Continuing would fall through into the real-item success rewards even though the
    // deadline branch already removed the quest state and any carried quest item.
    if (flow.type == EVT_FLOW_NEXT && engine->gameTitle() == GameTitle::MM7 &&
        _context.source == EVT_SOURCE_GLOBAL &&
        ((_context.eventId == 125 && step == 11) || (_context.eventId == 132 && step == 12))) {
        return -1;
    }
    switch (flow.type) {
    default: assert(false); [[fallthrough]];
    case EVT_FLOW_STOP:
    case EVT_FLOW_YIELD: return -1; // The dialogue the instruction opened holds the rest of the event.
    case EVT_FLOW_NEXT: return step + 1;
    case EVT_FLOW_JUMP: return flow.target;
    }
}

int EvtInterpreter::checkTopicStep(int step) {
    const EvtInstruction *ir = instructionAt(step);
    if (!ir)
        return -1;

    switch (ir->opcode) {
    case EVENT_Exit:
    case EVENT_EndCanShowDialogItem:
        return -1;
    case EVENT_OnCanShowDialogItemCmp:
        if (engine->gameTitle() == GameTitle::MM6 &&
            ir->data.variable_descr.type == VAR_PlayerItemInHands) {
            for (const Character &character : pParty->pCharacters)
                if (mm6::hasEventItem(character, pParty->pPickedItem, 17,
                                      ir->data.variable_descr.value))
                    return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            ir->data.variable_descr.type == VAR_FixedGold) {
            if (mm6::hasEventGold(pParty->GetGold(), ir->data.variable_descr.value))
                return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            ir->data.variable_descr.type == VAR_DayOfYear) {
            if (mm6::compareDayOfYear(pParty->GetPlayingTime(), 19, ir->data.variable_descr.value))
                return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            std::to_underlying(ir->data.variable_descr.type) == 226) {
            if (mm6::compareMonthOfYear(pParty->uCurrentMonth, 226, ir->data.variable_descr.value))
                return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            std::to_underlying(ir->data.variable_descr.type) == 222) {
            if (mm6::comparePartyFlying(222, ir->data.variable_descr.value, pParty->bFlying,
                                        pParty->pPartyBuffs[PARTY_BUFF_FLY].Active()))
                return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            std::to_underlying(ir->data.variable_descr.type) == 213) {
            for (Character &character : pParty->pCharacters)
                if (mm6::CharacterEventBitState(character._characterEventBits).compare(
                        213, ir->data.variable_descr.value))
                    return ir->target_step;
            break;
        }
        if (engine->gameTitle() == GameTitle::MM6 &&
            ir->data.variable_descr.type == VAR_QBits_QuestsDone) {
            if (MM6QuestBitState(pParty->_questBits).compare(16, ir->data.variable_descr.value))
                return ir->target_step;
            break;
        }
        if (isMM6AutonoteVariable(ir->data.variable_descr.type)) {
            if (mm6AutonoteState().compare(std::to_underlying(ir->data.variable_descr.type),
                                           ir->data.variable_descr.value))
                return ir->target_step;
            break;
        }
        if (!validateVariableValue(*ir))
            break;
        for (Character &player : pParty->pCharacters)
            if (compareEvtVariable(player, ir->data.variable_descr.type, ir->data.variable_descr.value))
                return ir->target_step;
        break;
    case EVENT_SetCanShowDialogItem:
        _canShowTopic = ir->data.can_show_npc_dialogue;
        break;
    case EVENT_CanShowTopic_IsActorKilled:
        // TODO: enconunter and process
        assert(false);
#if 0
        if (Actor::isActorKilled(ir->data.actor_descr.policy, ir->data.actor_descr.param, ir->data.actor_descr.num))
            return ir->target_step;
#endif
        break;
    default:
        break;
    }
    return step + 1;
}

EvtContinuation EvtInterpreter::continuation(int step) const {
    EvtContext context = _context;
    context.duringMapLoad = false; // The loading screen is gone by the time the dialogue closes.
    return EvtContinuation(context, step);
}

bool EvtInterpreter::validateVariableValue(const EvtInstruction &ir) const {
    if (engine->gameTitle() == GameTitle::MM6) {
        const int rawId = std::to_underlying(ir.data.variable_descr.type);
        if (!describeMM6Variable(rawId).canUseInheritedExecutor())
            throw Exception("MM6 evt variable {} needs a title-specific executor", rawId);
        if (rawId == 19 && ir.opcode != EVENT_Compare && ir.opcode != EVENT_OnCanShowDialogItemCmp)
            throw Exception("MM6 day-of-year event variable only supports comparison");
        if (rawId == 21 && ir.opcode == EVENT_Set)
            throw Exception("MM6 gold event variable Set is absent from retained scripts");
        if (rawId == 17 && ir.opcode == EVENT_Set)
            throw Exception("MM6 item event Set is absent from retained scripts");
        if (!isValidMM6SharedCandidateValue(rawId, ir.data.variable_descr.value))
            throw Exception("MM6 evt variable {} value {} is outside title range", rawId,
                            ir.data.variable_descr.value);
    }

    if (isEvtVariableValueValid(ir.opcode, ir.data.variable_descr.type, ir.data.variable_descr.value))
        return true;

    MM_ERROR("Skipping step {} of evt event {}, value {} is out of range for evt variable {}",
             ir.step, _context.eventId, ir.data.variable_descr.value, std::to_underlying(ir.data.variable_descr.type));
    return false;
}

const EvtInstruction *EvtInterpreter::instructionAt(int step) const {
    auto pos = std::ranges::find(_events, step, &EvtInstruction::step);
    return pos == _events.end() ? nullptr : &*pos;
}
