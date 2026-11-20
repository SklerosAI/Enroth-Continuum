#include "Testing/Unit/UnitTest.h"

#include "Engine/Evt/MM6DoorSwitch.h"
#include "Engine/Graphics/Indoor.h"

UNIT_TEST(MM6DoorSwitch, SwitchesDoorInEveryState) {
    // Door action 3 from the GrayFace patch did nothing, so a modded script couldn't switch a door, moving or not.
    struct Case {
        DoorState state;
        Duration time;
        DoorState expectedState;
        Duration expectedTime;
    };
    // 2 seconds to open and 1 to close. A turned door starts from where it was.
    const Case cases[] = {
        {DOOR_OPEN, 0_ticks, DOOR_CLOSING, 0_ticks},
        {DOOR_CLOSED, 0_ticks, DOOR_OPENING, 0_ticks},
        {DOOR_CLOSING, Duration::fromRealtimeMilliseconds(250), DOOR_OPENING, Duration::fromRealtimeMilliseconds(1500)},
        {DOOR_OPENING, Duration::fromRealtimeMilliseconds(1500), DOOR_CLOSING, Duration::fromRealtimeMilliseconds(250)},
    };

    IndoorLocation *savedIndoor = pIndoor;
    for (const Case &c : cases) {
        IndoorLocation location;
        BLVDoor &door = location.doors.emplace_back();
        door.doorId = 5;
        door.moveLength = 512;
        door.openSpeed = 256;
        door.closeSpeed = 512;
        door.state = c.state;
        door.timeSinceTriggered = c.time;
        pIndoor = &location;

        mm6::switchDoorEvenIfMoving(5);

        EXPECT_EQ(door.state, c.expectedState);
        EXPECT_EQ(door.timeSinceTriggered, c.expectedTime);
    }
    pIndoor = savedIndoor;
}
