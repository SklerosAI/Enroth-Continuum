#include "Processor.h"

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Data/HouseEnums.h"
#include "Engine/Localization.h"
#include "Engine/mm7_data.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MM6/MM6MapVisit.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Party.h"

#include "GUI/UI/UIStatusBar.h"

#include "Library/Logger/Logger.h"

struct MapTimer {
    Duration interval;
    Duration timeInsideDay;
    Duration altInterval;
    Time alarmTime;
    int eventId = 0;
    int eventStep = 0;
};

static std::vector<EventTrigger> onMapLoadTriggers;
static std::vector<EventTrigger> onMapLeaveTriggers;

static std::vector<MapTimer> onLongTimerTriggers;
static std::vector<MapTimer> onTimerTriggers;

static std::vector<int> decorationsWithEvents;

// Was in original code and ensures that timers are checked not more often than 30 game seconds.
// Do not needed in practice but can be considered optimization to avoid checking timers too often.
static Time timerGuard;

namespace {

struct PaintingReloadSource {
    MapId map;
    int eventId;
    ItemId item;
    QuestBit takenBit;
    int emptyTextureCog;
    std::string_view emptyTexture;
};

constexpr std::array paintingReloadSources = {
    PaintingReloadSource{MAP_HAUNTED_MANSION, 376, ITEM_QUEST_ANGEL_STATUE_PAINTING,
                         QBIT_ANGEL_STATUE_PAINTING_TAKEN, 15, "t2bs"},
    PaintingReloadSource{MAP_CASTLE_GRYPHONHEART, 377, ITEM_QUEST_ROLAND_IRONFIST_PAINTING,
                         QBIT_ROLAND_PAINTING_TAKEN, 15, "chb1"},
    PaintingReloadSource{MAP_CASTLE_GRYPHONHEART, 378, ITEM_QUEST_ARCHIBALD_IRONFIST_PAINTING,
                         QBIT_ARCHIBALD_PAINTING_TAKEN, 16, "chb1"},
};

constexpr Vec3f originalAvleeRockPosition(-15088, -4288, 208);
constexpr float correctedAvleeRockZ = 148;

constexpr int mountNighonSetupEventId = 1;
constexpr int mountNighonWarlockGroupA = 26;
constexpr int mountNighonWarlockGroupB = 27;

struct MountNighonDoorHalf {
    int model;
    int face;
    int originalEvent;
    int correctedEvent;
    HouseId house;
};

constexpr std::array mountNighonDoorHalves = {
    MountNighonDoorHalf{0, 38, 12, 11, HOUSE_TAVERN_MOUNT_NIGHON},
    MountNighonDoorHalf{12, 17, 10, 9, HOUSE_TRAINING_HALL_MOUNT_NIGHON},
};

const EvtInstruction *originalPaintingEmptyTexture(const PaintingReloadSource &source) {
    if (engine->gameTitle() != GameTitle::MM7 || engine->_currentLoadedMapId != source.map ||
        !engine->_localEventMap.hasEvent(source.eventId))
        return nullptr;

    const std::span<const EvtInstruction> instructions = engine->_localEventMap.function(source.eventId);
    if (instructions.size() != 6 ||
        std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
            return instruction.opcode == EVENT_OnMapReload;
        }))
        return nullptr;

    const EvtInstruction &gate = engine->_localEventMap.instruction(source.eventId, 0);
    const EvtInstruction &texture = engine->_localEventMap.instruction(source.eventId, 1);
    const EvtInstruction &item = engine->_localEventMap.instruction(source.eventId, 2);
    const EvtInstruction &taken = engine->_localEventMap.instruction(source.eventId, 3);
    const EvtInstruction &exit = engine->_localEventMap.instruction(source.eventId, 4);
    const int textureCount = std::ranges::count(instructions, EVENT_SetTexture, &EvtInstruction::opcode);
    if (gate.opcode != EVENT_Compare || gate.target_step != 4 ||
        gate.data.variable_descr.type != VAR_QBits_QuestsDone ||
        gate.data.variable_descr.value != std::to_underlying(source.takenBit) ||
        texture.opcode != EVENT_SetTexture || texture.data.sprite_texture_descr.cog != source.emptyTextureCog ||
        texture.str != source.emptyTexture || textureCount != 1 ||
        item.opcode != EVENT_Add || item.data.variable_descr.type != VAR_PlayerItemInHands ||
        item.data.variable_descr.value != std::to_underlying(source.item) ||
        taken.opcode != EVENT_Add || taken.data.variable_descr.type != VAR_QBits_QuestsDone ||
        taken.data.variable_descr.value != std::to_underlying(source.takenBit) || exit.opcode != EVENT_Exit)
        return nullptr;
    return &texture;
}

