#include "MM6PreEventGeneratedNpcReplay.h"

#include <deque>
#include <stdexcept>

#include "MM6GameplayRngSaveFields.h"
#include "MM6NativeActorNpcPolicy.h"

namespace mm6 {

PreEventGeneratedNpcReplay stagePreEventGeneratedNpcReplay(
    const GeneratedNpcSaveFields &savedGenerated,
    const GameplayRandomState &savedRng,
    const std::deque<Actor> &rebuiltActors,
    const NPCStats &freshNpcs,
    std::string_view loadedMapName,
    RandomEngineType selectedRngType,
    GameTitle title) {
    if (title != GameTitle::MM6 || freshNpcs.uNewlNPCBufPos != 0 ||
        savedGenerated.mapName != loadedMapName ||
        savedRng.type != selectedRngType)
        throw std::runtime_error("MM6 pre-event generated owner identity differs");
    // Reuse the standalone codecs' complete field validation. No live owner is
    // mutated, and neither container is made admissible to the strict v2 save.
    (void) encodeGeneratedNpcSection(savedGenerated, title);
    (void) encodeGameplayRngSection(savedRng, title);

    size_t expectedRow = 0;
    for (size_t slot = 0; slot < rebuiltActors.size(); ++slot) {
        const Actor &actor = rebuiltActors[slot];
        const MonsterId monster = actor.monsterInfo.id;
        if (actor.id != slot || monster < MONSTER_FIRST ||
            monster > MONSTER_LAST)
            throw std::runtime_error("MM6 pre-event actor identity is invalid");

        if (!getsStreetNpc(monster, actor.npcId)) {
            if (actor.npcId >= 5000)
                throw std::runtime_error("MM6 pre-event ineligible actor has generated link");
            continue;
        }

        if (expectedRow >= savedGenerated.rows.size())
            throw std::runtime_error("MM6 pre-event generated actor lacks saved row");
        const GeneratedNpcRow &row = savedGenerated.rows[expectedRow];
        if (row.actorSlot != slot || row.monster != monster ||
            (actor.npcId >= 5000 && actor.npcId != 5000 + expectedRow))
            throw std::runtime_error("MM6 pre-event generated actor anchor differs");
        ++expectedRow;
    }
    if (expectedRow != savedGenerated.rows.size())
        throw std::runtime_error("MM6 pre-event saved generated row lacks actor");
    return {savedGenerated, savedRng, expectedRow};
}

} // namespace mm6
