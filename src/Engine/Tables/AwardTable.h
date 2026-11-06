#pragma once

#include <array>
#include <string_view>

#include "Engine/Data/AwardData.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/GameTitle.h"

#include "Utility/IndexedArray.h"

class Blob;

/**
 * @offset 0x4763E0
 */
void initializeAwards(const Blob &awards, GameTitle gameTitle = GameTitle::MM7);
bool canRenderAwardText(GameTitle gameTitle, std::string_view text);

extern IndexedArray<AwardData, AWARD_FIRST, AWARD_LAST> pAwards;
