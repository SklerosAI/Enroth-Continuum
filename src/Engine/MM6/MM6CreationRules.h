#pragma once

#include <array>
#include <cstdint>

class Blob;

namespace mm6 {

inline constexpr int kClassCount = 18;
inline constexpr int kClassFamilyCount = 6;
inline constexpr int kClassTiers = 3;
inline constexpr int kSkillCount = 31;
inline constexpr int kAttributeCount = 7;

enum class ManaAttribute {
    None,
    Personality,
    Intellect,
    Both
};

inline constexpr bool isValidClass(int classId) {
    return classId >= 0 && classId < kClassCount;
}

inline constexpr int classFamily(int classId) {
    return classId / kClassTiers;
}

inline constexpr bool isStartingClass(int classId) {
    return isValidClass(classId) && classId % kClassTiers == 0;
}

inline constexpr ManaAttribute manaAttribute(int classId) {
    if (!isValidClass(classId) || classId < 3)
        return ManaAttribute::None;
    if (classId < 6 || (classId >= 9 && classId < 12))
        return ManaAttribute::Personality;
    if (classId < 15)
        return ManaAttribute::Intellect;
    return ManaAttribute::Both;
}

struct CreationRules {
    std::array<int, kClassFamilyCount> hpBase{};
    std::array<int, kClassFamilyCount> spBase{};
    std::array<int, kClassCount> hpFactor{};
    std::array<int, kClassCount> spFactor{};
    std::array<std::array<int, kAttributeCount>, kClassFamilyCount> startingStats{};
    std::array<std::array<uint8_t, kSkillCount>, kClassFamilyCount> startingSkills{};
};

// Reads only a private, generated profile manifest. Does not open the original
// game executable or publish its creation arrays in the source tree.
void loadCreationRules(const Blob &source);
const CreationRules &creationRules();

}  // namespace mm6
