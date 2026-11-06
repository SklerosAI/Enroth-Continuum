#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Library/Snapshots/CommonSnapshots.h"
#include "Library/Vid/VidReader.h"
#include "Library/Vid/VidSnapshots.h"

#include "Media/MM6MovieLookup.h"

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

static std::vector<std::string> retainedNames(const std::filesystem::path &path) {
    std::ifstream input(path);
    std::vector<std::string> result;
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty()) result.push_back(std::move(line));
    }
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

UNIT_TEST(MM6MovieLookup, CoversF3MatchedRetainedNameInventory) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "private retained MM6 VID inventory is unavailable";

    const auto firstNames = retainedNames(std::filesystem::path(research) / "Anims1.entries");
    const auto secondNames = retainedNames(std::filesystem::path(research) / "Anims2.entries");
    ASSERT_EQ(firstNames.size(), 40);
    ASSERT_EQ(secondNames.size(), 87);
    VidReader first(makeMovieVid(firstNames));
    VidReader second(makeMovieVid(secondNames));
    for (const std::string &name : firstNames) {
        EXPECT_TRUE(lookupMovieBlob(GameTitle::MM6, first, second, name));
        EXPECT_FALSE(lookupMovieBlob(GameTitle::MM6, first, second, name + ".smk"));
    }
    for (const std::string &name : secondNames) {
        EXPECT_TRUE(lookupMovieBlob(GameTitle::MM6, first, second, name));
        EXPECT_FALSE(lookupMovieBlob(GameTitle::MM6, first, second, name + ".smk"));
    }
}
