#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Archive.h"

#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormats.h"

static std::uint32_t headerWord(const Blob &blob, size_t index) {
    std::uint32_t result = 0;
    std::memcpy(&result, static_cast<const char *>(blob.data()) + index * 4, 4);
    return result;
}

UNIT_TEST(MM6Archive, EncodedEntryDecodesBack) {
    // A map delta written back into the session must decode to the same bytes it was made from.
    std::string repetitive(5000, 'x');
    std::string mixed;
    for (int i = 0; i < 3000; ++i)
        mixed.push_back(static_cast<char>((i * 7919 + i / 13) & 0xFF));

    for (const std::string &data : {std::string("a"), repetitive, mixed}) {
        const Blob source = Blob::fromString(data);
        const Blob encoded = encodeMM6ArchiveEntry(source);
        ASSERT_GE(encoded.size(), 8);
        EXPECT_EQ(headerWord(encoded, 0), encoded.size() - 8);
        EXPECT_EQ(headerWord(encoded, 1), data.size());
        EXPECT_FALSE(lod::detectCompressedData(encoded));
        EXPECT_EQ(decodeMM6ArchiveEntry(encoded).str(), data);
    }
    EXPECT_LT(encodeMM6ArchiveEntry(Blob::fromString(repetitive)).size(), repetitive.size());
}

UNIT_TEST(MM6Archive, RefusesEmptyEntry) {
    // An empty entry has no MM6 archive form, its header would read back as an uncompressed entry.
    EXPECT_ANY_THROW((void) encodeMM6ArchiveEntry(Blob()));
}

UNIT_TEST(MM6Archive, ShippedDeltasRoundTrip) {
    // Every map delta shipped in the MM6 games archive re-encodes to an entry that decodes to the same bytes.
    const char *archive = std::getenv("ENROTH_MM6_STAGED_GAMES_LOD");
    if (!archive)
        GTEST_SKIP() << "private staged MM6 games archive is unavailable";

    const LodReader games{PathView(archive)};
    int deltas = 0;
    for (const std::string &name : games.ls()) {
        if (!name.ends_with(".ddm") && !name.ends_with(".dlv"))
            continue;
        const Blob original = games.read(name);
        const Blob decoded = decodeMM6ArchiveEntry(original);
        ASSERT_NE(decoded.size(), original.size()) << name;
        const Blob encoded = encodeMM6ArchiveEntry(decoded);
        EXPECT_EQ(decodeMM6ArchiveEntry(encoded).str(), decoded.str()) << name;
        deltas++;
    }
    EXPECT_EQ(deltas, 67);
}
