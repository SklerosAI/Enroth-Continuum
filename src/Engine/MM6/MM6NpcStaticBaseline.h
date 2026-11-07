#pragma once

#include <memory>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Tables/NPCTable.h"
#include "Utility/Memory/Blob.h"

class ResourceManager;

namespace mm6 {

// A fresh NPCStats-owned MM6 table image, built from MM6 source tables rather
// than from the current process NPCStats. RESC overlays its mutable records.
class NpcStaticBaseline {
 public:
    NpcStaticBaseline(NpcStaticBaseline &&) noexcept = default;
    NpcStaticBaseline &operator=(NpcStaticBaseline &&) noexcept = default;
    NpcStaticBaseline(const NpcStaticBaseline &) = delete;
    NpcStaticBaseline &operator=(const NpcStaticBaseline &) = delete;

    const NPCStats &stats() const { return *_stats; }

 private:
    explicit NpcStaticBaseline(std::unique_ptr<NPCStats> stats) : _stats(std::move(stats)) {}
    friend NpcStaticBaseline loadNpcStaticBaseline(const Blob &, const Blob &,
                                                  const Blob &, const Blob &, GameTitle);
    std::unique_ptr<NPCStats> _stats;
};

// The four NPCStats-owned MM6 source tables. Global pNPCTopics, map-generated
// NPCs and live dialogue state are separate owners, not copied by this seam.
NpcStaticBaseline loadNpcStaticBaseline(const Blob &npcData, const Blob &npcNews,
                                        const Blob &npcNames, const Blob &npcProfs,
                                        GameTitle title);
NpcStaticBaseline loadNpcStaticBaseline(ResourceManager &resources, GameTitle title);


} // namespace mm6
