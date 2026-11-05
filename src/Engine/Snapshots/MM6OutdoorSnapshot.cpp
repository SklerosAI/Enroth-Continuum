#include "MM6OutdoorSnapshot.h"

#include <array>
#include <cstdint>
#include <vector>

#include "CompositeSnapshots.h"

#include "Library/Binary/BinarySerialization.h"

#include "Utility/Exception.h"
#include "Utility/Streams/MemoryInputStream.h"

#pragma pack(push, 1)
struct MM6OutdoorDecoration {
    uint16_t decorationId;
    uint16_t flags;
    Vec3i position;
    int32_t yaw;
    int16_t eventVariable;
    int16_t eventId;
    int16_t triggerRadius;
    int16_t direction;
};
static_assert(sizeof(MM6OutdoorDecoration) == 28);
MM_DECLARE_MEMCOPY_SERIALIZABLE(MM6OutdoorDecoration)

struct MM6OutdoorBspNode {
    int32_t front;
    int32_t back;
    int16_t faceOffset;
    int16_t faceCount;
};
static_assert(sizeof(MM6OutdoorBspNode) == 12);
MM_DECLARE_MEMCOPY_SERIALIZABLE(MM6OutdoorBspNode)
#pragma pack(pop)

void deserializeMM6Outdoor(InputStream &src, OutdoorLocation_MM7 *dst) {
    deserialize(src, &dst->name);
    deserialize(src, &dst->fileName);
    deserialize(src, &dst->description);
    deserialize(src, &dst->skyTexture);
    deserialize(src, &dst->groundTilesetUnused);
    deserialize(src, &dst->tileTypes);
    deserialize(src, &dst->heightMap);
    deserialize(src, &dst->tileMap);
    deserialize(src, &dst->attributeMap);

    deserialize(src, &dst->models);
    if (dst->models.size() > 2000)
        throw Exception("MM6 outdoor map has {} models", dst->models.size());

    dst->modelExtras.clear();
    dst->modelExtras.reserve(dst->models.size());
    for (const BSPModelData_MM7 &model : dst->models) {
        if (model.numVertices > 100000 || model.numFaces > 100000 || model.numNodes > 100000)
            throw Exception("MM6 outdoor model has unreasonable dimensions");

        BSPModelExtras_MM7 &extra = dst->modelExtras.emplace_back();
        deserialize(src, &extra.vertices, tags::presized(model.numVertices));
        deserialize(src, &extra.faces, tags::presized(model.numFaces));
        deserialize(src, &extra.facesOrdering, tags::presized(model.numFaces));
        deserialize(src, &extra.faceTextures, tags::presized(model.numFaces));

        // MM6 stores two 12-byte BSP node lists. The outdoor renderer works from
        // model faces directly; these legacy ordering lists are not needed there.
        src.skipOrFail(model.numNodes * 2 * sizeof(MM6OutdoorBspNode));
    }

    std::vector<MM6OutdoorDecoration> mm6Decorations;
    deserialize(src, &mm6Decorations);
    if (mm6Decorations.size() > 100000)
        throw Exception("MM6 outdoor map has {} decorations", mm6Decorations.size());
    dst->decorations.clear();
    dst->decorations.reserve(mm6Decorations.size());
    for (const MM6OutdoorDecoration &source : mm6Decorations) {
        LevelDecoration_MM7 &target = dst->decorations.emplace_back();
        target.uDecorationDescID = source.decorationId;
        target.uFlags = source.flags;
        target.vPosition = source.position;
        target._yawAngle = source.yaw;
        target.uEventID = source.eventId;
        target.uTriggerRange = source.triggerRadius;
        target.eventVarId = source.eventVariable;
        target.field_1A = source.direction;
    }

    deserialize(src, &dst->decorationNames, tags::presized(dst->decorations.size()));
    deserialize(src, &dst->decorationPidList);
    deserialize(src, &dst->decorationMap);

    std::vector<SpawnPoint_MM6> mm6Spawns;
    deserialize(src, &mm6Spawns);
    if (mm6Spawns.size() > 100000)
        throw Exception("MM6 outdoor map has {} spawn points", mm6Spawns.size());
    dst->spawnPoints.clear();
    dst->spawnPoints.reserve(mm6Spawns.size());
    for (const SpawnPoint_MM6 &source : mm6Spawns) {
        SpawnPoint_MM7 &target = dst->spawnPoints.emplace_back();
        target.position = source.position;
        target.radius = source.radius;
        target.type = source.type;
        target.treasureLevelOrMonsterIndex = source.treasureLevelOrMonsterIndex;
        target.attributes = source.attributes;
    }
}

void deserializeMM6Outdoor(const Blob &src, OutdoorLocation_MM7 *dst) {
    MemoryInputStream stream(src.data(), src.size(), "MM6 outdoor map");
    deserializeMM6Outdoor(stream, dst);
    if (stream.position() != stream.size())
        throw Exception("MM6 outdoor map has {} trailing bytes", stream.size() - stream.position());
}
