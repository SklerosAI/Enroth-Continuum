#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIBranchlessDialogue.h"

#include "Utility/String/Format.h"

#include "MM6MapEntranceForTest.h"

struct SweepTimer {
    int eventId = 0;
    Duration firstAlarm; // After entry.
    Duration altInterval;
    Duration nextAlarm;
    bool fired = false;
};

struct SweepEvent {
    int eventId = 0;
    std::string_view kind;
    Pid pid;
};

struct SweepEventThrow {
    int eventId = 0;
    std::string_view kind;
    std::string error;
};

struct SweepRecord {
    std::string map;
    int mapId = 0;
    bool actors = true;
    std::string entrance;
    std::string phase = "first-map";
    std::string loadStage;
    std::string loadError;
    std::vector<int> onLoadEvents;
    int onLoadThrowEvent = 0;
    int actorCount = -1;
    int actorsAlive = -1;
    std::vector<SweepTimer> timers;
    int frames = 0;
    Duration elapsed;
    Duration target;
    std::string tickStop;
    std::vector<int> nextFrameTimers;
    std::set<int> attackingMonsters;
    int plateEvents = 0;
    int clickEvents = 0;
    int otherFaceEvents = 0;
    int decorationEvents = 0;
    int missingEvents = 0;
    int eventsFired = 0;
    int currentEvent = 0;
    std::vector<SweepEventThrow> eventThrows;
};

static std::string jsonString(std::string_view value) {
    std::string result = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') {
            result += '\\';
            result += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            result += fmt::format("\\u{:04x}", static_cast<unsigned char>(c));
        } else {
            result += c;
        }
    }
    return result + '"';
}

template<class Range, class Format>
static std::string jsonArray(const Range &range, Format format) {
    std::string result = "[";
    for (const auto &value : range) {
        if (result.size() > 1)
            result += ", ";
        result += format(value);
    }
    return result + ']';
}

static void writeSweepRecord(const std::filesystem::path &path, const SweepRecord &record) {
    // Rewritten after every step, so a process that dies mid-run leaves the step it died in.
    const auto number = [](int value) { return std::to_string(value); };
    std::string json = "{\n";
    json += fmt::format("  \"map\": {}, \"mapId\": {}, \"actors\": {}, \"entrance\": {},\n", jsonString(record.map), record.mapId,
                        record.actors, jsonString(record.entrance));
    json += fmt::format("  \"phase\": {}, \"loadStage\": {}, \"loadError\": {},\n", jsonString(record.phase), jsonString(record.loadStage),
                        jsonString(record.loadError));
    json += fmt::format("  \"onLoadEvents\": {}, \"onLoadThrowEvent\": {},\n", jsonArray(record.onLoadEvents, number), record.onLoadThrowEvent);
    json += fmt::format("  \"actorCount\": {}, \"actorsAlive\": {},\n", record.actorCount, record.actorsAlive);
    json += fmt::format("  \"timers\": {},\n", jsonArray(record.timers, [](const SweepTimer &timer) {
        return fmt::format("{{\"event\": {}, \"firstAlarmSeconds\": {}, \"altIntervalSeconds\": {}, \"fired\": {}}}", timer.eventId,
                           timer.firstAlarm.seconds(), timer.altInterval.seconds(), timer.fired);
    }));
    json += fmt::format("  \"frames\": {}, \"elapsedSeconds\": {}, \"targetSeconds\": {}, \"tickStop\": {},\n", record.frames,
                        record.elapsed.seconds(), record.target.seconds(), jsonString(record.tickStop));
    json += fmt::format("  \"nextFrameTimers\": {}, \"attackingMonsters\": {},\n", jsonArray(record.nextFrameTimers, number),
                        jsonArray(record.attackingMonsters, number));
    json += fmt::format("  \"plateEvents\": {}, \"clickEvents\": {}, \"otherFaceEvents\": {}, \"decorationEvents\": {},\n",
                        record.plateEvents, record.clickEvents, record.otherFaceEvents, record.decorationEvents);
    json += fmt::format("  \"missingEvents\": {}, \"eventsFired\": {}, \"currentEvent\": {},\n", record.missingEvents, record.eventsFired,
                        record.currentEvent);
    json += fmt::format("  \"eventThrows\": {}\n", jsonArray(record.eventThrows, [](const SweepEventThrow &value) {
        return fmt::format("{{\"event\": {}, \"kind\": {}, \"error\": {}}}", value.eventId, jsonString(value.kind), jsonString(value.error));
    }));
    json += "}\n";

    const std::filesystem::path temporary = path.string() + ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file << json;
    }
    std::filesystem::rename(temporary, path);
}

