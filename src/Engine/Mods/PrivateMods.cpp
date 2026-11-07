#include "Engine/Mods/PrivateMods.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/ImageLoader.h"
#include "Library/Image/Png.h"
#include "Library/Logger/Logger.h"
#include "Utility/Memory/Blob.h"
#include "Utility/System/Fs.h"

namespace mm7::mods {
namespace {

constexpr std::array<std::string_view, static_cast<size_t>(PortraitMood::Count)> moodNames = {
    "normal", "talk", "smile", "sad", "hurt", "pain", "afraid", "cast", "sleep", "unconscious",
    "weak", "cursed", "drunk", "insane", "sick", "paralyzed", "stone", "zombie", "dead", "eradicated"
};

class PortraitLoader final : public ImageLoader {
 public:
    PortraitLoader(std::string name, RgbaImage image)
        : ImageLoader(nullptr, name), image(std::move(image)), imageSize(this->image.size()) {}

    RgbaImage loadRgba() override { return std::move(image); }
    Sizei loadSize() override { return imageSize; }

 private:
    RgbaImage image;
    Sizei imageSize;
};

std::unique_ptr<GraphicsImage> readPortrait(PathView path, int slot, PortraitMood mood) {
    if (!fs::exists(path))
        return nullptr;
    try {
        Blob data = Blob::fromFile(path);
        if (data.size() > 4 * 1024 * 1024)
            throw std::runtime_error("portrait exceeds size limit");
        Sizei size = png::decodeSize(data);
        if (size.w < 32 || size.h < 32 || size.w > 512 || size.h > 512)
            throw std::runtime_error("portrait exceeds dimension limit");
        RgbaImage image = png::decode(data);
        std::string name = "mm7-mod-portrait-" + std::to_string(slot) + "-" + std::string(portraitMoodName(mood));
        return GraphicsImage::Create(std::make_unique<PortraitLoader>(std::move(name), std::move(image)));
    } catch (const std::exception &) {
        // File paths and decoder errors may reveal private local data.
        MM_WARNING("Ignoring invalid MM7 portrait for slot {} mood {}.", slot, portraitMoodName(mood));
        return nullptr;
    }
}

}  // namespace

PrivatePortraits::~PrivatePortraits() = default;

std::string_view portraitMoodName(PortraitMood mood) {
    size_t index = static_cast<size_t>(mood);
    return index < moodNames.size() ? moodNames[index] : "normal";
}

PortraitMood portraitMood(PortraitId portrait, Condition condition) {
    // A zombie uses PORTRAIT_NORMAL in the original frame table, so its condition takes priority.
    if (condition == CONDITION_ERADICATED || portrait == PORTRAIT_ERADICATED)
        return PortraitMood::Eradicated;
    if (condition == CONDITION_DEAD || portrait == PORTRAIT_DEAD)
        return PortraitMood::Dead;
    if (condition == CONDITION_ZOMBIE)
        return PortraitMood::Zombie;

    switch (portrait) {
    case PORTRAIT_MOUTH_OPEN_WIDE:
    case PORTRAIT_MOUTH_OPEN_A:
    case PORTRAIT_MOUTH_OPEN_O:
        return PortraitMood::Talk;
    case PORTRAIT_SMILE:
    case PORTRAIT_WIDE_SMILE:
        return PortraitMood::Smile;
    case PORTRAIT_SAD:
        return PortraitMood::Sad;
    case PORTRAIT_AVOID_DAMAGE:
    case PORTRAIT_DMGRECVD_MINOR:
        return PortraitMood::Hurt;
    case PORTRAIT_DMGRECVD_MODERATE:
    case PORTRAIT_DMGRECVD_MAJOR:
        return PortraitMood::Pain;
    case PORTRAIT_FEAR:
    case PORTRAIT_SCARED:
        return PortraitMood::Afraid;
    case PORTRAIT_CAST_SPELL:
        return PortraitMood::Cast;
    case PORTRAIT_SLEEP:
        return PortraitMood::Sleep;
    case PORTRAIT_UNCONSCIOUS:
        return PortraitMood::Unconscious;
    case PORTRAIT_WEAK:
        return PortraitMood::Weak;
    case PORTRAIT_CURSED:
        return PortraitMood::Cursed;
    case PORTRAIT_DRUNK:
        return PortraitMood::Drunk;
    case PORTRAIT_INSANE:
        return PortraitMood::Insane;
    case PORTRAIT_POISONED:
    case PORTRAIT_DISEASED:
        return PortraitMood::Sick;
    case PORTRAIT_PARALYZED:
        return PortraitMood::Paralyzed;
    case PORTRAIT_PETRIFIED:
        return PortraitMood::Stone;
    default:
        return PortraitMood::Normal;
    }
}

void PrivatePortraits::load(PathView directory) {
    Path root = directory / "portraits";
    for (int slot = 1; slot <= 4; ++slot) {
        auto &images = portraits[slot - 1];
        Path slotRoot = root / std::to_string(slot);
        images[static_cast<size_t>(PortraitMood::Normal)] =
            readPortrait(slotRoot / "normal.png", slot, PortraitMood::Normal);
        if (!images[static_cast<size_t>(PortraitMood::Normal)])
            images[static_cast<size_t>(PortraitMood::Normal)] =
                readPortrait(directory / ("portrait-" + std::to_string(slot) + ".png"), slot, PortraitMood::Normal);
        if (!images[static_cast<size_t>(PortraitMood::Normal)])
            continue;
        MM_INFO("Loaded private MM7 portrait override for party slot {}.", slot);
        for (size_t mood = 1; mood < moodNames.size(); ++mood) {
            images[mood] = readPortrait(slotRoot / (std::string(moodNames[mood]) + ".png"),
                                        slot, static_cast<PortraitMood>(mood));
        }
    }
}

GraphicsImage *PrivatePortraits::get(int partySlot, PortraitId portrait, Condition condition) const {
    if (partySlot < 0 || partySlot >= portraits.size())
        return nullptr;
    const auto &images = portraits[partySlot];
    PortraitMood mood = portraitMood(portrait, condition);
    GraphicsImage *specific = images[static_cast<size_t>(mood)].get();
    if (specific)
        return specific;
    if (mood == PortraitMood::Dead || mood == PortraitMood::Eradicated)
        return nullptr;  // Keep the original death art when no custom death image exists.
    if (mood == PortraitMood::Pain && images[static_cast<size_t>(PortraitMood::Hurt)])
        return images[static_cast<size_t>(PortraitMood::Hurt)].get();
    if (mood == PortraitMood::Unconscious && images[static_cast<size_t>(PortraitMood::Sleep)])
        return images[static_cast<size_t>(PortraitMood::Sleep)].get();
    return images[static_cast<size_t>(PortraitMood::Normal)].get();
}

void PrivatePortraits::releaseRenderIds() {
    for (const auto &slot : portraits)
        for (const auto &image : slot)
            if (image)
                image->releaseRenderId();
}

}  // namespace mm7::mods
