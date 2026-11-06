#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Resources/ResourceManager.h"

#include "GUI/MM6TopPanelResources.h"

TEST(MM6TopPanelSelector, OriginalHourlyThresholdsAndTitleIsolation) {
    for (int hour = 0; hour < 24; ++hour) {
        const std::string_view expected = hour < 5 ? "tap4" : hour == 5 ? "tap3" :
                                          hour < 20 ? "tap2" : hour == 20 ? "tap1" : "tap4";
        EXPECT_EQ(mm6TopPanelIconForHour(GameTitle::MM6, hour), expected) << hour;
        EXPECT_TRUE(mm6TopPanelIconForHour(GameTitle::MM7, hour).empty()) << hour;
    }
    EXPECT_EQ(mm6TopPanelIconForHour(GameTitle::MM6, -1), "tap4");
    EXPECT_EQ(mm6TopPanelIconForHour(GameTitle::MM6, 24), "tap4");
}

GAME_TEST(MM6TopPanel, NativeImagesAndPaletteZeroAperture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    for (std::string_view name : {"tap1", "tap2", "tap3", "tap4"}) {
        LodImage raw = engine->resources()->rawIcon(name);
        RgbaImage image = engine->resources()->icon(name);
        ASSERT_EQ(raw.image.size(), Sizei(172, 142));
        ASSERT_EQ(image.size(), raw.image.size());
        ASSERT_FALSE(raw.zeroIsTransparent);
        int transparent = 0;
        int opaque = 0;
        for (size_t i = 0; i < raw.image.pixels().size(); ++i) {
            if (raw.image.pixels()[i] == 0) {
                EXPECT_EQ(image.pixels()[i].a, 0) << name << " at " << i;
                ++transparent;
            } else if (image.pixels()[i].a == 255) {
                ++opaque;
            }
        }
        EXPECT_GT(transparent, 0) << name;
        EXPECT_GT(opaque, 0) << name;
    }
}
