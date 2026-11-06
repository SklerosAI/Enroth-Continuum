#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

class Blob;

namespace mm6 {

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
    friend RoomDescriptors decodeRoomDescriptors(const Blob &source);
};

// Reads only a generated private profile manifest. The fields are not yet
// wired to MM6 house entry, which remains guarded before inherited effects.
RoomDescriptors decodeRoomDescriptors(const Blob &source);

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
