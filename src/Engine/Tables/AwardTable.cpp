#include "AwardTable.h"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

IndexedArray<AwardData, AWARD_FIRST, AWARD_LAST> pAwards;

bool canRenderAwardText(GameTitle gameTitle, std::string_view text) {
    return gameTitle != GameTitle::MM6 || text.find('%') == std::string_view::npos;
}

static void initializeMm6Awards(const Blob &awards) {
    // MM6's 100 shipped rows have text but no priority value. Preserve their
    // text while leaving ordering/color priority at the neutral zero value.
    IndexedArray<AwardData, AWARD_FIRST, AWARD_LAST> staged{};
    std::array<bool, 101> seen{};
    for (std::string_view line : split(awards.str()).by("\r\n").drop(1).skip("")) {
        std::vector<std::string_view> cells = split(line).by('\t');
        if (cells.size() < 2 || cells.size() > 3)
            throw std::runtime_error("Invalid MM6 award row width");
        int id = fromString<int>(cells[0]);
        if (id < 1 || id > 100 || seen[id])
            throw std::runtime_error("Invalid or duplicate MM6 award ID");
        if (cells.size() == 3 && !cells[2].empty())
            throw std::runtime_error("MM6 award priority is not established");
        seen[id] = true;
        staged[static_cast<AwardId>(id)].pText = unquote(cells[1]);
    }
    for (int id = 1; id <= 100; ++id)
        if (!seen[id])
            throw std::runtime_error("Missing MM6 award row");
    pAwards = std::move(staged);
}

void initializeAwards(const Blob &awards, GameTitle gameTitle) {
    if (gameTitle == GameTitle::MM6) {
        initializeMm6Awards(awards);
        return;
    }
    // awards.txt table structure: index | text (localized) | priority.
    for (std::string_view line : split(awards.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 3> chunks = split(line).by('\t');
        if (chunks[2].empty())
            continue; // Truncated lines with just the index exist in the file.

        AwardId awardId = static_cast<AwardId>(fromString<int>(chunks[0]));
        pAwards[awardId].pText = unquote(chunks[1]);
        pAwards[awardId].uPriority = fromString<int>(chunks[2]);
    }
}
