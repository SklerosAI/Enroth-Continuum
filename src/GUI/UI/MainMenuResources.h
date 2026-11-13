#pragma once

#include <array>
#include <string_view>

#include "Engine/Data/SoundEnums.h"
#include "Engine/GameTitle.h"

#include "Library/Geometry/Point.h"
#include "Library/Geometry/Size.h"

struct MainMenuButtonResource {
    std::string_view icon; // Drawn while the button is hovered.
    std::string_view pressedIcon;
    Pointi position;
};

/**
 * Art, hit boxes and music of the title screen.
 *
 * MM6 animates the hovered button through `start00a`..`start06a` and back, and its hit boxes are smaller than the
 * 135x45 button art. MM7 draws a still hover image and sizes its hit boxes from it.
 */
struct MainMenuResources {
    std::string_view background;
    std::array<MainMenuButtonResource, 4> buttons; // New, load, credits, exit.
    Sizei buttonSize; // Empty means the hover art's size.
    int hoverFrameCount = 1;
    MusicId music = MUSIC_MAIN_MENU;
    bool exitHasHotkey = true;
};

inline constexpr MainMenuResources mainMenuResources(GameTitle title) {
    if (title == GameTitle::MM6) {
        return {
            "title.pcx",
            {{{"start00a", "mmnew1", {482, 9}}, {"start00b", "mmloa1", {482, 71}},
              {"start00c", "mmcre1", {482, 133}}, {"start00d", "mmesc1", {482, 195}}}},
            {132, 44},
            7,
            MusicId(13), // MM6 starts this CD track 18.5 s in.
            false
        };
    }

    return {
        "title.pcx",
        {{{"title_new", "title_new", {495, 172}}, {"title_load", "title_load", {495, 227}},
          {"title_cred", "title_cred", {495, 282}}, {"title_exit", "title_exit", {495, 337}}}}
    };
}
