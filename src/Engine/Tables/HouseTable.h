#pragma once

#include <string_view>

#include "Utility/IndexedArray.h"

#include "Engine/Data/HouseData.h"
#include "Engine/GameTitle.h"

class Blob;

void initializeHouses(const Blob &houses, GameTitle gameTitle = GameTitle::MM7);
HouseType parseHouseType(std::string_view label, GameTitle gameTitle);

extern IndexedArray<HouseData, HOUSE_FIRST, HOUSE_LAST> houseTable;
