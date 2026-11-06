#pragma once

#include "Engine/GameTitle.h"

namespace mm6 {

enum class CreationVitalStage { BeforeStartingItems, AfterStartingItems };

// MM6's original creation finalizer fills current HP/SP after all starting
// item grants. Preserve the inherited order for other titles.
template <class CharacterT>
void refillCreationVitalsAtStage(GameTitle title, CreationVitalStage stage,
                                 CharacterT *character) {
    if ((title == GameTitle::MM6) != (stage == CreationVitalStage::AfterStartingItems))
        return;
    character->health = character->GetMaxHealth();
    character->mana = character->GetMaxMana();
}

}  // namespace mm6
