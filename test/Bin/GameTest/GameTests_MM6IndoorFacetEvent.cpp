#include <cstdint>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"

#include "Utility/Memory/Blob.h"

namespace {

void appendU32(std::string *bytes, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes->push_back(static_cast<char>(value >> (8 * i)));
}

EvtInstruction instruction(EvtOpcode opcode, std::string payload) {
    std::string bytes;
    bytes.push_back(static_cast<char>(4 + payload.size()));
    bytes.push_back(1); // Event ID.
    bytes.push_back(0);
    bytes.push_back(0); // Step.
    bytes.push_back(static_cast<char>(opcode));
    bytes += payload;
    return EvtProgram::load(Blob::fromString(bytes), GameTitle::MM6).instruction(1, 0);
}

void appendRecord(std::string *bytes, int step, EvtOpcode opcode, std::string payload = {}) {
    bytes->push_back(static_cast<char>(4 + payload.size()));
    bytes->push_back(1);
    bytes->push_back(0);
    bytes->push_back(static_cast<char>(step));
    bytes->push_back(static_cast<char>(opcode));
    *bytes += payload;
}

std::string faceBitPayload(bool set) {
    std::string payload;
    appendU32(&payload, 1);
    appendU32(&payload, static_cast<uint32_t>(FACE_IsInvisible));
    payload.push_back(set ? 1 : 0);
    return payload;
}

std::string faceTexturePayload() {
    std::string payload;
    appendU32(&payload, 1);
    payload += std::string("pending\0", 8);
    return payload;
}

MM6IndoorSnapshot syntheticBase() {
    MM6IndoorSnapshot base;
    base.sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
    base.sections.push_back({MM6IndoorSectionKind::Vertices, Blob(), 0});
    base.sections.push_back({MM6IndoorSectionKind::Faces, Blob(), 3});
    base.sections.push_back({MM6IndoorSectionKind::Rooms, Blob(), 0});
    return base;
}

std::vector<uint8_t> syntheticDelta() {
    return std::vector<uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0);
}

struct ScopedSyntheticIndoorMap {
    IndoorLocation location;
    IndoorLocation *previousIndoor = pIndoor;
    decltype(uCurrentlyLoadedLevelType) previousLevelType = uCurrentlyLoadedLevelType;
    EvtProgram previousEvents = std::move(engine->_localEventMap);
    std::deque<Actor> previousActors;
    std::vector<SpriteObject> previousObjects;
    std::vector<Chest> previousChests;

    ScopedSyntheticIndoorMap() {
        previousActors.swap(pActors);
        previousObjects.swap(pSpriteObjects);
        previousChests.swap(vChests);
        pIndoor = &location;
        uCurrentlyLoadedLevelType = LEVEL_INDOOR;
        location.faces.resize(3);
        location.doors.resize(200);
        location.mm6DeltaBytes = syntheticDelta();
        location._visible_outlines.fill(0);
        location.dlv.respawnCount = 0;
        location.dlv.lastRespawnDay = 0;
    }

    ~ScopedSyntheticIndoorMap() {
        engine->_localEventMap = std::move(previousEvents);
        pIndoor = previousIndoor;
        uCurrentlyLoadedLevelType = previousLevelType;
        pActors.swap(previousActors);
        pSpriteObjects.swap(previousObjects);
        vChests.swap(previousChests);
    }
};

struct ScopedIndoorFaces {
    decltype(uCurrentlyLoadedLevelType) previousLevelType = uCurrentlyLoadedLevelType;
    std::vector<BLVFace> previousFaces;

    ScopedIndoorFaces() {
        std::vector<BLVFace> syntheticFaces(3);
        previousFaces = std::move(pIndoor->faces);
        pIndoor->faces = std::move(syntheticFaces);
        uCurrentlyLoadedLevelType = LEVEL_INDOOR;
        for (BLVFace &face : pIndoor->faces) {
            face.attributes = static_cast<FaceAttribute>(0);
            face.cogNumber = 77; // A group lookup would modify all three faces.
            face.texture = nullptr;
        }
    }

    ~ScopedIndoorFaces() {
        pIndoor->faces = std::move(previousFaces);
        uCurrentlyLoadedLevelType = previousLevelType;
    }
};

} // namespace