static std::string currentExceptionText() {
    try {
        throw;
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "non-standard exception";
    }
}

static void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

static void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

static std::vector<SweepTimer> loadedMapTimers() {
    // First alarm of every OnTimer and OnLongTimer, computed the way the event processor registers them on map load.
    const Time lastVisit = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? pIndoor->lastVisitTime : pOutdoor->lastVisitTime;
    const Time now = pParty->GetPlayingTime();
    std::vector<SweepTimer> result;
    for (EvtOpcode type : {EVENT_OnTimer, EVENT_OnLongTimer}) {
        for (const EventTrigger &trigger : engine->_localEventMap.enumerateTriggers(type)) {
            const auto &timer = engine->_localEventMap.instruction(trigger.eventId, trigger.eventStep).data.timer_descr;
            const Duration altInterval = Duration::fromSeconds(timer.alt_halfmin_interval * 30);
            Time alarm = now;
            if (altInterval) {
                alarm = now + altInterval;
            } else if (lastVisit) {
                const bool daily = !timer.is_yearly && !timer.is_monthly && !timer.is_weekly;
                if (daily) {
                    const Duration timeInsideDay = Duration::fromHours(timer.daily_start_hour) +
                                                   Duration::fromMinutes(timer.daily_start_minute) + Duration::fromSeconds(timer.daily_start_second);
                    alarm = Time::fromDays(lastVisit.toDays()) + timeInsideDay;
                    if (alarm < lastVisit)
                        alarm += Duration::fromDays(1);
                } else {
                    alarm = lastVisit + (timer.is_yearly ? Duration::fromYears(1) : timer.is_monthly ? Duration::fromDays(28) : Duration::fromDays(7));
                }
            }
            const Duration firstAlarm = std::max(Duration(), alarm - now);
            result.push_back({trigger.eventId, firstAlarm, altInterval, firstAlarm});
        }
    }
    return result;
}

static std::vector<SweepEvent> loadedMapTriggerEvents(SweepRecord *record) {
    // Each face and decoration event of the loaded map that its script has, once. Pressure plates come first, then
    // clickable faces, faces that objects or monsters trigger, and decorations.
    std::vector<SweepEvent> plates, clicks, others, decorations;
    std::set<int> seen;
    const std::span<BLVFace> faces = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? std::span(pIndoor->faces) : std::span(pOutdoor->faces);
    for (int index = 0; index < faces.size(); ++index) {
        const BLVFace &face = faces[index];
        if (!face.eventId || (face.attributes & FACE_EVENT_IS_HINT))
            continue;
        if (face.attributes & FACE_PRESSURE_PLATE) {
            plates.push_back({face.eventId, "plate", Pid()});
        } else if (face.Clickable()) {
            clicks.push_back({face.eventId, "click", Pid(OBJECT_Face, index)});
        } else {
            others.push_back({face.eventId, "other-face", Pid()});
        }
    }
    for (int index = 0; index < pLevelDecorations.size(); ++index)
        if (pLevelDecorations[index].uEventID)
            decorations.push_back({pLevelDecorations[index].uEventID, "decoration", Pid(OBJECT_Decoration, index)});

    std::vector<SweepEvent> result;
    for (const std::vector<SweepEvent> *group : {&plates, &clicks, &others, &decorations}) {
        for (const SweepEvent &event : *group) {
            if (!seen.insert(event.eventId).second)
                continue;
            if (!engine->_localEventMap.hasEvent(event.eventId)) {
                record->missingEvents++; // The map data names an event its script doesn't have.
                continue;
            }
            result.push_back(event);
            int &count = group == &plates ? record->plateEvents : group == &clicks ? record->clickEvents :
                         group == &others ? record->otherFaceEvents : record->decorationEvents;
            count++;
        }
    }
    return result;
}

static std::set<int> attackingMonsters() {
    std::set<int> result;
    for (const Actor &actor : pActors)
        if (actor.aiState == AttackingMelee || actor.aiState == AttackingRanged1 || actor.aiState == AttackingRanged2 ||
            actor.aiState == AttackingRanged3 || actor.aiState == AttackingRanged4)
            result.insert(std::to_underlying(actor.monsterInfo.id));
    return result;
}

static std::vector<int> parseEventList(const char *value) {
    std::vector<int> result;
    for (std::string_view rest = value ? value : ""; !rest.empty();) {
        const size_t comma = rest.find(',');
        result.push_back(std::atoi(std::string(rest.substr(0, comma)).c_str()));
        rest = comma == std::string_view::npos ? std::string_view() : rest.substr(comma + 1);
    }
    return result;
}

