#include "Engine/Evt/MM6DoorSwitch.h"

#include <algorithm>

#include "Engine/Graphics/Indoor.h"

namespace mm6 {

void switchDoorEvenIfMoving(unsigned int doorId) {
    auto pos = std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId);
    bool toInitialState = pos != pIndoor->doors.end() && (pos->state == DOOR_CLOSED || pos->state == DOOR_CLOSING);
    switchDoorAnimation(doorId, toInitialState ? DOOR_ACTION_OPEN : DOOR_ACTION_CLOSE);
}

} // namespace mm6
