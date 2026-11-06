#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"

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
