#pragma once

#include <array>
#include <memory>
#include <string_view>
#include <variant>

#include "Engine/GameTitle.h"

struct Party;

namespace mm6 {

// Guardian Angel is not yet mapped to a runtime MM6 buff slot. The caller
// must supply its independently verified state, including absence.
enum class GuardianAngelState { Absent, Expert, Master, Unknown };

enum class PartyDeathRefusal {
    ForeignTitle,
    UnknownGuardianAngel,
    UnmappedBuff,
    UnmappedCondition,
    InvalidGold,
    InvalidMaximumHealth,
    DeathCountOverflow,
    PlayingTimeOverflow
};

struct PartyDeathPlan {
    std::unique_ptr<Party> party;
    std::string_view mapFile = "oute3.odm";
};

using PartyDeathResult = std::variant<PartyDeathPlan, PartyDeathRefusal>;

// Detached result only. The caller owns movie, NPC/actor cleanup, map
// transition and any award once its MM6 mapping is proved.
PartyDeathResult planPartyDeath(const Party &source, GameTitle title,
                               GuardianAngelState guardianAngel,
                               const std::array<int, 4> &maximumHealth);

} // namespace mm6
