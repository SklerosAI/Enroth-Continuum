#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6BeaconSaveFields.h"
#include "Engine/MM6/MM6BeaconSaveRuntime.h"
#include "Engine/Objects/Character.h"
#include "Engine/Tables/MapTable.h"

namespace {

mm6::BeaconSaveFields example() {
    mm6::BeaconSaveFields fields;
    fields.thumbnailRgba = mm6::copyRgbaPixels(
        RgbaImage::solid(Color(8, 16, 24, 255), mm6::SAVE_THUMBNAIL_WIDTH, mm6::SAVE_THUMBNAIL_HEIGHT),
        mm6::SAVE_THUMBNAIL_WIDTH, mm6::SAVE_THUMBNAIL_HEIGHT);
    mm6::BeaconSaveRecord beacon;
    beacon.expiresAtTicks = 987654;
    beacon.mapFileName = "o0.odm";
    beacon.position = {12.5f, -27.0f, 340.0f};
    beacon.yaw = -512;
    beacon.pitch = 91;
    beacon.unknown = 0xa5a5;
    beacon.rgba = mm6::copyRgbaPixels(
        RgbaImage::solid(Color(10, 20, 30, 40), mm6::BEACON_IMAGE_WIDTH, mm6::BEACON_IMAGE_HEIGHT),
        mm6::BEACON_IMAGE_WIDTH, mm6::BEACON_IMAGE_HEIGHT);
    fields.beacons[2][4] = std::move(beacon);
    return fields;
}

std::vector<std::uint8_t> saveBytes(const mm6::BeaconSaveFields &fields) {
    return mm6::encodeSaveContainer({{mm6::encodeBeaconSection(fields)}});
}

std::vector<std::string> expectedNames() {
    std::vector<std::string> names;
    names.push_back("o0.ddm");
    for (int i = 0; i < 66; ++i)
        names.push_back("d" + std::to_string(i) + ".dlv");
    return names;
}

} // namespace

UNIT_TEST(MM6BeaconSaveFields, RoundTripExactImagesAndResolveMapByFilename) {
    const mm6::BeaconSaveFields fields = example();
    const std::vector<std::uint8_t> bytes = saveBytes(fields);
    EXPECT_EQ(mm6::decodeBeaconSection(mm6::decodeSaveContainer(bytes)), fields);

    mm6::BeaconSaveFields destination;
    mm6::loadBeaconFieldsBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, fields);

    auto table = std::make_unique<MapTable>();
    table->pInfos[static_cast<MapId>(1)].fileName = "o0.odm";
    const std::vector<std::string> names = expectedNames();
    mm6::StagedBeaconVisuals staged = mm6::stageBeaconVisuals(fields, *table, names, GameTitle::MM6);
    EXPECT_EQ(staged.thumbnailRgba, fields.thumbnailRgba);
    EXPECT_EQ(staged.thumbnailRgba[0], 8);
    EXPECT_EQ(staged.thumbnailRgba[1], 16);
    EXPECT_EQ(staged.thumbnailRgba[2], 24);
    EXPECT_EQ(staged.thumbnailRgba[3], 255);
    ASSERT_TRUE(staged.beacons[2][4].has_value());
    EXPECT_EQ(staged.beacons[2][4]->mapId, static_cast<MapId>(1));
    EXPECT_EQ(staged.beacons[2][4]->rgba, fields.beacons[2][4]->rgba);
    EXPECT_EQ(staged.beacons[2][4]->rgba[0], 10);
    EXPECT_EQ(staged.beacons[2][4]->rgba[1], 20);
    EXPECT_EQ(staged.beacons[2][4]->rgba[2], 30);
    EXPECT_EQ(staged.beacons[2][4]->rgba[3], 40);
    EXPECT_EQ(staged.beacons[2][4]->unknown, 0xa5a5);
    EXPECT_FALSE(staged.beacons[0][0].has_value());
}

UNIT_TEST(MM6BeaconSaveFields, CapturesCurrentMapAsVerifiedFilenameWithoutGraphicsPublication) {
    auto table = std::make_unique<MapTable>();
    table->pInfos[static_cast<MapId>(1)].fileName = "o0.odm";
    const std::vector<std::string> names = expectedNames();
    LloydBeacon beacon;
    beacon.uBeaconTime = Time::fromTicks(987654);
    beacon.mapId = static_cast<MapId>(1);
    beacon._partyPos = Vec3f(12.5f, -27.0f, 340.0f);
    beacon._partyViewYaw = -512;
    beacon._partyViewPitch = 91;
    beacon.unknown = 0xa5a5;
    RgbaImage image = RgbaImage::solid(Color(10, 20, 30, 40), 92, 68);
    EXPECT_EQ(mm6::captureBeaconRecord(beacon, image, *table, names, GameTitle::MM6),
              *example().beacons[2][4]);
    EXPECT_THROW((void) mm6::captureBeaconRecord(beacon, image, *table, names, GameTitle::MM7),
                 std::exception);
    EXPECT_THROW((void) mm6::captureBeaconRecord(
        beacon, RgbaImage::solid(Color(), 1, 1), *table, names, GameTitle::MM6), std::exception);
    table->pInfos[static_cast<MapId>(1)].fileName.clear();
    EXPECT_THROW((void) mm6::captureBeaconRecord(beacon, image, *table, names, GameTitle::MM6),
                 std::exception);
}