void restoreTakenPaintingTextures() {
    for (const PaintingReloadSource &source : paintingReloadSources) {
        if (!pParty->_questBits[source.takenBit])
            continue;
        if (const EvtInstruction *texture = originalPaintingEmptyTexture(source))
            setTexture(texture->data.sprite_texture_descr.cog, texture->str);
    }
}

void correctOriginalAvleeFlyingRock() {
    if (engine->gameTitle() != GameTitle::MM7 || engine->_currentLoadedMapId != MAP_AVLEE)
        return;

    LevelDecoration *candidate = nullptr;
    for (LevelDecoration &decoration : pLevelDecorations) {
        if (decoration.vPosition.x != originalAvleeRockPosition.x ||
            decoration.vPosition.y != originalAvleeRockPosition.y)
            continue;
        if (candidate)
            return; // A duplicate coordinate is custom data, so leave both untouched.
        candidate = &decoration;
    }
    if (!candidate || candidate->uDecorationDescID != DECORATION_ROCK_110 ||
        pDecorationTable->decoration(candidate->uDecorationDescID)->internalName != "rock01" ||
        candidate->uFlags != LevelDecorationFlags() || candidate->vPosition != originalAvleeRockPosition ||
        candidate->_yawAngle != 0 || candidate->uCog != 0 || candidate->uEventID != 0 ||
        candidate->uTriggerRange != 0 || candidate->eventVarId != -75)
        return;

    bool onWater = false;
    int faceId = -1;
    const float floor = ODM_GetFloorLevel(candidate->vPosition, &onWater, &faceId);
    if (onWater || floor != correctedAvleeRockZ)
        return;
    candidate->vPosition.z = floor;
}

bool hasMountNighonHouseInstruction(std::span<const EvtInstruction> instructions, int step,
                                    EvtOpcode opcode, HouseId house = HOUSE_INVALID) {
    return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
        return instruction.step == step && instruction.opcode == opcode &&
               (opcode != EVENT_SpeakInHouse || instruction.data.house_id == house);
    });
}

bool hasOriginalMountNighonDoorEvents(const MountNighonDoorHalf &source) {
    if (!engine->_localEventMap.hasEvent(source.originalEvent) ||
        !engine->_localEventMap.hasEvent(source.correctedEvent))
        return false;

    const std::span<const EvtInstruction> original = engine->_localEventMap.function(source.originalEvent);
    const std::span<const EvtInstruction> corrected = engine->_localEventMap.function(source.correctedEvent);
    return original.size() == 3 && corrected.size() == 2 &&
           hasMountNighonHouseInstruction(original, 0, EVENT_Exit) &&
           hasMountNighonHouseInstruction(original, 1, EVENT_SpeakInHouse, source.house) &&
           hasMountNighonHouseInstruction(corrected, 0, EVENT_SpeakInHouse, source.house);
}

void correctOriginalMountNighonDoorHalves() {
    if (engine->gameTitle() != GameTitle::MM7 || engine->_currentLoadedMapId != MAP_MOUNT_NIGHON)
        return;

    for (const MountNighonDoorHalf &source : mountNighonDoorHalves) {
        if (!hasOriginalMountNighonDoorEvents(source) || source.model >= std::ssize(pOutdoor->pBModels))
            continue;
        const BSPModel &model = pOutdoor->pBModels[source.model];
        if (source.face >= std::ssize(model.faces))
            continue;
        BLVFace &face = pOutdoor->faces[model.faces[source.face]];
        if (face.eventId != source.originalEvent || !face.Clickable())
            continue;
        face.eventId = source.correctedEvent;
    }
}

