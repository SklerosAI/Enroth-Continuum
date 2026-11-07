#pragma once

#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Candidate for a future EC6 manifest. The current nineteen-section v2
// manifest deliberately rejects GNPC, so this cannot silently change saves.
struct GeneratedNpcRow {
    std::uint16_t actorSlot = 0;
    MonsterId monster = MONSTER_FIRST;
    NPCData npc;
};

struct GeneratedNpcSaveFields {
    std::string mapName;
    std::vector<GeneratedNpcRow> rows;
};

GeneratedNpcSaveFields captureGeneratedNpcFields(
    const std::deque<Actor> &actors, const NPCStats &npcs,
    std::string_view mapName, GameTitle title);

SaveSection encodeGeneratedNpcSection(const GeneratedNpcSaveFields &fields,
                                      GameTitle title);
GeneratedNpcSaveFields decodeGeneratedNpcSection(const SaveContainer &container,
                                                  GameTitle title);

// Compare the saved row-to-actor anchors against a rebuilt final map before
// any candidate would replace live generated rows. Does not prove RNG state.
void requireGeneratedNpcActorAgreement(const GeneratedNpcSaveFields &fields,
                                       const std::deque<Actor> &actors,
                                       std::string_view loadedMapName,
                                       GameTitle title);

} // namespace mm6
