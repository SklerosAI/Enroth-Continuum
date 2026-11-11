#include "MM6MapTransition.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"
#include "Engine/mm7_data.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "Library/Logger/Logger.h"

#include "Utility/String/TransparentFunctors.h"

#include "Media/Audio/AudioPlayer.h"

#include "MM6ChestMapLoadRoute.h"
#include "MM6FirstMapManualSave.h"
#include "MM6OutdoorBorderConfirmation.h"
#include "MM6PartyRest.h"

namespace mm6 {

struct ParkedMap {
    Blob delta;
    std::vector<Actor> actors;
    std::vector<SpriteObject> sprites;
    PersistentVariables variables;
};

static bool borderArrivalPending = false;
static std::unordered_map<std::string, ParkedMap, TransparentStringHash, TransparentStringEquals> parkedMaps;

static void parkActiveMap() {
    const std::string &mapName = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;
    const bool indoor = uCurrentlyLoadedLevelType == LEVEL_INDOOR;
    const std::string deltaName = mapName.substr(0, mapName.size() - 3) + (indoor ? "dlv" : "ddm");
    ParkedMap parked;
    try {
        parked.delta = indoor ? encodeMM6IndoorParkedState(*pIndoor) : encodeMM6OutdoorParkedState(*pOutdoor);
    } catch (const std::exception &e) {
        MM_WARNING("MM6 map '{}' will be back as it was saved, its state can't be parked: {}", mapName, e.what());
        parkedMaps.erase(deltaName);
        return;
    }
    parked.actors.assign(pActors.begin(), pActors.end());
    parked.sprites = pSpriteObjects;
    parked.variables = engine->_persistentVariables;
    parkedMaps.insert_or_assign(deltaName, std::move(parked));
}

MapDestination borderTravelDestination(int partyX, int partyY) {
    const BorderTravelAssessment route = assessOutdoorBorderTravel(
        GameTitle::MM6, pMapTable->pInfos[engine->_currentLoadedMapId].fileName, partyX, partyY, {});
    if (!route.candidate)
        return {};

    uDefaultTravelTime_ByFoot = borderTravelDays({});
    const Vec3f arrival(route.candidate->arrivalX, route.candidate->arrivalY, pParty->pos.z);
    return MapDestination(pMapTable->GetMapInfo(route.candidate->destinationMap),
                          PartyPlacement(arrival, -1, pParty->_viewPitch, 0));
}

bool confirmBorderTravel(const MapDestination &offered) {
    const std::string &sourceMap = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;
    const BorderTravelHirelings hirelings = borderTravelHirelings(CheckHiredNPCSpeciality);
    const BorderTravelAssessment route = assessOutdoorBorderTravel(GameTitle::MM6, sourceMap, pParty->pos.x, pParty->pos.y,
                                                                   hirelings);
    if (!route.candidate || offered.map() != pMapTable->GetMapInfo(route.candidate->destinationMap))
        return false;
    BorderConfirmation confirmed = stageBorderConfirmation(GameTitle::MM6, sourceMap, *route.candidate, *pParty, hirelings,
                                                           restAndHealDetachedParty);
    if (confirmed.refusal != BorderConfirmationRefusal::None)
        return false;

    pAudioPlayer->stopSounds();
    gameTimer->setPaused(true);
    *pParty = std::move(*confirmed.party);
    for (int character : confirmed.weakReactions)
        pParty->pCharacters[character].playReaction(SPEECH_WEAK);

    leaveMap(offered.map());
    borderArrivalPending = true;
    uGameState = GAME_STATE_CHANGE_LOCATION;
    engine->_pendingTransition = MapDestination(offered.map(), PartyPlacement(pParty->pos, -1, pParty->_viewPitch, 0));
    return true;
}

void settleBorderArrival(PartyPlacement *placement) {
    if (!borderArrivalPending)
        return;
    borderArrivalPending = false;
    placement->pos.z = pOutdoor->pTerrain.heightByPos(placement->pos);
}

void leaveMap(MapId target) {
    parkActiveMap();
    disarmFirstMapManualSave(); // Its anchor covers only the first map of a session that never left it.
    admitChestMapLoad(target);
}

std::optional<Blob> parkedMapDelta(std::string_view deltaName) {
    const auto pos = parkedMaps.find(deltaName);
    if (pos == parkedMaps.end())
        return std::nullopt;
    return Blob::share(pos->second.delta);
}

void restoreParkedMap(std::string_view deltaName, bool respawned) {
    const auto pos = parkedMaps.find(deltaName);
    if (pos == parkedMaps.end())
        return;
    if (!respawned) {
        pActors.assign(pos->second.actors.begin(), pos->second.actors.end());
        pSpriteObjects = std::move(pos->second.sprites);
        engine->_persistentVariables = pos->second.variables;
    }
    parkedMaps.erase(pos);
}

void forgetParkedMaps() {
    parkedMaps.clear();
}

} // namespace mm6