bool isOriginalMountNighonFactionSetup() {
    if (engine->gameTitle() != GameTitle::MM7 || engine->_currentLoadedMapId != MAP_MOUNT_NIGHON ||
        !engine->_localEventMap.hasEvent(mountNighonSetupEventId))
        return false;

    const std::span<const EvtInstruction> instructions =
        engine->_localEventMap.function(mountNighonSetupEventId);
    if (instructions.size() != 7)
        return false;

    const auto hasInstruction = [instructions](int step, EvtOpcode opcode) {
        return std::ranges::any_of(instructions, [=](const EvtInstruction &instruction) {
            return instruction.step == step && instruction.opcode == opcode;
        });
    };
    const bool hasTownPortalGate = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 1 && instruction.target_step == 3 && instruction.opcode == EVENT_Compare &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_TOWN_PORTAL_TO_MOUNT_NIGHON);
    });
    const bool hasTownPortalUnlock = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 2 && instruction.opcode == EVENT_Add &&
               instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
               instruction.data.variable_descr.value == std::to_underlying(QBIT_TOWN_PORTAL_TO_MOUNT_NIGHON);
    });
    const bool hasWildernessAggression = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.step == 3 && instruction.opcode == EVENT_ToggleActorGroupFlag &&
               instruction.data.actor_flag_descr.id == 5 &&
               instruction.data.actor_flag_descr.attr == ACTOR_AGGRESSOR &&
               instruction.data.actor_flag_descr.is_set;
    });
    const bool hasPatchedFactionWrite = std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_ChangeGroupAlly;
    });
    return hasInstruction(0, EVENT_OnMapReload) && hasTownPortalGate &&
           hasTownPortalUnlock && hasWildernessAggression &&
           hasInstruction(4, EVENT_Exit) && hasInstruction(5, EVENT_Exit) && !hasPatchedFactionWrite;
}

void correctOriginalMountNighonWarlockFaction() {
    if (!isOriginalMountNighonFactionSetup())
        return;

    for (Actor &actor : pActors) {
        if (actor.group == mountNighonWarlockGroupA || actor.group == mountNighonWarlockGroupB)
            actor.hostilityGroup = MONSTER_TYPE_WARLOCK;
    }
}

} // namespace

void initDecorationEvents() {
    DecorationId id = pDecorationTable->decorationId("Event Trigger");

    decorationsWithEvents.clear();
    for (int i = 0; i < pLevelDecorations.size(); ++i) {
        if (pLevelDecorations[i].uDecorationDescID == id) {
            decorationsWithEvents.push_back(i);
        }
    }
}

void checkDecorationEvents() {
    for (int decorationId : decorationsWithEvents) {
        const LevelDecoration &decoration = pLevelDecorations[decorationId];

        if (decoration.uFlags & LEVEL_DECORATION_TRIGGERED_BY_TOUCH)
            if ((decoration.vPosition - pParty->pos).length() < decoration.uTriggerRange)
                runEvent(EvtContext(EVT_SOURCE_MAP, decoration.uEventID, Pid(OBJECT_Decoration, decorationId)));

        if (decoration.uFlags & LEVEL_DECORATION_TRIGGERED_BY_MONSTER)
            for (const Actor &actor : pActors)
                if ((decoration.vPosition - actor.pos).length() < decoration.uTriggerRange)
                    runEvent(EvtContext(EVT_SOURCE_MAP, decoration.uEventID));

        if (decoration.uFlags & LEVEL_DECORATION_TRIGGERED_BY_OBJECT)
            for (const SpriteObject &object : pSpriteObjects)
                if ((decoration.vPosition - object.vPosition).length() < decoration.uTriggerRange)
                    runEvent(EvtContext(EVT_SOURCE_MAP, decoration.uEventID));
    }
}

