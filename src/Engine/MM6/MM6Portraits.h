#pragma once

#include <optional>
#include <string>

namespace mm6 {

inline constexpr int kPortraitFaceCount = 12;
inline constexpr int kPortraitFrameCount = 53;
inline constexpr int kMasculineFaceCount = 8;

inline constexpr bool isValidPortraitFace(int face) {
    return face >= 0 && face < kPortraitFaceCount;
}

inline constexpr bool isFemininePortraitFace(int face) {
    return isValidPortraitFace(face) && face >= kMasculineFaceCount;
}

inline constexpr bool isValidPortraitFrame(int frame) {
    return frame >= 1 && frame <= kPortraitFrameCount;
}

// The owner's original executable orders eight masculine and four feminine
// frame families. Form the archive stem without carrying an asset inventory.
inline std::optional<std::string> portraitStem(int face) {
    if (!isValidPortraitFace(face))
        return std::nullopt;

    const int withinFamily = face < kMasculineFaceCount ? face : face - kMasculineFaceCount;
    std::string result = face < kMasculineFaceCount ? "male" : "girl";
    result += static_cast<char>('a' + withinFamily);
    return result;
}

}  // namespace mm6
