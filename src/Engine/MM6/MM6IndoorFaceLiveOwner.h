#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/TextureFrameTable.h"
#include "MM6IndoorFaceOverrides.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// Capture canonical face identity from a rebuilt indoor owner. The projected
// source is used only to identify animated faces: GetTexture() returns a
// time-dependent frame, not the stable animation key. If an event changes an
// animated face to another sequence, this adapter refuses until a reverse
// animation-key lookup is available. Call at the actual pre-OnMapReload
// checkpoint and again when capturing the saved runtime state.
inline std::vector<IndoorFaceState> captureLiveIndoorFaceStates(
    const IndoorLocation &live, const IndoorLocation_MM7 &source, GameTitle title) {
    if (title != GameTitle::MM6 || live.faces.empty() ||
        live.faces.size() != source.faces.size() ||
        source.faceTextures.size() != source.faces.size())
        throw std::runtime_error("MM6 indoor face owner lacks a matching BLV source");
    std::vector<IndoorFaceState> result;
    result.reserve(live.faces.size());
    for (std::size_t i = 0; i < live.faces.size(); ++i) {
        const BLVFace &face = live.faces[i];
        std::string texture;
        if (face.IsAnimated()) {
            if (!pTextureFrameTable)
                throw std::runtime_error("MM6 indoor animated face lacks a frame table");
            const auto &raw = source.faceTextures[i];
            const auto end = std::find(raw.begin(), raw.end(), '\0');
            texture = ascii::toLower(std::string_view(raw.data(), end - raw.begin()));
            if (pTextureFrameTable->animationId(texture) != face.animationId)
                throw std::runtime_error("MM6 indoor animated face identity changed without a stable key");
        } else {
            if (!face.texture)
                throw std::runtime_error("MM6 indoor face lacks a stable bitmap identity");
            texture = ascii::toLower(face.texture->name());
        }
        result.push_back({static_cast<std::uint32_t>(face.attributes), std::move(texture)});
    }
    return result;
}

} // namespace mm6
