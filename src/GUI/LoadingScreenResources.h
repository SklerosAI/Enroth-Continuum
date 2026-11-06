#pragma once

#include <string>
#include <string_view>

#include "Engine/GameTitle.h"

struct LoadingScreenResources {
    std::string background;
    std::string_view progress;
};

inline LoadingScreenResources fullscreenLoadingResources(GameTitle title, int variation) {
    if (title == GameTitle::MM6)
        return {"loading.pcx", {}};
    return {"loading" + std::to_string(variation) + ".pcx", "loadprog"};
}
