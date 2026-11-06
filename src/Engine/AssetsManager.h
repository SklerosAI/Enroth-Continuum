#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <memory>

#include "GUI/GUIFont.h"

class GraphicsImage;

class AssetsManager {
 public:
    AssetsManager();
    ~AssetsManager();

    void releaseAllTextures();

    /**
     * Removes an image from the cache and frees it.
     *
     * @param image                     Image returned by one of the getters below.
     */
    void releaseImage(GraphicsImage *image);

    GraphicsImage *getIcon(std::string_view name);
    /** Decodes MM6's palette-zero aframe1 sprite for the two-dimensional portrait UI. */
    GraphicsImage *getMM6SelectionFrame();
    /** Local, per-party-slot PNG override. Returns nullptr when vanilla mode is selected or the image is invalid. */
    GraphicsImage *getCustomPortrait(int partySlot);
    GraphicsImage *getImage_Buff(std::string_view name);

    GraphicsImage *getBitmap(std::string_view name, bool generated = false);

    /**
     * @param name                      Name of a sprite in `sprites.lod`, case-insensitive.
     * @return                          The sprite, or `nullptr` if there is no such sprite.
     */
    GraphicsImage *getSprite(std::string_view name);

    void releaseSprites();

    std::unique_ptr<GUIFont> pFontBookOnlyShadow;
    std::unique_ptr<GUIFont> pFontBookLloyds;
    std::unique_ptr<GUIFont> pFontArrus;
    std::unique_ptr<GUIFont> pFontLucida;
    std::unique_ptr<GUIFont> pFontBookTitle;
    std::unique_ptr<GUIFont> pFontBookCalendar;
    std::unique_ptr<GUIFont> pFontCreate;
    std::unique_ptr<GUIFont> pFontCChar;
    std::unique_ptr<GUIFont> pFontComic;
    std::unique_ptr<GUIFont> pFontSmallnum;

 protected:
    std::unordered_map<std::string, std::unique_ptr<GraphicsImage>> bitmaps;
    std::unordered_map<std::string, std::unique_ptr<GraphicsImage>> sprites;
    std::unordered_map<std::string, std::unique_ptr<GraphicsImage>> images;
    std::array<std::unique_ptr<GraphicsImage>, 4> customPortraits;
    std::unique_ptr<GraphicsImage> mm6SelectionFrame;
    bool customPortraitsLoaded = false;
};

extern AssetsManager *assets;
