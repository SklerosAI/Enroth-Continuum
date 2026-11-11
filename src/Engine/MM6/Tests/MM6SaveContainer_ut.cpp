#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SaveContainer.h"

UNIT_TEST(MM6SaveContainer, RoundTripAndOptionalSection) {
    mm6::SaveContainer source;
    source.sections.push_back({{'H', 'E', 'A', 'D'}, true, {1, 2, 3}});
    source.sections.push_back({{'M', 'A', 'P', 'S'}, true, {0, 255, 8, 0}});
    source.sections.push_back({{'X', 'T', 'R', 'A'}, false, {7, 9}});

    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(source);
    EXPECT_EQ(mm6::decodeSaveContainer(bytes), source);
}

UNIT_TEST(MM6SaveContainer, RejectsWrongTitleVersionAndDamage) {
    mm6::SaveContainer source;
    source.sections.push_back({{'H', 'E', 'A', 'D'}, true, {1, 2, 3}});
    std::vector<std::uint8_t> good = mm6::encodeSaveContainer(source);

    std::vector<std::uint8_t> wrongTitle = good;
    wrongTitle[2] = '7';
    EXPECT_THROW((void) mm6::decodeSaveContainer(wrongTitle), std::exception);

    std::vector<std::uint8_t> future = good;
    future[8] = 8;
    EXPECT_THROW((void) mm6::decodeSaveContainer(future), std::exception);

    std::vector<std::uint8_t> truncated = good;
    truncated.pop_back();
    EXPECT_THROW((void) mm6::decodeSaveContainer(truncated), std::exception);

    std::vector<std::uint8_t> corrupt = good;
    corrupt.back() ^= 1;
    EXPECT_THROW((void) mm6::decodeSaveContainer(corrupt), std::exception);

    std::vector<std::uint8_t> appended = good;
    appended.push_back(0);
    EXPECT_THROW((void) mm6::decodeSaveContainer(appended), std::exception);
}

UNIT_TEST(MM6SaveContainer, RejectsUnknownRequiredAndDuplicateSections) {
    mm6::SaveContainer unknown;
    unknown.sections.push_back({{'X', 'T', 'R', 'A'}, true, {1}});
    EXPECT_THROW((void) mm6::decodeSaveContainer(mm6::encodeSaveContainer(unknown)), std::exception);

    mm6::SaveContainer duplicate;
    duplicate.sections.push_back({{'H', 'E', 'A', 'D'}, true, {1}});
    duplicate.sections.push_back({{'H', 'E', 'A', 'D'}, true, {2}});
    EXPECT_THROW((void) mm6::encodeSaveContainer(duplicate), std::exception);

    mm6::SaveContainer empty;
    EXPECT_THROW((void) mm6::encodeSaveContainer(empty), std::exception);
}
