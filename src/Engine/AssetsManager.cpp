#include "Engine/AssetsManager.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Graphics/ImageLoader.h"
#include "Engine/Graphics/Image.h"

#include "Library/Image/Png.h"

#include "GUI/GUIFont.h"

#include "Library/Logger/Logger.h"

#include "Utility/String/Ascii.h"
#include "Utility/Memory/Blob.h"
#include "Utility/System/Fs.h"

AssetsManager *assets = new AssetsManager();

static void ReloadFonts() {
    if (assets->pFontBookOnlyShadow)
        assets->pFontBookOnlyShadow->CreateFontTex();
    if (assets->pFontBookLloyds)
        assets->pFontBookLloyds->CreateFontTex();
    if (assets->pFontArrus)
        assets->pFontArrus->CreateFontTex();
    if (assets->pFontLucida)
        assets->pFontLucida->CreateFontTex();
    if (assets->pFontBookTitle)
        assets->pFontBookTitle->CreateFontTex();
    if (assets->pFontBookCalendar)
        assets->pFontBookCalendar->CreateFontTex();
    if (assets->pFontCreate)
        assets->pFontCreate->CreateFontTex();
    if (assets->pFontCChar)
        assets->pFontCChar->CreateFontTex();
    if (assets->pFontComic)
        assets->pFontComic->CreateFontTex();
    if (assets->pFontSmallnum)
        assets->pFontSmallnum->CreateFontTex();
}

AssetsManager::AssetsManager() = default;
AssetsManager::~AssetsManager() = default;

void AssetsManager::releaseAllTextures() {
    MM_TRACE("Render - Releasing Textures.");
    // clears any textures from gpu
    for (auto *cache : {&images, &bitmaps, &sprites})
        for (const auto &[name, image] : *cache)
            image->releaseRenderId();
    for (const auto &portrait : customPortraits)
        if (portrait)
            portrait->releaseRenderId();

    ReloadFonts();
}

void AssetsManager::releaseImage(GraphicsImage *image) {
    for (auto *cache : {&images, &bitmaps, &sprites}) {
        auto pos = cache->find(image->name());
        if (pos != cache->end() && pos->second.get() == image) {
            cache->erase(pos);
            return;
        }
    }

    assert(false && "Image is not in the cache");
}

GraphicsImage *AssetsManager::getIcon(std::string_view name) {
    std::string filename = ascii::toLower(name);

    auto i = images.find(filename);
    if (i == images.end())
        i = images.emplace(filename, GraphicsImage::Create(std::make_unique<Icon_LOD_Loader>(engine->resources(), filename))).first;

    return i->second.get();
}

GraphicsImage *AssetsManager::getCustomPortrait(int partySlot) {
    if (partySlot < 0 || partySlot >= 4)
        return nullptr;

    if (!customPortraitsLoaded) {
        customPortraitsLoaded = true;
        const char *directory = std::getenv("ENROTH_MM7_MOD_DIR");
        if (directory && *directory) {
            for (int i = 0; i < 4; ++i) {
                Path path(std::string(directory) + "/portrait-" + std::to_string(i + 1) + ".png");
                if (!fs::exists(path))
                    continue;
                try {
                    Blob data = Blob::fromFile(path);
                    if (data.size() > 4 * 1024 * 1024)
                        throw std::runtime_error("portrait exceeds 4 MB");
                    RgbaImage portrait = png::decode(data);
                    if (portrait.width() < 32 || portrait.height() < 32 || portrait.width() > 512 || portrait.height() > 512)
                        throw std::runtime_error("portrait dimensions must be 32-512 pixels");
                    customPortraits[i] = GraphicsImage::Create(std::move(portrait));
                    MM_INFO("Loaded private portrait override for party slot {}.", i + 1);
                } catch (const std::exception &error) {
                    MM_WARNING("Ignoring portrait override for party slot {}: {}", i + 1, error.what());
                }
            }
        }
    }
    return customPortraits[partySlot].get();
}

GraphicsImage *AssetsManager::getImage_Buff(std::string_view name) {
    std::string filename = ascii::toLower(name);

    auto i = images.find(filename);
    if (i == images.end())
        i = images.emplace(filename, GraphicsImage::Create(std::make_unique<Buff_LOD_Loader>(engine->resources(), filename))).first;

    return i->second.get();
}

GraphicsImage *AssetsManager::getBitmap(std::string_view name, bool generated) {
    std::string filename = ascii::toLower(name);

    auto i = bitmaps.find(filename);
    if (i == bitmaps.end()) {
        std::unique_ptr<ImageLoader> loader;
        if (generated) {
            loader = std::make_unique<Bitmaps_GEN_Loader>(engine->resources(), filename);
        } else {
            loader = std::make_unique<Bitmaps_LOD_Loader>(engine->resources(), filename);
        }
        i = bitmaps.emplace(filename, GraphicsImage::Create(std::move(loader))).first;
    }

    return i->second.get();
}

GraphicsImage *AssetsManager::getSprite(std::string_view name) {
    std::string filename = ascii::toLower(name);

    auto i = sprites.find(filename);
    if (i == sprites.end()) {
        std::unique_ptr<GraphicsImage> image = GraphicsImage::Create(std::make_unique<Sprites_LOD_Loader>(engine->resources(), filename));
        if (!image->size())
            return nullptr;
        i = sprites.emplace(filename, std::move(image)).first;
    }

    return i->second.get();
}

void AssetsManager::releaseSprites() {
    sprites.clear();
}
