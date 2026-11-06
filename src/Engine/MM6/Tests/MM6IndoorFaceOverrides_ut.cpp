#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6IndoorFaceOverrides.h"

namespace {

std::vector<mm6::IndoorFaceState> baseFaces() {
    return {{1, "stone"}, {2, "door"}, {3, "wall"}};
}

mm6::IndoorFaceMapOverrides changedMap(std::string_view name) {
    std::vector<mm6::IndoorFaceState> current = baseFaces();
    current[0].attributes = 9;
    current[1].attributes = 42;
    current[1].textureName = "doornew";
    return *mm6::captureIndoorFaceOverrides(name, baseFaces(), current, GameTitle::MM6);
}

mm6::IndoorBaseFaceReader matchingBases() {
    return [](std::string_view) { return baseFaces(); };
}

} // namespace

UNIT_TEST(MM6IndoorFaceOverrides, DetachedCaptureStageAndPostReloadPrecedence) {
    const std::vector<mm6::IndoorFaceState> base = baseFaces();
    EXPECT_FALSE(mm6::captureIndoorFaceOverrides("a.blv", base, base, GameTitle::MM6));
    mm6::IndoorFaceOverrides source{{changedMap("a.blv")}};
    ASSERT_EQ(source.maps[0].faces.size(), 2);
    EXPECT_EQ(source.maps[0].faces[0].faceIndex, 0);
    EXPECT_TRUE(source.maps[0].faces[0].attributes);
    EXPECT_FALSE(source.maps[0].faces[0].textureName);

    mm6::SaveSection section = mm6::encodeIndoorFaceOverrideSection(source);
    EXPECT_FALSE(section.required);
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer({{section}});
    mm6::IndoorFaceOverrides staged;
    mm6::loadIndoorFaceOverridesBeforeMutation(bytes, matchingBases(), &staged, GameTitle::MM6);
    EXPECT_EQ(staged, source);

    // Script reload has already changed both fields. IFOV wins only where the
    // saved face had a difference from the base; other script changes survive.
    std::vector<mm6::IndoorFaceState> afterScript = base;
    afterScript[0].textureName = "script";
    afterScript[1] = {77, "script"};
    afterScript[2] = {88, "wall"};
    mm6::applyIndoorFaceOverridesAfterMapReload(staged, "a.blv", &afterScript, GameTitle::MM6);
    EXPECT_EQ(afterScript[0], (mm6::IndoorFaceState{9, "script"}));
    EXPECT_EQ(afterScript[1], (mm6::IndoorFaceState{42, "doornew"}));
    EXPECT_EQ(afterScript[2], (mm6::IndoorFaceState{88, "wall"}));

    std::vector<mm6::IndoorFaceState> other = base;
    mm6::applyIndoorFaceOverridesAfterMapReload(staged, "c.blv", &other, GameTitle::MM6);
    EXPECT_EQ(other, base);
    other.pop_back();
    const auto shortTarget = other;
    EXPECT_THROW(mm6::applyIndoorFaceOverridesAfterMapReload(staged, "a.blv", &other,
                                                             GameTitle::MM6), std::exception);
    EXPECT_EQ(other, shortTarget);
    EXPECT_THROW(mm6::applyIndoorFaceOverridesAfterMapReload(staged, "a.blv", &other,
                                                             GameTitle::MM7), std::exception);
    EXPECT_EQ(other, shortTarget);
}

UNIT_TEST(MM6IndoorFaceOverrides, RejectsMalformedAndDuplicateEntries) {
    mm6::IndoorFaceOverrides source{{changedMap("a.blv")}};
    mm6::SaveSection section = mm6::encodeIndoorFaceOverrideSection(source);
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section, section}}), std::exception);
    section.payload[0] = 2;
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);
    section.payload[0] = 1;
    section.payload.pop_back();
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);
    section = mm6::encodeIndoorFaceOverrideSection(source);
    section.payload.push_back(0);
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);
    section = mm6::encodeIndoorFaceOverrideSection(source);
    const std::size_t firstFace = 2 + 1 + source.maps[0].mapName.size() + 4 + 8 + 4;
    section.payload[firstFace + 4] = 0x80;
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);
    section = mm6::encodeIndoorFaceOverrideSection(source);
    const std::size_t secondFace = firstFace + 4 + 1 + 4;
    for (std::size_t i = 0; i < 4; ++i)
        section.payload[secondFace + i] = 0; // Duplicate index in untrusted payload.
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);
    section = mm6::encodeIndoorFaceOverrideSection(source);
    section.required = true;
    EXPECT_THROW(mm6::decodeIndoorFaceOverrideSection({{section}}), std::exception);

    source.maps[0].faces.push_back(source.maps[0].faces.back());
    EXPECT_THROW(mm6::encodeIndoorFaceOverrideSection(source), std::exception);
    source.maps[0].faces.back().faceIndex = 3;
    EXPECT_THROW(mm6::encodeIndoorFaceOverrideSection(source), std::exception);
    source.maps[0].faces.pop_back();
    source.maps.push_back(source.maps.front());
    EXPECT_THROW(mm6::encodeIndoorFaceOverrideSection(source), std::exception);
    source.maps.back().mapName = "../escape.blv";
    EXPECT_THROW(mm6::encodeIndoorFaceOverrideSection(source), std::exception);
    source.maps.pop_back();
    source.maps[0].faces[0].textureName = "bad/name";
    EXPECT_THROW(mm6::encodeIndoorFaceOverrideSection(source), std::exception);
    auto specialBase = baseFaces();
    specialBase[0].textureName = "$wall";
    auto specialCurrent = specialBase;
    specialCurrent[0].attributes = 17;
    EXPECT_TRUE(mm6::captureIndoorFaceOverrides("a.blv", specialBase, specialCurrent,
                                                GameTitle::MM6));
    EXPECT_THROW(mm6::captureIndoorFaceOverrides("a.blv", baseFaces(), baseFaces(),
                                                GameTitle::MM7), std::exception);
}

UNIT_TEST(MM6IndoorFaceOverrides, RejectsLateBaseFailureBeforeDestinationMutation) {
    mm6::IndoorFaceOverrides source{{changedMap("a.blv"), changedMap("b.blv")}};
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(
        {{mm6::encodeIndoorFaceOverrideSection(source)}});
    mm6::IndoorFaceOverrides destination{{changedMap("safe.blv")}};
    const mm6::IndoorFaceOverrides original = destination;
    int reads = 0;
    const auto lateMismatch = [&reads](std::string_view) {
        std::vector<mm6::IndoorFaceState> base = baseFaces();
        if (++reads == 2)
            base[1].textureName = "different";
        return base;
    };
    EXPECT_THROW(mm6::loadIndoorFaceOverridesBeforeMutation(bytes, lateMismatch,
                                                             &destination, GameTitle::MM6), std::exception);
    EXPECT_EQ(reads, 2);
    EXPECT_EQ(destination, original);
    bytes.back() ^= 1; // Container checksum fails before even the first map read.
    reads = 0;
    EXPECT_THROW(mm6::loadIndoorFaceOverridesBeforeMutation(bytes, lateMismatch,
                                                             &destination, GameTitle::MM6), std::exception);
    EXPECT_EQ(reads, 0);
    EXPECT_EQ(destination, original);
}
