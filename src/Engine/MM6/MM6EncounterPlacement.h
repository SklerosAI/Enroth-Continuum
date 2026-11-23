#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>

#include "Engine/Graphics/Indoor.h"

#include "Library/Geometry/Vec.h"

#include "Utility/Math/TrigLut.h"

namespace mm6 {

/** Highest row of the A, B and C level odds table an MM6 encounter rolls on. */
constexpr int MAX_ENCOUNTER_LEVEL_ODDS = 4;

/**
 * @offset 0x455B76
 *
 * @param outdoor                       Whether the current map is outdoor.
 * @return                              Radius of the ring around an encounter spawn point that MM6 places the monsters
 *                                      after the first one on.
 */
constexpr int encounterRingRadius(bool outdoor) {
    return outdoor ? 128 : 64;
}

/**
 * MM6.exe's 16.16 fixed-point cosine, from the table built at 0x4451F0.
 *
 * @param angle                         Angle in 1/2048ths of a full circle.
 * @return                              Cosine times 65536, rounded.
 */
inline int fixedCos(int angle) {
    angle &= TrigTableLookup::uDoublePiMask;
    if (angle > TrigTableLookup::uIntegerPi)
        angle = TrigTableLookup::uIntegerDoublePi - angle;
    auto table = [](int index) {
        return static_cast<int>(std::cos(index * std::numbers::pi / TrigTableLookup::uIntegerPi) * 65536 + 0.5);
    };
    if (angle >= TrigTableLookup::uIntegerHalfPi)
        return -table(TrigTableLookup::uIntegerPi - angle);
    return table(angle);
}

/**
 * @param value                         Integer value.
 * @param fixed                         16.16 fixed-point factor.
 * @return                              Product rounded down, as MM6.exe's fixed-point multiply at 0x4453C0.
 */
inline int fixedMultiply(int value, int fixed) {
    return static_cast<int>((static_cast<int64_t>(value) * fixed) >> 16);
}

/**
 * @offset 0x455D40
 *
 * Where MM6 places the next monster of an encounter, at a point on the ring around the spawn point. Indoors the point
 * must be in the spawn point's sector and have a floor at most 1024 above or below the spawn point, else the monster
 * stands on the spawn point. Outdoors nothing is checked.
 *
 * @param spawn                         Spawn point position.
 * @param angle                         Angle in 1/2048ths of a full circle.
 * @param outdoor                       Whether the current map is outdoor.
 * @param spawnSector                   Indoor sector of the spawn point.
 * @return                              Position of the next monster.
 */
inline Vec3f encounterRingPosition(const Vec3f &spawn, int angle, bool outdoor, int spawnSector) {
    const int radius = encounterRingRadius(outdoor);
    Vec3f result(spawn.x + fixedMultiply(radius, fixedCos(angle)),
                 spawn.y + fixedMultiply(radius, fixedCos(angle - TrigTableLookup::uIntegerHalfPi)), spawn.z);
    if (outdoor)
        return result;

    if (pIndoor->GetSector(result) != spawnSector)
        return spawn;
    const float floorZ = BLV_GetFloorLevel(result, spawnSector);
    if (floorZ == -30000 || std::abs(floorZ - spawn.z) > 1024)
        return spawn;
    result.z = floorZ;
    return result;
}

} // namespace mm6