UNIT_TEST(MM6BeaconSaveFields, RejectsMalformedImageAndMapBeforeMutation) {
    const mm6::BeaconSaveFields before = example();
    mm6::BeaconSaveFields destination = before;

    mm6::BeaconSaveFields invalid = before;
    invalid.thumbnailRgba.pop_back();
    EXPECT_THROW((void) mm6::encodeBeaconSection(invalid), std::exception);
    invalid = before;
    invalid.beacons[2][4]->rgba.pop_back();
    EXPECT_THROW((void) mm6::encodeBeaconSection(invalid), std::exception);
    invalid = before;
    invalid.beacons[2][4]->position[1] = std::numeric_limits<float>::infinity();
    EXPECT_THROW((void) mm6::encodeBeaconSection(invalid), std::exception);

    mm6::SaveContainer container{{mm6::encodeBeaconSection(before)}};
    container.sections[0].payload[3] = 2; // Invalid thumbnail flag with valid CRC.
    EXPECT_THROW(mm6::loadBeaconFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    container = {{mm6::encodeBeaconSection(before)}};
    const std::string mapName = "o0.odm";
    const auto nameAt = std::search(container.sections[0].payload.begin(),
                                    container.sections[0].payload.end(),
                                    mapName.begin(), mapName.end());
    ASSERT_NE(nameAt, container.sections[0].payload.end());
    *nameAt = '/'; // A path component is invalid even with valid container checksums.
    EXPECT_THROW(mm6::loadBeaconFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);
    container = {{mm6::encodeBeaconSection(before)}};
    container.sections[0].payload.back() = 0; // Valid pixel change is allowed.
    EXPECT_NO_THROW(mm6::loadBeaconFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination));
    destination = before;
    container.sections[0].payload.pop_back();
    EXPECT_THROW(mm6::loadBeaconFieldsBeforeMutation(mm6::encodeSaveContainer(container), &destination),
                 std::exception);
    EXPECT_EQ(destination, before);

    std::vector<std::uint8_t> wrongTitle = saveBytes(before);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::loadBeaconFieldsBeforeMutation(wrongTitle, &destination), std::exception);
    EXPECT_EQ(destination, before);
}

UNIT_TEST(MM6BeaconSaveFields, SupportsEmptyNewGameAndAllTwentySlots) {
    mm6::BeaconSaveFields fields;
    EXPECT_EQ(mm6::decodeBeaconSection(mm6::decodeSaveContainer(saveBytes(fields))), fields);

    const mm6::BeaconSaveRecord record = *example().beacons[2][4];
    fields.thumbnailRgba = example().thumbnailRgba;
    for (auto &character : fields.beacons)
        for (auto &slot : character)
            slot = record;
    const mm6::SaveSection section = mm6::encodeBeaconSection(fields);
    EXPECT_LE(section.payload.size(), 600000);
    EXPECT_EQ(mm6::decodeBeaconSection(mm6::decodeSaveContainer(saveBytes(fields))), fields);
}

UNIT_TEST(MM6BeaconSaveFields, RejectsForeignAmbiguousOrAbsentMapResolution) {
    const mm6::BeaconSaveFields fields = example();
    auto table = std::make_unique<MapTable>();
    table->pInfos[static_cast<MapId>(1)].fileName = "o0.odm";
    std::vector<std::string> names = expectedNames();
    EXPECT_THROW((void) mm6::stageBeaconVisuals(fields, *table, names, GameTitle::MM7), std::exception);
    names[0] = "absent.ddm";
    EXPECT_THROW((void) mm6::stageBeaconVisuals(fields, *table, names, GameTitle::MM6), std::exception);
    names[0] = "o0.ddm";
    table->pInfos[static_cast<MapId>(2)].fileName = "o0.odm";
    EXPECT_THROW((void) mm6::stageBeaconVisuals(fields, *table, names, GameTitle::MM6), std::exception);
    table->pInfos[static_cast<MapId>(2)].fileName.clear();
    table->pInfos[static_cast<MapId>(1)].fileName.clear();
    EXPECT_THROW((void) mm6::stageBeaconVisuals(fields, *table, names, GameTitle::MM6), std::exception);
}