static void registerTimerTriggers(EvtOpcode triggerType, std::vector<MapTimer> *triggers) {
    std::vector<EventTrigger> timerTriggers = engine->_localEventMap.enumerateTriggers(triggerType);

    // TODO(Nik-RE-dev): using time of last visit will help timers only slightly because each map leaving resets it.
    //                   To support fair timers they need to be saved directly.
    Time levelLastVisit = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? pIndoor->lastVisitTime : pOutdoor->lastVisitTime;

    triggers->clear();
    for (EventTrigger &trigger : timerTriggers) {
        MapTimer timer;
        EvtInstruction ir = engine->_localEventMap.instruction(trigger.eventId, trigger.eventStep);

        if (ir.data.timer_descr.alt_halfmin_interval) {
            // Alternative interval is defined in terms of half-minutes
            timer.altInterval = Duration::fromSeconds(ir.data.timer_descr.alt_halfmin_interval * 30);
            timer.alarmTime = pParty->GetPlayingTime() + timer.altInterval;
        } else {
            if (ir.data.timer_descr.is_yearly) {
                timer.interval = Duration::fromYears(1);
            } else if (ir.data.timer_descr.is_monthly) {
                timer.interval = Duration::fromDays(28);
            } else if (ir.data.timer_descr.is_weekly) {
                timer.interval = Duration::fromDays(7);
            } else {
                // Interval is daily with exact time of day
                timer.interval = Duration::fromDays(1);
                timer.timeInsideDay = Duration::fromHours(ir.data.timer_descr.daily_start_hour);
                timer.timeInsideDay += Duration::fromMinutes(ir.data.timer_descr.daily_start_minute);
                timer.timeInsideDay += Duration::fromSeconds(ir.data.timer_descr.daily_start_second);
            }

            if (engine->gameTitle() == GameTitle::MM6 && engine->config->gameplay.FixMapTimers.value()) {
                const auto firings = mm6::mapTimerFirings().find(pMapTable->pInfos[engine->_currentLoadedMapId].fileName);
                const Time lastFiring = firings == mm6::mapTimerFirings().end() ? Time() : firings->second[mm6::mapTimerIntervalIndex(timer.interval)];
                timer.alarmTime = mm6::mapTimerFirstAlarmAfterLastFiring(triggerType, timer.interval, timer.timeInsideDay, levelLastVisit,
                                                                         lastFiring, pParty->GetPlayingTime());
            } else if (engine->gameTitle() == GameTitle::MM6) {
                timer.alarmTime = mm6::mapTimerFirstAlarm(triggerType, timer.interval, timer.timeInsideDay, levelLastVisit,
                                                          pParty->GetPlayingTime());
            } else if (timer.interval == Duration::fromDays(1)) {
                if (levelLastVisit) {
                    // Calculate alarm time inside last visit day
                    timer.alarmTime = Time::fromDays(levelLastVisit.toDays()) + timer.timeInsideDay;
                    if (timer.alarmTime < levelLastVisit) {
                        // Last visit time already passed alarm time inside that day so move alarm to next day
                        timer.alarmTime = timer.alarmTime + Duration::fromDays(1);
                    }
                } else {
                    // Set alarm time to zero because it must always fire
                    timer.alarmTime = Time();
                }
            } else {
                if (levelLastVisit) {
                    timer.alarmTime = levelLastVisit + timer.interval;
                } else {
                    // Without last visit all timers must fire immediately
                    timer.alarmTime = pParty->GetPlayingTime();
                }
            }

            assert(timer.interval > 0_ticks);
        }
        timer.eventId = trigger.eventId;
        timer.eventStep = trigger.eventStep;

        triggers->push_back(timer);
    }
}

void runEvent(EvtContext context, int startStep) {
    if (!context.eventId) {
        engine->_statusBar->nothingHere();
        return;
    }

    EvtInterpreter interpreter;
    MM_TRACE("Executing event starting from step {}", startStep);
    interpreter.prepare(context);

    if (!interpreter.isValid()) {
        MM_WARNING("Face has invalid event ID");
        engine->_statusBar->nothingHere();
        return;
    }

    if (interpreter.execute(startStep) == EVT_TRANSITION_STARTED)
        onMapLeave();
}

bool canShowTopic(int eventId) {
    EvtInterpreter interpreter;
    interpreter.prepare(EvtContext(EVT_SOURCE_GLOBAL, eventId));
    return interpreter.canShowTopic();
}

