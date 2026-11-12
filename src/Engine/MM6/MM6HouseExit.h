#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <string_view>

#include "Engine/MapEnums.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"

#include "Utility/IndexedBitset.h"

namespace mm6 {

/**
 * A house's way out to another map, columns 20 to 22 of 2DEvents.txt.
 */
struct HouseExit {
    int picture = 0; // One-based index into the table of `houseExitIcon`, 0 for a house without an exit.
    int map = 0; // Mapstats row of the destination.
    int restriction = 0; // Above 0 a quest bit the exit needs, below 0 a sewer arrival, see `houseExitDestination`.
};

/**
 * @param text                          Cell of column 20, 21 or 22.
 * @return                              The cell the way the original's table loader reads it, with `atoi` into a
 *                                      signed word. Text reads as the number it starts with, so "Need Key" is 0.
 */
inline int houseExitField(std::string_view text) {
    size_t pos = 0;
    while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t'))
        ++pos;
    int sign = 1;
    if (pos < text.size() && (text[pos] == '-' || text[pos] == '+'))
        sign = text[pos++] == '-' ? -1 : 1;
    int value = 0;
    for (; pos < text.size() && text[pos] >= '0' && text[pos] <= '9' && value < 0x10000; ++pos)
        value = value * 10 + (text[pos] - '0');
    return static_cast<int16_t>(sign * value);
}

/**
 * @offset 0x43C140
 *
 * @param exit                          House's exit.
 * @param questBits                     Party's quest bits.
 * @return                              Whether the house lists its exit after the people in it. An exit with a quest
 *                                      bit shows only once the bit is set.
 */
inline bool isHouseExitOffered(const HouseExit &exit, const IndexedBitset<QBIT_FIRST, QBIT_LAST> &questBits) {
    if (exit.picture == 0)
        return false;
    return exit.restriction <= 0 || questBits[static_cast<QuestBit>(exit.restriction)];
}

/**
 * @offset 0x4BEFF8
 *
 * @param picture                       Exit picture, 1 to 8.
 * @return                              Icon the exit's portrait shows.
 */
inline std::string_view houseExitIcon(int picture) {
    static constexpr std::array<std::string_view, 8> icons = {
        "castle", "dungeon", "idoor", "isecdoor", "istairdn", "istairup", "itrap", "outside"
    };
    assert(picture >= 1 && picture <= std::ssize(icons));
    return icons[picture - 1];
}

/**
 * @offset 0x42EFE2
 *
 * @param exit                          Exit the party takes.
 * @return                              Where the party arrives. A restriction of -n picks the n-th arrival of a table
 *                                      the original keeps for the Free Haven sewer, any other one the map's party
 *                                      start.
 */
inline MapDestination houseExitDestination(const HouseExit &exit) {
    struct Arrival {
        int x;
        int y;
        int z;
        int yaw;
    };
    static constexpr std::array<Arrival, 6> arrivals = {{ // x, y, z and yaw tables at 0x4BE3B8, 0x4BE3D0, 0x4BE3E8 and 0x4BE400.
        {-6575, 13740, 177, 1536},
        {4234, 13224, -767, 512},
        {8316, 5128, 273, 512},
        {8853, 764, 1, 1024},
        {7557, -4224, 257, 1536},
        {-8171, -7633, 17, 512}
    }};
    const MapId map = static_cast<MapId>(exit.map);
    if (exit.restriction >= 0)
        return MapDestination(map, MAP_START_POINT_PARTY);
    assert(-exit.restriction <= std::ssize(arrivals));
    const Arrival &arrival = arrivals[-exit.restriction - 1];
    return MapDestination(map, PartyPlacement(Vec3f(arrival.x, arrival.y, arrival.z), arrival.yaw, 0, 0));
}

} // namespace mm6
