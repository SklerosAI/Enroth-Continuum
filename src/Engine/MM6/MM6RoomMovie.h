#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>

#include "Utility/Memory/Blob.h"

namespace mm6 {

inline constexpr int MM6_HOUSE_KINGS_LIBRARY = 168; // Archibald's statue.
inline constexpr int MM6_HOUSE_KINGS_LIBRARY_ARCHIBALD = 553; // Archibald, awake.
inline constexpr int MM6_HOUSE_KINGS_LIBRARY_EMPTY = 554;

/**
 * @param movie                         Smacker file.
 * @return                              Milliseconds that one pass of the movie plays, 0 for anything that isn't a
 *                                      Smacker file.
 */
inline int64_t roomMovieLength(const Blob &movie) {
    if (movie.size() < 20 || !movie.str().starts_with("SMK"))
        return 0;
    uint32_t frames = 0;
    int32_t frameRate = 0;
    std::memcpy(&frames, static_cast<const char *>(movie.data()) + 12, sizeof(frames));
    std::memcpy(&frameRate, static_cast<const char *>(movie.data()) + 16, sizeof(frameRate));
    // A positive rate is milliseconds per frame, a negative one hundredths of them, and 0 means 10 frames a second.
    const double frameMs = frameRate > 0 ? frameRate : frameRate < 0 ? -frameRate / 100.0 : 100.0;
    return std::llround(frames * frameMs);
}

/**
 * @offset 0x43C300
 *
 * @param houseId                       House the party enters.
 * @param carriesBell                   Whether a character carries Tanir's Bell.
 * @return                              Movie the room plays once in place of its looping one, empty for a room that
 *                                      loops its own. The King's Library plays `archie` when the party brings the
 *                                      bell.
 */
inline std::optional<std::string_view> oneShotRoomMovie(int houseId, bool carriesBell) {
    if (houseId == MM6_HOUSE_KINGS_LIBRARY && carriesBell)
        return "archie";
    return std::nullopt;
}

/**
 * @offset 0x4A6113
 *
 * @param houseId                       House the party is in.
 * @return                              House the party is in once the room movie has played out, empty for a room
 *                                      that stays.
 */
inline std::optional<int> roomAfterMovie(int houseId) {
    if (houseId == MM6_HOUSE_KINGS_LIBRARY)
        return MM6_HOUSE_KINGS_LIBRARY_ARCHIBALD;
    if (houseId == MM6_HOUSE_KINGS_LIBRARY_ARCHIBALD)
        return MM6_HOUSE_KINGS_LIBRARY_EMPTY;
    return std::nullopt;
}

/**
 * @offset 0x43CD61
 *
 * @param houseId                       House the party is in when a script moves an NPC.
 * @return                              Whether the move lets the room movie play out instead of looping, as it does
 *                                      in the room where Archibald waits.
 */
inline bool npcMoveStopsRoomLoop(int houseId) {
    return houseId == MM6_HOUSE_KINGS_LIBRARY_ARCHIBALD;
}

} // namespace mm6
