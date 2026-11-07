#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Detached full ordered actor runtime truth. V4 ACTR remains an unchanged
// source anchor; the v5 ARTR payload carries exact dynamic fields separately.
struct ActorRuntimeSaveFields {
    std::string mapName;
    std::vector<Actor> actors;
    // IART only: ordered native IACR prefix, followed by generated actors.
    std::uint32_t nativePrefixCount = 0;
};

enum class ActorRuntimeSectionKind { OutdoorV5, IndoorV6 };

SaveSection encodeActorRuntimeSectionAs(
    const ActorRuntimeSaveFields &fields, ActorRuntimeSectionKind kind,
    GameTitle title);
ActorRuntimeSaveFields decodeActorRuntimeSectionAs(
    const SaveContainer &container, ActorRuntimeSectionKind kind,
    GameTitle title);

ActorRuntimeSaveFields captureActorRuntimeFields(
    std::string_view mapName, std::span<const std::uint8_t> baselineActr,
    std::span<const Actor> live, GameTitle title);
SaveSection encodeActorRuntimeSection(const ActorRuntimeSaveFields &fields,
                                      GameTitle title);
ActorRuntimeSaveFields decodeActorRuntimeSection(const SaveContainer &container,
                                                 GameTitle title);
void requireActorRuntimeBaselineAgreement(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    std::span<const std::uint8_t> baselineActr, GameTitle title);

} // namespace mm6
