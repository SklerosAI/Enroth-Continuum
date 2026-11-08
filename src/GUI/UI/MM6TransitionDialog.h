#pragma once

#include <string_view>

#include "Engine/Data/HouseEnums.h"
#include "Engine/GameTitle.h"

namespace mm6 {

struct TransitionDialogPlan {
    std::string_view backgroundIcon;
    std::string_view destinationIcon;
    std::string_view confirmIcon;
    std::string_view cancelIcon;
    int transitionStringId = 0;
};

// One-based exit-picture IDs select the retained MM6 executable's eight-pointer
// table; zero means no icon. This is not the MM7 pHouse_ExitPictures array.
// Text comes from the loaded title's trans.txt; this helper never copies prose.
TransitionDialogPlan planIndoorTransition(GameTitle title, HouseId houseId,
                                          unsigned exitPictureId,
                                          std::string_view transitionText);

} // namespace mm6