GAME_TEST(MM6IndoorFacetEvent, UsesDirectFaceIndexAndBounds) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(pIndoor, nullptr);
    ScopedIndoorFaces restore;
    EvtInterpreter interpreter;

    std::string bitPayload;
    appendU32(&bitPayload, 1);
    appendU32(&bitPayload, static_cast<uint32_t>(FACE_IsInvisible));
    bitPayload.push_back(1);
    EvtInstruction setBit = instruction(EVENT_SetFacesBit, bitPayload);
    EXPECT_EQ(interpreter.executeInstruction(setBit).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pIndoor->faces[0].Invisible());
    EXPECT_TRUE(pIndoor->faces[1].Invisible());
    EXPECT_FALSE(pIndoor->faces[2].Invisible());

    bitPayload.back() = 0;
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, bitPayload)).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pIndoor->faces[1].Invisible());

    std::string texturePayload;
    appendU32(&texturePayload, 1);
    texturePayload += std::string("pending\0", 8);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetTexture, texturePayload)).type, EVT_FLOW_NEXT);
    EXPECT_EQ(pIndoor->faces[0].texture, nullptr);
    EXPECT_NE(pIndoor->faces[1].texture, nullptr);
    EXPECT_EQ(pIndoor->faces[2].texture, nullptr);

    bitPayload.clear();
    appendU32(&bitPayload, 99);
    appendU32(&bitPayload, static_cast<uint32_t>(FACE_IsInvisible));
    bitPayload.push_back(1);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, bitPayload)).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pIndoor->faces[0].Invisible());
    EXPECT_FALSE(pIndoor->faces[1].Invisible());
    EXPECT_FALSE(pIndoor->faces[2].Invisible());

    uCurrentlyLoadedLevelType = LEVEL_OUTDOOR;
    bitPayload.clear();
    appendU32(&bitPayload, 1);
    appendU32(&bitPayload, static_cast<uint32_t>(FACE_IsInvisible));
    bitPayload.push_back(1);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, bitPayload)).type, EVT_FLOW_NEXT);
    EXPECT_FALSE(pIndoor->faces[1].Invisible());
}

GAME_TEST(MM6IndoorFacetEvent, ChangedFacesRequireReloadReplayOrSeparateSaveState) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ScopedSyntheticIndoorMap map;
    EvtInterpreter interpreter;
    const std::vector<uint8_t> originalDelta = map.location.mm6DeltaBytes;

    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, faceBitPayload(true))).type,
              EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, faceBitPayload(false))).type,
              EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetFacesBit, faceBitPayload(true))).type,
              EVT_FLOW_NEXT);
    EXPECT_EQ(interpreter.executeInstruction(instruction(EVENT_SetTexture, faceTexturePayload())).type,
              EVT_FLOW_NEXT);
    ASSERT_TRUE(map.location.faces[1].Invisible());
    ASSERT_NE(map.location.faces[1].texture, nullptr);

    Blob saved = encodeMM6IndoorChangedState(map.location);
    ASSERT_EQ(saved.size(), originalDelta.size());
    EXPECT_EQ(saved.str(), std::string(originalDelta.begin(), originalDelta.end()));
    MM6IndoorRuntimeDelta staged = decodeMM6IndoorRuntimeDelta(saved, syntheticBase());

    // A new BLV reconstruction starts with the base faces. The DLV replay has
    // no face state to apply, including its retained opaque bytes.
    map.location.faces = std::vector<BLVFace>(3);
    reconstructMM6IndoorDelta(staged, &map.location);
    ASSERT_FALSE(map.location.faces[1].Invisible());
    ASSERT_EQ(map.location.faces[1].texture, nullptr);
    engine->_localEventMap.clear();
    onMapLoad();
    EXPECT_FALSE(map.location.faces[1].Invisible());
    EXPECT_EQ(map.location.faces[1].texture, nullptr);

    std::string reloadRecords;
    appendRecord(&reloadRecords, 0, EVENT_OnMapReload, std::string(1, '\0'));
    appendRecord(&reloadRecords, 1, EVENT_SetFacesBit, faceBitPayload(true));
    appendRecord(&reloadRecords, 2, EVENT_SetTexture, faceTexturePayload());
    appendRecord(&reloadRecords, 3, EVENT_Exit, std::string(1, '\0'));
    engine->_localEventMap = EvtProgram::load(Blob::fromString(reloadRecords), GameTitle::MM6);
    onMapLoad();
    EXPECT_TRUE(map.location.faces[1].Invisible());
    EXPECT_NE(map.location.faces[1].texture, nullptr);
}
