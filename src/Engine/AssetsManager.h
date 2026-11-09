#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#include "Core/Time/Duration.h"
#include "GUI/GUIFont.h"

class GraphicsImage;
enum class Condition : uint32_t;
enum class PortraitId : uint16_t;
namespace mm7::mods { class PrivatePortraits; }

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
    /** Decodes MM6's selected-portrait sprite for the two-dimensional UI. */
    GraphicsImage *getMM6SelectionFrame();
    GraphicsImage *getMM6SelectionFrame(Duration elapsed);
    /** MM7-only local portrait, with condition fallback. Null retains the original art. */
    GraphicsImage *getCustomPortrait(int partySlot, PortraitId portrait, Condition condition);
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
    std::unique_ptr<mm7::mods::PrivatePortraits> customPortraits;
    std::unordered_map<std::string, std::unique_ptr<GraphicsImage>> mm6SelectionFrames;
    bool customPortraitsLoaded = false;
};

extern AssetsManager *assets;
