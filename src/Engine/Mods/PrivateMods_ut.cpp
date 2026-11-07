#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Graphics/Image.h"
#include "Engine/Mods/PrivateMods.h"

#include "Library/Image/Png.h"

namespace {

class PortraitTestDirectory {
 public:
    PortraitTestDirectory() {
        std::string pattern = (std::filesystem::temp_directory_path() / "mm7-portraits-XXXXXX").string();
        char *created = ::mkdtemp(pattern.data());
        if (created)
            directory = created;
    }

    ~PortraitTestDirectory() {
        if (!directory.empty())
            std::filesystem::remove_all(directory);
    }

    void write(std::string_view relative, Color color) {
        std::filesystem::path path = directory / relative;
        std::filesystem::create_directories(path.parent_path());
        Blob data = png::encode(RgbaImage::solid(color, 32, 32));
        std::ofstream file(path, std::ios::binary);
        file.write(static_cast<const char *>(data.data()), data.size());
    }

    std::filesystem::path directory;
};

}  // namespace

UNIT_TEST(MM7PrivatePortraits, MoodMapping) {
    using namespace mm7::mods;
    EXPECT_EQ(portraitMood(PORTRAIT_NORMAL, CONDITION_GOOD), PortraitMood::Normal);
    EXPECT_EQ(portraitMood(PORTRAIT_NORMAL, CONDITION_ZOMBIE), PortraitMood::Zombie);
    EXPECT_EQ(portraitMood(PORTRAIT_TALK, CONDITION_GOOD), PortraitMood::Normal);
    EXPECT_EQ(portraitMood(PORTRAIT_MOUTH_OPEN_A, CONDITION_GOOD), PortraitMood::Talk);
    EXPECT_EQ(portraitMood(PORTRAIT_DMGRECVD_MAJOR, CONDITION_GOOD), PortraitMood::Pain);
    EXPECT_EQ(portraitMood(PORTRAIT_POISONED, CONDITION_POISON_WEAK), PortraitMood::Sick);
    EXPECT_EQ(portraitMood(PORTRAIT_PETRIFIED, CONDITION_PETRIFIED), PortraitMood::Stone);
    EXPECT_EQ(portraitMood(PORTRAIT_NORMAL, CONDITION_ERADICATED), PortraitMood::Eradicated);
}

UNIT_TEST(MM7PrivatePortraits, LoadingAndFallback) {
    PortraitTestDirectory files;
    ASSERT_FALSE(files.directory.empty());
    files.write("portraits/1/normal.png", Color(20, 30, 40));
    files.write("portraits/1/hurt.png", Color(80, 90, 100));
    files.write("portraits/1/sleep.png", Color(40, 50, 60));
    files.write("portraits/1/zombie.png", Color(100, 110, 120));
    files.write("portrait-2.png", Color(10, 20, 30));
    {
        std::filesystem::path invalid = files.directory / "portraits/2/normal.png";
        std::filesystem::create_directories(invalid.parent_path());
        std::ofstream(invalid, std::ios::binary) << "invalid PNG";
    }
    files.write("portraits/3/dead.png", Color(1, 2, 3)); // No normal: slot stays original.

    mm7::mods::PrivatePortraits portraits;
    portraits.load(Path(files.directory.string()));

    GraphicsImage *normal = portraits.get(0, PORTRAIT_NORMAL, CONDITION_GOOD);
    ASSERT_NE(normal, nullptr);
    EXPECT_EQ(normal->name(), "mm7-mod-portrait-1-normal");
    EXPECT_EQ(normal->size(), Sizei(32, 32));
    EXPECT_EQ(portraits.get(0, PORTRAIT_DMGRECVD_MAJOR, CONDITION_GOOD)->name(), "mm7-mod-portrait-1-hurt");
    EXPECT_EQ(portraits.get(0, PORTRAIT_UNCONSCIOUS, CONDITION_GOOD)->name(), "mm7-mod-portrait-1-sleep");
    EXPECT_EQ(portraits.get(0, PORTRAIT_NORMAL, CONDITION_ZOMBIE)->name(), "mm7-mod-portrait-1-zombie");
    EXPECT_EQ(portraits.get(0, PORTRAIT_TALK, CONDITION_GOOD), normal);
    EXPECT_EQ(portraits.get(0, PORTRAIT_DEAD, CONDITION_DEAD), nullptr);
    EXPECT_EQ(portraits.get(0, PORTRAIT_ERADICATED, CONDITION_ERADICATED), nullptr);
    EXPECT_EQ(portraits.get(1, PORTRAIT_NORMAL, CONDITION_GOOD)->name(), "mm7-mod-portrait-2-normal");
    EXPECT_EQ(portraits.get(2, PORTRAIT_DEAD, CONDITION_DEAD), nullptr);
    EXPECT_EQ(portraits.get(3, PORTRAIT_NORMAL, CONDITION_GOOD), nullptr);
    EXPECT_EQ(portraits.get(-1, PORTRAIT_NORMAL, CONDITION_GOOD), nullptr);
}
