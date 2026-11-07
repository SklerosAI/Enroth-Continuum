#pragma once

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Detached candidate for a future EC6 manifest. V2 deliberately rejects it.
GameplayRandomState captureGameplayRngFields(
    const EngineRandomComponent &random, GameTitle title);
SaveSection encodeGameplayRngSection(const GameplayRandomState &state,
                                     GameTitle title);
GameplayRandomState decodeGameplayRngSection(const SaveContainer &container,
                                             GameTitle title);

} // namespace mm6
