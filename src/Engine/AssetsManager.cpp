#include "Engine/AssetsManager.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Graphics/ImageLoader.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Resources/ResourceManager.h"

#include "Library/Logger/Logger.h"

#include "GUI/GUIFont.h"

#include "Utility/String/Ascii.h"

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
    for (const auto &[name, image] : mm6SelectionFrames)
        image->releaseRenderId();
    if (customPortraits)
        customPortraits->releaseRenderIds();

    ReloadFonts();
}

void AssetsManager::releaseImage(GraphicsImage *image) {
    for (auto it = mm6SelectionFrames.begin(); it != mm6SelectionFrames.end(); ++it)
        if (it->second.get() == image) {
            mm6SelectionFrames.erase(it);
            return;
        }
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

GraphicsImage *AssetsManager::getMM6SelectionFrame() {
    return getMM6SelectionFrame(Duration());
}

GraphicsImage *AssetsManager::getMM6SelectionFrame(Duration elapsed) {
    if (engine->gameTitle() != GameTitle::MM6)
        return nullptr;
    const int frameId = pSpriteFrameTable->FastFindSprite("aframe1");
    if (frameId <= 0)
        return nullptr;
    const SpriteFrame *frame = pSpriteFrameTable->GetFrame(frameId, elapsed);
    const std::string name = ascii::toLower(frame->textureName);
    if (auto it = mm6SelectionFrames.find(name); it != mm6SelectionFrames.end())
        return it->second.get();

    RgbaImage image = engine->resources()->sprite(name);
    if (!image)
        return nullptr;
    // Sprite decoding keeps palette indices in red for billboards; 2D HUD art needs colors.
    const Palette palette = engine->resources()->palette(frame->paletteId);
    for (Color &pixel : image.pixels()) {
        if (pixel.a)
            pixel = palette.colors[pixel.r];
    }
    return mm6SelectionFrames.emplace(name, GraphicsImage::Create(std::move(image))).first->second.get();
}

GraphicsImage *AssetsManager::getCustomPortrait(int partySlot, PortraitId portrait, Condition condition) {
    if (!engine || engine->gameTitle() != GameTitle::MM7)
        return nullptr;
    if (partySlot < 0 || partySlot >= 4)
        return nullptr;

    if (!customPortraitsLoaded) {
        customPortraitsLoaded = true;
        const char *directory = std::getenv("ENROTH_MM7_MOD_DIR");
        if (directory && *directory) {
            customPortraits = std::make_unique<mm7::mods::PrivatePortraits>();
            customPortraits->load(Path(directory));
        }
    }
    return customPortraits ? customPortraits->get(partySlot, portrait, condition) : nullptr;
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
