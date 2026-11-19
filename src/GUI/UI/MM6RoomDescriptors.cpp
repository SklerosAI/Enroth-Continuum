#include "GUI/UI/MM6RoomDescriptors.h"

#include <cstdint>
#include <stdexcept>
#include <string>

#include "Engine/MM6/MM6Executable.h"

namespace mm6 {

const RoomDescriptor &RoomDescriptors::at(int roomId) const {
    if (roomId < 0 || roomId >= static_cast<int>(_rooms.size()))
        throw std::runtime_error("MM6 room descriptor ID is out of range");
    return _rooms[roomId];
}

RoomDescriptors decodeRoomDescriptors(const Executable &exe) {
    RoomDescriptors result;
    for (int roomId = 0; roomId < 119; roomId++) {
        const std::uint32_t row = 0x4BE888 + 16 * roomId;
        RoomDescriptor &room = result._rooms[roomId];
        room.videoName = exe.string(exe.u32(row)).value_or(""); // Room 0 names memory past the file's data, which loads zeroed.
        room.background = exe.u8(row + 4);
        room.npcPicture = static_cast<std::uint16_t>(exe.i32(row + 8));
        room.rawHouseType = exe.u8(row + 12);
        room.soundGroup = exe.u8(row + 13);
    }
    return result;
}

HouseRoomSelection selectHouseRoom(const RoomDescriptors &rooms, int houseId,
                                   std::optional<int> roomId) {
    if (houseId == 600 || houseId == 601) {
        if (roomId)
            throw std::runtime_error("MM6 terminal house must not carry a room ID");
        return {HouseRoomKind::Terminal, std::nullopt};
    }
    if (houseId < 1 || houseId > 557)
        throw std::runtime_error("MM6 house ID is outside the retained table");
    if (!roomId)
        return {HouseRoomKind::Blank, std::nullopt};
    if (*roomId < 1 || *roomId > 118)
        throw std::runtime_error("MM6 house room ID is outside usable descriptors");

    const RoomDescriptor &room = rooms.at(*roomId);
    if (room.videoName.empty())
        throw std::runtime_error("MM6 house room has no file-backed movie name");
    return {HouseRoomKind::Room, room};
}

} // namespace mm6
