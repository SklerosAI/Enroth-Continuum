#pragma once

#include <array>
#include <string_view>

class FileSystem;

inline constexpr std::array<std::string_view, 9> mm6HeadlessRuleNames = {
    "creation.json", "spells.json", "first-aid.json", "power-cure.json",
    "timed-cures.json", "combat-buffs.json", "house-rooms.json", "transport.json",
    "shop-stock.json"
};

void seedMm6HeadlessRules(const FileSystem &source, FileSystem *destination);
