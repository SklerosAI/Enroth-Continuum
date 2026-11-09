#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "Engine/MM6/MM6Portraits.h"
#include "Library/Geometry/Point.h"
#include "Utility/String/Ascii.h"

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

struct PaperdollLayer {
    std::string_view image;
    Pointi offset;
};

struct PaperdollArmorArt {
    std::string_view itemPicture;
    PaperdollLayer body;
    PaperdollLayer arm1;
    PaperdollLayer arm2;
};

// Armor layers and offsets from the same pinned public MM6 PaperDol.txt table.
// The body layer sits in front of boots; arm1/arm2 replace the bare arm pose.
inline constexpr std::array<PaperdollArmorArt, 13> kPaperdollArmorArt = {{
    {"lr1icon", {"LR1BOD", {53, 98}}, {"LR1ARM1", {88, 98}}, {"LR1ARM2", {82, 98}}},
    {"lr2icon", {"LR2BOD", {35, 94}}, {"LR2ARM1", {87, 96}}, {"LR2ARM2", {87, 96}}},
    {"lr3icon", {"LR3BOD", {33, 93}}, {"LR3ARM1", {88, 97}}, {"LR3ARM2", {88, 99}}},
    {"lr4icon", {"LR4BOD", {29, 92}}, {"LR4ARM1", {88, 96}}, {"LR4ARM2", {91, 95}}},
    {"lr5icon", {"LR5BOD", {44, 93}}, {"LR5ARM1", {96, 96}}, {"LR5ARM2", {85, 97}}},
    {"chn1icon", {"CHN1BOD", {42, 92}}, {"CHN1ARM1", {96, 96}}, {"CHN1ARM2", {84, 95}}},
    {"chn2icon", {"CHN2BOD", {40, 91}}, {"CHN2ARM1", {95, 95}}, {"CHN2ARM2", {83, 94}}},
    {"chn3icon", {"CHN3BOD", {25, 94}}, {"CHN3ARM1", {87, 95}}, {"CHN3ARM2", {87, 94}}},
    {"chn4icon", {"CHN4BOD", {27, 94}}, {"CHN4ARM1", {88, 97}}, {"CHN4ARM2", {87, 97}}},
    {"chn5icon", {"CHN5BOD", {16, 94}}, {"CHN5ARM1", {91, 96}}, {"CHN5ARM2", {87, 97}}},
    {"pl1icon", {"PL1BOD", {13, 102}}, {"PL1ARM1", {99, 103}}, {"PL1ARM2", {32, 103}}},
    {"pl2icon", {"PL2BOD", {13, 101}}, {"PL2ARM1", {102, 103}}, {"PL2ARM2", {32, 104}}},
    {"pl3icon", {"PL3BOD", {14, 97}}, {"PL3ARM1", {101, 104}}, {"PL3ARM2", {32, 103}}},
}};

inline const PaperdollArmorArt *paperdollArmorArt(std::string_view itemPicture) {
    for (const PaperdollArmorArt &art : kPaperdollArmorArt)
        if (ascii::noCaseEquals(art.itemPicture, itemPicture))
            return &art;
    return nullptr;
}

}  // namespace mm6
