#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ArtifactCollectionSection.h"

UNIT_TEST(MM6ArtifactCollectionSection, RoundTripsTheLedger) {
    // The ledger comes back from a save exactly as the party left it.
    mm6::ArtifactCollection collection;
    collection.initialized = true;
    collection.collected[0] = 1;
    collection.collected[15] = 1;
    collection.collected[29] = 1;
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeArtifactCollectionSection(collection, GameTitle::MM6).value());

    EXPECT_FALSE(container.sections[0].required);
    EXPECT_EQ(mm6::decodeArtifactCollectionSection(mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)), GameTitle::MM6), collection);
}

UNIT_TEST(MM6ArtifactCollectionSection, LedgerTheModNeverStartedIsNotSaved) {
    // A save of a game that never ran the All Artifacts mod keeps the bytes it had before the mod existed.
    EXPECT_EQ(mm6::encodeArtifactCollectionSection(mm6::ArtifactCollection(), GameTitle::MM6), std::nullopt);
    EXPECT_EQ(mm6::decodeArtifactCollectionSection(mm6::SaveContainer(), GameTitle::MM6), mm6::ArtifactCollection());
}

UNIT_TEST(MM6ArtifactCollectionSection, RefusesMalformedSections) {
    // A damaged ledger refuses the save rather than loading it with the wrong artifacts collected.
    mm6::ArtifactCollection collection;
    collection.initialized = true;
    const mm6::SaveSection valid = *mm6::encodeArtifactCollectionSection(collection, GameTitle::MM6);

    mm6::SaveContainer truncated;
    truncated.sections.push_back(valid);
    truncated.sections[0].payload.pop_back();
    EXPECT_ANY_THROW((void) mm6::decodeArtifactCollectionSection(truncated, GameTitle::MM6));

    mm6::SaveContainer duplicate;
    duplicate.sections = {valid, valid};
    EXPECT_ANY_THROW((void) mm6::decodeArtifactCollectionSection(duplicate, GameTitle::MM6));

    mm6::SaveContainer badFlag;
    badFlag.sections.push_back(valid);
    badFlag.sections[0].payload[5] = 2;
    EXPECT_ANY_THROW((void) mm6::decodeArtifactCollectionSection(badFlag, GameTitle::MM6));

    mm6::SaveContainer badVersion;
    badVersion.sections.push_back(valid);
    badVersion.sections[0].payload[0] = 2;
    EXPECT_ANY_THROW((void) mm6::decodeArtifactCollectionSection(badVersion, GameTitle::MM6));

    mm6::SaveContainer required;
    required.sections.push_back(valid);
    required.sections[0].required = true;
    EXPECT_ANY_THROW((void) mm6::decodeArtifactCollectionSection(required, GameTitle::MM6));

    EXPECT_ANY_THROW((void) mm6::encodeArtifactCollectionSection(collection, GameTitle::MM7));
}
