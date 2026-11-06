#include "GUI/UI/MM6RoomDescriptors.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Ascii.h"

namespace mm6 {
namespace {

int checkedNumber(const Json &value, int minimum, int maximum) {
    if (!value.is_number_integer())
        throw std::runtime_error("MM6 room descriptor field is not an integer");
    if (value.is_number_unsigned()) {
        const std::uint64_t number = value.get<std::uint64_t>();
        if (number > static_cast<std::uint64_t>(maximum))
            throw std::runtime_error("MM6 room descriptor field exceeds range");
        return static_cast<int>(number);
    }
    const std::int64_t number = value.get<std::int64_t>();
    if (number < minimum || number > maximum)
        throw std::runtime_error("MM6 room descriptor field exceeds range");
    return static_cast<int>(number);
}

bool isHash(const Json &value) {
    if (!value.is_string()) return false;
    const std::string hash = value.get<std::string>();
    return hash.size() == 64 && std::all_of(hash.begin(), hash.end(), [](char character) {
        return (character >= '0' && character <= '9') ||
               (character >= 'a' && character <= 'f');
    });
}

bool isVideoName(std::string_view name) {
    return !name.empty() && name.size() <= 8 &&
           std::all_of(name.begin(), name.end(), [](char character) {
               return (character >= 'A' && character <= 'Z') ||
                      (character >= 'a' && character <= 'z') ||
                      (character >= '0' && character <= '9');
           });
}

} // namespace

const RoomDescriptor &RoomDescriptors::at(int roomId) const {
    if (roomId < 0 || roomId >= static_cast<int>(_rooms.size()))
        throw std::runtime_error("MM6 room descriptor ID is out of range");
    return _rooms[roomId];
}

RoomDescriptors decodeRoomDescriptors(const Blob &source) {
    if (source.size() > 65536)
        throw std::runtime_error("MM6 room descriptor manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.size() != 4 || document.at("format") != 1 ||
        !isHash(document.at("source_report_sha256")) ||
        !isHash(document.at("source_house_table_sha256")))
        throw std::runtime_error("Invalid MM6 room descriptor manifest header");

    const Json &rows = document.at("rooms");
    if (!rows.is_array() || rows.size() != 119)
        throw std::runtime_error("MM6 room descriptor manifest requires 119 rows");
    RoomDescriptors candidate;
    std::unordered_set<std::string> names;
    for (int roomId = 0; roomId < 119; ++roomId) {
        const Json &row = rows[roomId];
        if (!row.is_object() || row.size() != 6 || checkedNumber(row.at("id"), 0, 118) != roomId ||
            !row.at("video_name").is_string())
            throw std::runtime_error("MM6 room descriptor row has invalid shape or ID");
        RoomDescriptor &room = candidate._rooms[roomId];
        room.videoName = row.at("video_name").get<std::string>();
        if ((roomId == 0 && !room.videoName.empty()) || (roomId != 0 && !isVideoName(room.videoName)))
            throw std::runtime_error("MM6 room descriptor has invalid video name");
        if (roomId != 0 && !names.insert(ascii::toLower(room.videoName)).second)
            throw std::runtime_error("MM6 room descriptor has duplicate video name");
        room.background = static_cast<std::uint8_t>(checkedNumber(row.at("background"), 0, 255));
        room.npcPicture = static_cast<std::uint16_t>(checkedNumber(row.at("npc_picture"), 0, 65535));
        room.rawHouseType = static_cast<std::uint8_t>(checkedNumber(row.at("house_type"), 0, 255));
        room.soundGroup = static_cast<std::uint8_t>(checkedNumber(row.at("sound_group"), 0, 255));
    }
    return candidate;
}

} // namespace mm6
