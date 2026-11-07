#include "MM6NpcStaticBaseline.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "Engine/Resources/ResourceManager.h"

namespace mm6 {

NpcStaticBaseline loadNpcStaticBaseline(const Blob &npcData, const Blob &npcNews,
                                        const Blob &npcNames, const Blob &npcProfs,
                                        GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 NPC baseline requires MM6 title");
    auto fresh = std::make_unique<NPCStats>();
    fresh->InitializeMm6Static(npcData, npcNews, npcNames, npcProfs);
    if (fresh->uNumNewNPCs <= 0 || fresh->uNumNewNPCs > fresh->pOriginalNPCData.size())
        throw std::runtime_error("MM6 NPC baseline has invalid static row count");
    return NpcStaticBaseline(std::move(fresh));
}

NpcStaticBaseline loadNpcStaticBaseline(ResourceManager &resources, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 NPC baseline requires MM6 title");
    return loadNpcStaticBaseline(resources.eventsData("npcdata.txt"),
                                 resources.eventsData("npcnews.txt"),
                                 resources.eventsData("npcnames.txt"),
                                 resources.eventsData("npcprof.txt"), title);
}

} // namespace mm6
