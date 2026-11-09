#pragma once

#include <array>
#include <optional>
#include <string>

#include "Engine/MM6/MM6Portraits.h"
#include "Library/Geometry/Point.h"

namespace mm6 {

struct PaperdollBaseArt {
    std::string body;
    std::string arm1;
    std::string arm2;
    Pointi bodyOffset;
    Pointi arm1Offset;
    Pointi arm2Offset;
};

// Face order agrees with MM6Portraits.h. These offsets are relative to the
// original paperdoll origin (481, 0). Source: GrayFace MMExtension's MM6
// PaperDol.txt at commit 9d73c0ff0482bd231ea522f752f4fe9b26640925.
inline std::optional<PaperdollBaseArt> paperdollBaseArt(int face) {
    if (!isValidPortraitFace(face))
        return std::nullopt;

    static constexpr std::array<Pointi, kPortraitFaceCount> bodyOffsets = {{
        {14, 52}, {14, 53}, {14, 52}, {14, 52}, {14, 52}, {14, 52},
        {13, 53}, {14, 51}, {16, 52}, {12, 52}, {14, 52}, {14, 51},
    }};
    static constexpr std::array<Pointi, kPortraitFaceCount> arm1Offsets = {{
        {106, 106}, {98, 100}, {93, 107}, {107, 102}, {111, 102}, {103, 105},
        {101, 100}, {104, 104}, {104, 104}, {102, 103}, {105, 102}, {103, 101},
    }};
    static constexpr std::array<Pointi, kPortraitFaceCount> arm2Offsets = {{
        {32, 104}, {32, 99}, {32, 103}, {32, 102}, {32, 100}, {32, 104},
        {32, 107}, {32, 106}, {32, 105}, {32, 104}, {32, 107}, {32, 101},
    }};

    const int familyIndex = face < kMasculineFaceCount ? face : face - kMasculineFaceCount;
    std::string stem = face < kMasculineFaceCount ? "ml" : "grl";
    stem += static_cast<char>('a' + familyIndex);
    return PaperdollBaseArt{stem + "bod", stem + "arm1", stem + "arm2",
                            bodyOffsets[face], arm1Offsets[face], arm2Offsets[face]};
}

}  // namespace mm6
