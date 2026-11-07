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

// Exit-picture indices come from the retained MM6 executable's eight-pointer
// table, not the MM7 pHouse_ExitPictures array. Text is supplied from the
// currently loaded title's trans.txt; this helper never copies game prose.
TransitionDialogPlan planIndoorTransition(GameTitle title, HouseId houseId,
                                          unsigned exitPictureId,
                                          std::string_view transitionText);

} // namespace mm6
