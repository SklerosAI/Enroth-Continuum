#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace mm6 {

class Executable;

struct RoomDescriptor {
    std::string videoName;
    std::uint8_t background = 0;
    std::uint16_t npcPicture = 0;
    std::uint8_t rawHouseType = 0;
    std::uint8_t soundGroup = 0;
};

class RoomDescriptors {
 public:
    const RoomDescriptor &at(int roomId) const;

 private:
    std::array<RoomDescriptor, 119> _rooms;
    friend RoomDescriptors decodeRoomDescriptors(const Executable &exe);
};

/**
 * @param exe                           `MM6.exe` to read the room table from.
 * @return                              The 119 house rooms. Room 0 has no movie.
 */
RoomDescriptors decodeRoomDescriptors(const Executable &exe);

enum class HouseRoomKind {
    Room,
    Blank,
    Terminal,
};

struct HouseRoomSelection {
    HouseRoomKind kind;
    std::optional<RoomDescriptor> room;
};

// Classifies a retained MM6 house reference without opening a house, movie, or
// sound. A blank table cell and terminal pseudo-house have no room descriptor.
HouseRoomSelection selectHouseRoom(const RoomDescriptors &rooms, int houseId,
                                   std::optional<int> roomId);

} // namespace mm6
