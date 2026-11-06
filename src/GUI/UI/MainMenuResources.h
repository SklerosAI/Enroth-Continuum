#pragma once

#include <array>
#include <string_view>

#include "Engine/GameTitle.h"

#include "Library/Geometry/Point.h"

struct MainMenuButtonResource {
    std::string_view icon;
    Pointi position;
};

struct MainMenuResources {
    std::string_view background;
    std::array<MainMenuButtonResource, 4> buttons; // New, load, credits, exit.
};

inline constexpr MainMenuResources mainMenuResources(GameTitle title) {
    if (title == GameTitle::MM6) {
        return {
            "title.pcx",
            {{{"start00a", {482, 9}}, {"start00b", {482, 71}},
              {"start00c", {482, 133}}, {"start00d", {482, 195}}}}
        };
    }

    return {
        "title.pcx",
        {{{"title_new", {495, 172}}, {"title_load", {495, 227}},
          {"title_cred", {495, 282}}, {"title_exit", {495, 337}}}}
    };
}
