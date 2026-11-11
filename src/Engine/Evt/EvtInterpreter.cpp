#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIDialogue.h"
#include "GUI/UI/UIBranchlessDialogue.h"
#include "GUI/UI/UITransition.h"
#include "GUI/UI/UIStatusBar.h"

#include "Library/Logger/Logger.h"

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

    int step = startStep;

    _who = !pParty->hasActiveCharacter() ? CHOOSE_RANDOM : CHOOSE_ACTIVE;

    while (step != -1)
        step = executeStep(step);

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
                    mm6::setEventSkill(character, std::to_underlying(ir.data.variable_descr.type),
                                       ir.data.variable_descr.value, GameTitle::MM6);
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
                current_npc_text = pNPCTopics[ir.data.text_id - 1].pText;
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
            // stores it relative to the dispatch base of 380. The -124 is -380 folded through the mod-256 store, so
            // the round trip only works for ids in [380, 635], and all shipped MM6 and MM7 records fit - every
            // non-zero operand in both games' evt files is in [383, 443]. MM8 data doesn't fit. Its global.evt uses
            // ids 268-289 and 531-570, and its engine maps them piecewise - states 0-21 are events 268-289, states
            // 23-62 are events 531-570, everything else passes through unchanged. Read out of the PS2 SLPS_250.31
            // binary at 0xa33e0 and 0xa3418, the PC MM8.exe is presumed to match.
            // TODO(captainurist): make the state<->event mapping per-game before feeding MM8 data through here, and
            //                     spell the MM7 store as -380 while at it.
            if (_context.objectPid.type() != OBJECT_Decoration) {
                MM_ERROR("Skipping step {} of evt event {}, ChangeEvent ran without a decoration", ir.step, _context.eventId);
                break;
            }
            LevelDecoration &decoration = pLevelDecorations[_context.objectPid.id()];
            if (ir.data.event_id) {
                engine->_persistentVariables.decorVars[decoration.eventVarId] = ir.data.event_id - 124;
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
        case EVENT_SetNPCGreeting:
            pNPCStats->pNPCData[ir.data.npc_descr.npc_id].flags &= ~NPC_GREETED_FIRST;
            pNPCStats->pNPCData[ir.data.npc_descr.npc_id].flags &= ~NPC_GREETED_SECOND;
            pNPCStats->pNPCData[ir.data.npc_descr.npc_id].greetingIndex = ir.data.npc_descr.greeting;
            break;
        case EVENT_IsActorKilled:
            if (Actor::isActorKilled(ir.data.actor_descr.policy, ir.data.actor_descr.param, ir.data.actor_descr.num)) {
                return {EVT_FLOW_JUMP, ir.target_step};
            }
            break;
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
