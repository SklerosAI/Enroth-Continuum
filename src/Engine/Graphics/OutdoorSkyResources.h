#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

inline constexpr std::string_view mm6OutdoorBaseSky(GameTitle title, std::string_view baseSky) {
    return title == GameTitle::MM6 ? baseSky : std::string_view{};
}