bool hasEventHint(int eventId) {
    return engine->_localEventMap.hasHint(eventId);
}

std::string getEventHintString(int eventId) {
    return engine->_localEventMap.hint(eventId);
}

static void registerEventTriggers() {
    onMapLoadTriggers.clear();
    onMapLoadTriggers = engine->_localEventMap.enumerateTriggers(EVENT_OnMapReload);
    onMapLeaveTriggers.clear();
    onMapLeaveTriggers = engine->_localEventMap.enumerateTriggers(EVENT_OnMapLeave);

    registerTimerTriggers(EVENT_OnLongTimer, &onLongTimerTriggers);
    registerTimerTriggers(EVENT_OnTimer, &onTimerTriggers);
}

void onMapLoad() {
    // Register all triggers when map done loading
    registerEventTriggers();

    timerGuard = pParty->GetPlayingTime();

    for (EventTrigger &triggers : onMapLoadTriggers)
        runEvent(EvtContext(EVT_SOURCE_MAP, triggers.eventId, Pid(), /* duringMapLoad = */ true), triggers.eventStep + 1);

    // The original painting pickups persist only their quest bits. Reapply their authored empty-wall texture after
    // normal reload events so revisiting the map cannot make a taken painting appear again.
    restoreTakenPaintingTextures();

    // Avlee's original rock01 at this exact coordinate is authored sixty units above its terrain tile.
    correctOriginalAvleeFlyingRock();

    // Mount Nighon's original data assigns two door halves to exit-only events and leaves two authored peasants in
    // factions that the town's Warlocks attack. Apply the two exact GrayFace corrections only while all shipped
    // event signatures are intact, leaving patched and custom map data authoritative.
    correctOriginalMountNighonDoorHalves();
    correctOriginalMountNighonWarlockFaction();
}

void onMapLeave() {
    for (EventTrigger &triggers : onMapLeaveTriggers)
        runEvent(EvtContext(EVT_SOURCE_MAP, triggers.eventId), triggers.eventStep + 1);

    // Cleanup timers to avoid firing while map transition is in process
    onLongTimerTriggers.clear();
    onTimerTriggers.clear();
}

static void checkTimer(MapTimer &timer, EvtOpcode triggerType) {
    if (pParty->GetPlayingTime() >= timer.alarmTime) {
        const bool mm6PeriodicTimer = engine->gameTitle() == GameTitle::MM6 && engine->config->gameplay.FixMapTimers.value() &&
                                      triggerType == EVENT_OnLongTimer && !timer.altInterval;
        if (mm6PeriodicTimer) {
            mm6::mapTimerFirings()[pMapTable->pInfos[engine->_currentLoadedMapId].fileName][mm6::mapTimerIntervalIndex(timer.interval)] =
                pParty->GetPlayingTime();
        }
        runEvent(EvtContext(EVT_SOURCE_MAP, timer.eventId), timer.eventStep + 1);
        if (timer.altInterval) {
            timer.alarmTime = pParty->GetPlayingTime() + timer.altInterval;
        } else if (mm6PeriodicTimer) {
            timer.alarmTime = mm6::longTimerNextAlarm(timer.alarmTime, timer.interval, pParty->GetPlayingTime());
        } else {
            if (!timer.alarmTime.isValid() && timer.interval == Duration::fromDays(1)) {
                // Initial firing of daily timers, next alarm must be configured to fire on exact time of day
                timer.alarmTime = Time() + timer.timeInsideDay;
            }
            while (pParty->GetPlayingTime() >= timer.alarmTime) {
                timer.alarmTime += timer.interval;
            }
        }
    }
}

void onTimer() {
    if (gameTimer->isPaused()) {
        return;
    }

    if ((pParty->GetPlayingTime() - timerGuard) < Duration::fromSeconds(30)) { // 30 game seconds = 1 realtime second.
        return;
    }

    timerGuard = pParty->GetPlayingTime();

    for (MapTimer &timer : onTimerTriggers) {
        checkTimer(timer, EVENT_OnTimer);
    }

    for (MapTimer &timer : onLongTimerTriggers) {
        checkTimer(timer, EVENT_OnLongTimer);
    }
}