GAME_TEST(MM6MapSweep, DISABLED_EnterTickAndFireEvents) {
    // Sweeps one MM6 map per process for throws, for tools/run_mm6_map_sweep.py. ENROTH_MM6_MAP_SWEEP_MAP names the
    // map by ID or file name, or is "list" to write every map's ID and file name instead. ENROTH_MM6_MAP_SWEEP_RESULT
    // is the file to write. Optional: ENROTH_MM6_MAP_SWEEP_NO_ACTORS=1 loads the map without actors,
    // ENROTH_MM6_MAP_SWEEP_MAX_FRAMES caps the timer wait at that many frames instead of 3000,
    // ENROTH_MM6_MAP_SWEEP_SKIP_TIMERS=1 skips the wait, and ENROTH_MM6_MAP_SWEEP_SKIP_EVENTS lists event IDs not to run.
    // The party enters the first map through New Game, then the target map at its first shipped entrance, the way
    // MM6WorldLoadProbe does. The map's on-load events run as part of the load. One frame runs, then frames until
    // every timer has fired once, then each face and decoration event is run once without input, with the party
    // restored after each. A throw that escapes the game loop ends the process, the runner takes it from the
    // log and the record says which step it happened in.
    const char *mapValue = std::getenv("ENROTH_MM6_MAP_SWEEP_MAP");
    const char *resultValue = std::getenv("ENROTH_MM6_MAP_SWEEP_RESULT");
    if (!mapValue || !resultValue)
        GTEST_SKIP() << "Set ENROTH_MM6_MAP_SWEEP_MAP and ENROTH_MM6_MAP_SWEEP_RESULT";
    const char *noActorsValue = std::getenv("ENROTH_MM6_MAP_SWEEP_NO_ACTORS");
    const char *maxFramesValue = std::getenv("ENROTH_MM6_MAP_SWEEP_MAX_FRAMES");
    const int maxFrames = maxFramesValue ? std::atoi(maxFramesValue) : 3000;
    const char *skipTimersValue = std::getenv("ENROTH_MM6_MAP_SWEEP_SKIP_TIMERS");
    const bool skipTimers = skipTimersValue && std::string_view(skipTimersValue) == "1";
    const std::vector<int> skipEvents = parseEventList(std::getenv("ENROTH_MM6_MAP_SWEEP_SKIP_EVENTS"));
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_TRUE(window->nativeHandle() == nullptr || !window->isVisible());
    if (std::string_view(mapValue) == "list") {
        std::ofstream mapList(resultValue, std::ios::binary | std::ios::trunc);
        for (MapId id : pMapTable->pInfos.indices())
            if (isMapOutdoor(GameTitle::MM6, id) || isMapIndoor(GameTitle::MM6, id))
                mapList << std::to_underlying(id) << ' ' << pMapTable->pInfos[id].fileName << '\n';
        return;
    }

    const MapId map = mm6MapByNameOrId(mapValue);
    ASSERT_NE(map, MAP_INVALID) << mapValue;
    ASSERT_TRUE(isMapOutdoor(GameTitle::MM6, map) || isMapIndoor(GameTitle::MM6, map)) << mapValue;
    const std::filesystem::path resultPath(resultValue);
    SweepRecord record;
    record.map = pMapTable->pInfos[map].fileName;
    record.mapId = std::to_underlying(map);
    record.actors = !(noActorsValue && std::string_view(noActorsValue) == "1");
    const std::optional<MM6ShippedEntrance> entrance = findShippedMM6Entrance(record.map);
    record.entrance = entrance ? entrance->source : "Party Start";
    writeSweepRecord(resultPath, record);

    enterFirstMapFromNewGame(game);
    if (::testing::Test::HasFatalFailure())
        return;

    record.phase = "load";
    writeSweepRecord(resultPath, record);
    bool finished = false;
    game.runGameRoutine([&] {
        try {
            engine->config->debug.NoActors.setValue(!record.actors);
            for (Chest &chest : vChests)
                chest.position.reset(); // The first map's chest positions would make the target's chest load refuse.
            engine->_pendingTransition = entrance ? MapDestination(map, entrance->placement) : MapDestination(map, MAP_START_POINT_PARTY);
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            PrepareWorld(0);
        } catch (...) {
            record.loadError = currentExceptionText();
            // DoPrepareWorld clears the pending transition right before the on-load events.
            record.loadStage = engine->_pendingTransition ? "prepare" : "on-load events";
        }
        finished = true;
    });
    for (int tick = 0; tick < 256 && !finished; ++tick)
        game.tick();
    ASSERT_TRUE(finished);

    const Time entryTime = pParty->GetPlayingTime();
    game.runGameRoutine([&] {
        for (const EventTrigger &trigger : engine->_localEventMap.enumerateTriggers(EVENT_OnMapReload))
            record.onLoadEvents.push_back(trigger.eventId);
        record.timers = loadedMapTimers();
        record.actorCount = pActors.size();
        record.actorsAlive = std::ranges::count_if(pActors, [](const Actor &actor) { return actor.aiState != Dead && actor.aiState != Removed; });
        if (record.loadStage != "on-load events")
            return;
        for (const EventTrigger &trigger : engine->_localEventMap.enumerateTriggers(EVENT_OnMapReload)) {
            try {
                runEvent(EvtContext(EVT_SOURCE_MAP, trigger.eventId, Pid(), /* duringMapLoad = */ true), trigger.eventStep + 1);
            } catch (...) {
                record.onLoadThrowEvent = trigger.eventId;
                return;
            }
        }
    });
    if (!record.loadError.empty()) {
        record.phase = "load failed";
        writeSweepRecord(resultPath, record);
        return;
    }
    EXPECT_EQ(engine->_currentLoadedMapId, map);

    record.phase = "frame";
    writeSweepRecord(resultPath, record);
    game.tick();

    // The event processor checks timers once 30 game seconds have passed since its last check, the first one after
    // entry. A frame checks them before it advances the party's time, so it sees the time the previous frame left.
    const Duration checkInterval = Duration::fromSeconds(30);
    Duration lastCheck;
    for (const SweepTimer &timer : record.timers)
        record.target = std::max(record.target, timer.firstAlarm + checkInterval);
    record.phase = "timers";
    record.frames = 1;
    Duration previousElapsed;
    int stalledFrames = 0;
    while (true) {
        record.elapsed = pParty->GetPlayingTime() - entryTime;
        const bool checkNext = record.elapsed - lastCheck >= checkInterval;
        record.nextFrameTimers.clear();
        if (checkNext)
            for (const SweepTimer &timer : record.timers)
                if (timer.nextAlarm <= record.elapsed)
                    record.nextFrameTimers.push_back(timer.eventId);
        record.attackingMonsters = attackingMonsters();
        stalledFrames = record.elapsed == previousElapsed ? stalledFrames + 1 : 0;
        previousElapsed = record.elapsed;
        if (skipTimers) {
            record.tickStop = "skipped";
        } else if (record.timers.empty()) {
            record.tickStop = "no timers";
        } else if (std::ranges::all_of(record.timers, &SweepTimer::fired)) {
            record.tickStop = "all timers fired";
        } else if (record.frames >= maxFrames) {
            record.tickStop = "frame cap";
        } else if (stalledFrames >= 50) {
            record.tickStop = "game time stopped";
        } else if (engine->_currentLoadedMapId != map) {
            record.tickStop = "map changed";
        }
        writeSweepRecord(resultPath, record);
        if (!record.tickStop.empty())
            break;
        game.tick();
        record.frames++;
        if (!checkNext)
            continue;
        lastCheck = record.elapsed;
        for (SweepTimer &timer : record.timers) {
            if (timer.nextAlarm > lastCheck)
                continue;
            timer.fired = true;
            timer.nextAlarm = timer.altInterval ? lastCheck + timer.altInterval : Duration::fromYears(100);
        }
    }
    record.nextFrameTimers.clear();
    record.attackingMonsters.clear();

    record.phase = "events";
    std::vector<SweepEvent> events;
    game.runGameRoutine([&] { events = loadedMapTriggerEvents(&record); });
    writeSweepRecord(resultPath, record);
    for (const SweepEvent &event : events) {
        if (std::ranges::contains(skipEvents, event.eventId))
            continue;
        record.currentEvent = event.eventId;
        writeSweepRecord(resultPath, record);
        game.runGameRoutine([&] {
            const Party party = *pParty;
            try {
                runEvent(EvtContext(EVT_SOURCE_MAP, event.eventId, event.pid));
            } catch (...) {
                record.eventThrows.push_back({event.eventId, event.kind, currentExceptionText()});
            }
            *pParty = party;
            pGUIWindow_BranchlessDialogue.reset(); // Dropped unanswered, an MM6 question refuses to open over another.
            current_screen_type = SCREEN_GAME;
            engine->_pendingTransition.reset();
            uGameState = GAME_STATE_PLAYING;
        });
        record.eventsFired++;
    }
    record.currentEvent = 0;
    record.phase = "done";
    writeSweepRecord(resultPath, record);
}
