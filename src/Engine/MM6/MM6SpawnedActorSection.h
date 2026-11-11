#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

class Actor;
struct Actor_MM7;

namespace mm6 {

// Detached EC6-only candidate. The byte stream is not a native MM6 DDM actor
// record and is not part of the production raw-save manifest.
std::vector<std::uint8_t> encodeSpawnedActorSection(
    std::span<const Actor> actors, size_t firstSlot, GameTitle title);

// Exact slots are assigned only in the returned vector. No world is mutated.
std::vector<Actor> decodeSpawnedActorSection(
    std::span<const std::uint8_t> bytes, size_t expectedFirstSlot, GameTitle title);

// V5's detached runtime image shares ACTR's exact scalar projection, but its
// separate ARTR section carries fields this native-shaped image narrows.
Actor_MM7 snapshotActorRuntimeImageForV5(const Actor &actor,
                                         std::size_t slot, GameTitle title);

/**
 * @param actor                         Any actor.
 * @return                              Its `Actor_MM7` image as the MM7 snapshot writes it, but with an empty name, so
 *                                      no NPC or monster table is read.
 */
Actor_MM7 snapshotActorImageWithoutName(const Actor &actor);

} // namespace mm6
