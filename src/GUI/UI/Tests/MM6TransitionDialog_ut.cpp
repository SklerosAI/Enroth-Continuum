#include <array>
#include <stdexcept>
#include <string_view>

#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/MM6TransitionDialog.h"

UNIT_TEST(MM6TransitionDialog, UsesOriginalExitPicturesAndTitleLocalArt) {
    static constexpr std::array<std::string_view, 8> expectedIcons = {
        "castle", "dungeon", "idoor", "isecdoor",
        "istairdn", "istairup", "itrap", "outside"
    };
    for (unsigned id = 0; id < expectedIcons.size(); ++id) {
        const mm6::TransitionDialogPlan plan = mm6::planIndoorTransition(
            GameTitle::MM6, static_cast<HouseId>(171), id, "present in MM6 trans.txt");
        EXPECT_EQ(plan.backgroundIcon, "backevt");
        EXPECT_EQ(plan.destinationIcon, expectedIcons[id]);
        EXPECT_EQ(plan.confirmIcon, "buttyes2");
        EXPECT_EQ(plan.cancelIcon, "buttesc2");
        EXPECT_EQ(plan.transitionStringId, 171);
    }
}

UNIT_TEST(MM6TransitionDialog, RejectsForeignOrUnprovedRows) {
    const mm6::TransitionDialogPlan iconOnly = mm6::planIndoorTransition(
        GameTitle::MM6, HOUSE_INVALID, 5, "");
    EXPECT_EQ(iconOnly.destinationIcon, "istairup");
    EXPECT_EQ(iconOnly.transitionStringId, 0);

    EXPECT_THROW(mm6::planIndoorTransition(GameTitle::MM7,
                                           static_cast<HouseId>(171), 1, "text"),
                 std::invalid_argument);
    EXPECT_THROW(mm6::planIndoorTransition(GameTitle::MM6,
                                           static_cast<HouseId>(171), 8, "text"),
                 std::invalid_argument);
    EXPECT_THROW(mm6::planIndoorTransition(GameTitle::MM6,
                                           static_cast<HouseId>(171), 1, ""),
                 std::invalid_argument);
    EXPECT_THROW(mm6::planIndoorTransition(GameTitle::MM6, HOUSE_INVALID, 0, ""),
                 std::invalid_argument);
}
