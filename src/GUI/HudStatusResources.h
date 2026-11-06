#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

inline constexpr std::string_view hudStatusBarIcon(GameTitle title, std::string_view mm7Icon) {
    return title == GameTitle::MM6 ? "footer" : mm7Icon;
}
