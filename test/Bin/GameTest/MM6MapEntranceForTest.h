#pragma once

#include <cctype>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Evt/EvtInstruction.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Tables/MapTable.h"

#include "Utility/Math/TrigLut.h"
#include "Utility/Memory/Blob.h"

struct MM6ShippedEntrance {
    PartyPlacement placement;
    std::string source;
};

inline MapId mm6MapByNameOrId(std::string_view value) {
    // A map ID or a map file name, MAP_INVALID for anything else.
    int id = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), id);
    if (error == std::errc() && end == value.data() + value.size())
        return id >= std::to_underlying(MAP_FIRST) && id <= std::to_underlying(MAP_LAST) ? static_cast<MapId>(id) : MAP_INVALID;
    return pMapTable->GetMapInfo(value);
}

inline std::string mm6LowercaseForTest(std::string_view value) {
    std::string result(value);
    for (char &c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

inline std::optional<MM6ShippedEntrance> findShippedMM6Entrance(std::string_view fileName) {
    // The first MoveToMap with coordinates that names this map, from any map script.
    const std::string target = mm6LowercaseForTest(fileName);
    for (MapId map : pMapTable->pInfos.indices()) {
        const std::string &source = pMapTable->pInfos[map].fileName;
        if (source.size() < 4)
            continue;
        const std::string evtName = source.substr(0, source.size() - 4) + ".evt";
        const Blob data = Blob::copy(engine->resources()->eventsData(evtName));
        if (data.size() == 0)
            continue;
        const EvtProgram program = EvtProgram::load(data, GameTitle::MM6);
        for (int eventId = 0; eventId <= 0xFFFF; ++eventId) {
            if (!program.hasEvent(eventId))
                continue;
            for (const EvtInstruction &ir : program.function(eventId)) {
                const auto &move = ir.data.move_map_descr;
                if (ir.opcode != EVENT_MoveToMap || mm6LowercaseForTest(ir.str) != target || !(move.x || move.y || move.z))
                    continue;
                const int yaw = move.yaw != -1 ? (move.yaw & TrigTableLookup::uDoublePiMask) : -1;
                return MM6ShippedEntrance{PartyPlacement(Vec3f(move.x, move.y, move.z), yaw, move.pitch, move.zspeed),
                                          evtName + " event " + std::to_string(eventId)};
            }
        }
    }
    return std::nullopt;
}
