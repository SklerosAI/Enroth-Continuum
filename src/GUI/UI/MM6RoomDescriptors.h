#pragma once

#include <array>
#include <cstdint>
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

} // namespace mm6
