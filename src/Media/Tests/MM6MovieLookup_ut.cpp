#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "Library/Snapshots/CommonSnapshots.h"
#include "Library/Vid/VidReader.h"
#include "Library/Vid/VidSnapshots.h"

#include "Media/MM6MovieLookup.h"

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Utility/Streams/BlobOutputStream.h"

static Blob makeMovieVid(const std::vector<std::string> &names, std::string_view payload = "SMK00000") {
    Blob result;
    BlobOutputStream stream(&result);
    serialize(uint32_t(names.size()), &stream);
    for (size_t i = 0; i < names.size(); ++i) {
        VidEntry_MM7 entry = {};
        snapshot(names[i], &entry.name);
        entry.offset = 4 + names.size() * sizeof(VidEntry_MM7) + i * payload.size();
        serialize(entry, &stream);
    }
    for (size_t i = 0; i < names.size(); ++i)
        stream.write(payload);
    stream.close();
    return result;
}

UNIT_TEST(MM6MovieLookup, UsesOnlyExactExtensionlessNamesAndValidSmacker) {
    VidReader first(makeMovieVid({"Intro"}));
    VidReader second(makeMovieVid({"House"}));
    EXPECT_EQ(lookupMovieBlob(GameTitle::MM6, first, second, "intro").str(), "SMK00000");
    EXPECT_EQ(lookupMovieBlob(GameTitle::MM6, first, second, "HOUSE").str(), "SMK00000");
    EXPECT_FALSE(lookupMovieBlob(GameTitle::MM6, first, second, "Intro.smk"));
    EXPECT_FALSE(lookupMovieBlob(GameTitle::MM6, first, second, "Missing"));

    VidReader duplicate(makeMovieVid({"INTRO"}));
    EXPECT_THROW(lookupMovieBlob(GameTitle::MM6, first, duplicate, "intro"), std::runtime_error);

    VidReader malformed(makeMovieVid({"Broken"}, "BAD00000"));
    EXPECT_THROW(lookupMovieBlob(GameTitle::MM6, malformed, second, "Broken"), std::runtime_error);
}

UNIT_TEST(MM6MovieLookup, RetainsMM7SuffixAndArchivePriority) {
    VidReader first(makeMovieVid({"Intro.smk", "Intro.bik"}, "FIRST000"));
    VidReader second(makeMovieVid({"Intro.bik"}, "SECOND00"));
    EXPECT_EQ(lookupMovieBlob(GameTitle::MM7, first, second, "intro").str(), "FIRST000");
    EXPECT_FALSE(lookupMovieBlob(GameTitle::MM7, first, second, "Missing"));
}

UNIT_TEST(MM6MovieLookup, ResolvesEveryShippedMovieByItsExtensionlessName) {
    const auto data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;

    VidReader first(data->read("anims/anims1.vid"));
    VidReader second(data->read("anims/anims2.vid"));
    std::vector<std::string> names = first.ls();
    ASSERT_EQ(names.size(), 40);
    std::ranges::copy(second.ls(), std::back_inserter(names));
    ASSERT_EQ(names.size(), 127);
    for (const std::string &name : names) {
        EXPECT_TRUE(lookupMovieBlob(GameTitle::MM6, first, second, name)) << name;
        EXPECT_FALSE(lookupMovieBlob(GameTitle::MM6, first, second, name + ".smk")) << name;
    }
}
