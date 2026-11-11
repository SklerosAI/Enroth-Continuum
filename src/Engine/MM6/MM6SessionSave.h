#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6ParkedMapSection.h"
#include "MM6RawSaveFields.h"
#include "MM6RawSessionStage.h"

namespace mm6 {

inline constexpr std::uint16_t SESSION_SAVE_VERSION = 7;

/**
 * EC6 v7, a save on any map. The nineteen v2 raw owners hold the party, and `MAPS` the native delta of every map with
 * the visited maps as the party left them. `PRKM` holds the actors, ground items and event variables of the active map
 * and of every visited map.
 */
struct SessionSaveFields {
    RawSaveFields raw;
    std::vector<ParkedMapRuntime> maps;
};

std::vector<std::uint8_t> encodeSessionSave(const SessionSaveFields &fields, GameTitle title);

/**
 * @param bytes                         Encoded save.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Decoded fields. Throws for another container version, a missing or extra
 *                                      section, or a malformed owner.
 */
SessionSaveFields decodeSessionSave(std::span<const std::uint8_t> bytes, GameTitle title);

struct StagedSessionSave {
    StagedRawSessionFields represented;
    std::vector<ParkedMapRuntime> maps;
};

/**
 * Stages a v7 save against the loaded MM6 archive and tables, the way its load would publish it. Read-only.
 *
 * @param bytes                         Encoded save.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Staged owners. Throws when the save can't be loaded as it is.
 */
std::unique_ptr<StagedSessionSave> stageSessionSave(std::span<const std::uint8_t> bytes, GameTitle title);

/**
 * Captures the running MM6 session on whatever map the party is on. Read-only.
 *
 * @param sessionTemplate               Raw owners the session was loaded or created with. The inventory capture keeps
 *                                      their item records where the items didn't change.
 * @param displayName                   Title shown in the save list.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Encoded v7 save. Throws for a session the save can't hold, such as an item on
 *                                      the cursor or a map change in flight.
 */
std::vector<std::uint8_t> captureSessionSave(const RawSaveFields &sessionTemplate, std::string_view displayName,
                                             GameTitle title);

/**
 * Publishes a staged v7 save to a fresh MM6 session: the party and world owners, and every map of the save as a parked
 * map. Then requests the active map with a savegame load, which restores it like a return. Owners published before a
 * throw stay partial.
 *
 * @param staged                        Staged save, its owners are moved out.
 * @param title                         Game title, only MM6 is accepted.
 */
void publishSessionSave(StagedSessionSave *staged, GameTitle title);

} // namespace mm6
