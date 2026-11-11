#include "MM6MapTransition.h"

#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"
#include "Engine/mm7_data.h"

#include "Media/Audio/AudioPlayer.h"

#include "MM6ChestMapLoadRoute.h"
#include "MM6FirstMapManualSave.h"
#include "MM6OutdoorBorderConfirmation.h"
#include "MM6PartyRest.h"

namespace mm6 {

static bool borderArrivalPending = false;

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
    disarmFirstMapManualSave(); // Its anchor covers only the first map of a session that never left it.
    admitChestMapLoad(target);
}

} // namespace mm6
