#include <array>
#include <cstdint>
#include <exception>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6KnownSaveFields.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6ShopTitleState.h"

namespace {

class TestProfile {
 public:
    TestProfile() {
#ifndef _WIN32
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-atomic-XXXXXX").string();
        char *result = ::mkdtemp(pattern.data());
        if (!result)
            throw std::runtime_error("cannot create MM6 test profile");
        _root = result;
#endif
    }

    ~TestProfile() {
        if (!_root.empty()) {
            std::error_code error;
            std::filesystem::remove_all(_root, error);
        }
    }

    const std::filesystem::path &root() const { return _root; }
    std::filesystem::path save() const { return _root / "saves" / "save001.ec6"; }
    std::filesystem::path backup() const { return _root / "saves" / "save001.ec6.bak"; }

 private:
    std::filesystem::path _root;
};

std::vector<std::uint8_t> sample(std::uint8_t marker) {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = marker;
    fields.core.party.activeCharacter = 2;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "Sample " + std::to_string(marker);
    fields.metadata.header.mapFileName = "d0.blv";
    fields.metadata.header.playingTicks = marker;
    fields.metadata.ui.activeCharacter = 2;
    fields.recovery.playingTicks = marker;
    for (int i = 0; i < 52; ++i)
        fields.maps.deltas.push_back({"d" + std::to_string(i) + ".dlv", {1}});
    for (int i = 0; i < 15; ++i)
        fields.maps.deltas.push_back({"o" + std::to_string(i) + ".ddm", {2}});
    fields.chest = {marker, {}, "d0.dlv", {1}};
    fields.shopGuildClocks.ready = true;
    fields.npcs.activeCount = 399;
    return mm6::encodeKnownSave(fields);
}

std::vector<std::uint8_t> malformedKnownSave(std::uint8_t marker) {
    mm6::SaveContainer container = mm6::decodeSaveContainer(sample(marker));
    for (mm6::SaveSection &section : container.sections) {
        if (section.tag == std::array{'H', 'E', 'A', 'D'}) {
            section.payload[0] = 2; // Unsupported HEAD field version with valid container CRCs.
            return mm6::encodeSaveContainer(container);
        }
    }
    throw std::runtime_error("missing synthetic MM6 HEAD section");
}

std::vector<std::uint8_t> sampleRaw(std::uint8_t marker) {
    const mm6::KnownSaveFields old = mm6::decodeKnownSave(sample(marker));
    mm6::RawSaveFields fields;
    fields.core = old.core;
    fields.bits = old.durable.bits;
    fields.effects = old.durable.effects;
    fields.inventory.ready = true;
    fields.metadata = old.metadata;
    fields.metadata.header.displayName = "Raw " + std::to_string(marker);
    fields.maps = old.maps;
    fields.npcs = old.npcs;
    fields.resistances = old.resistances;
    fields.buffs = old.buffs;
    fields.hirelings = old.hirelings;
    fields.recovery = old.recovery;
    fields.chest = old.chest;
    fields.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    fields.inventory.characters[0].items[137].setWord(0, 10);
    fields.inventory.characters[0].items[137][27] = marker;
    fields.inventory.characters[0].gridWords[0] = 138;
    fields.heldItem.record[27] = marker;
    return mm6::encodeRawSave(fields, GameTitle::MM6);
}

std::vector<std::uint8_t> malformedRawSave(std::uint8_t marker) {
    mm6::SaveContainer container = mm6::decodeSaveContainer(sampleRaw(marker));
    for (mm6::SaveSection &section : container.sections) {
        if (section.tag == std::array{'H', 'E', 'A', 'D'}) {
            section.payload[0] = 2;
            return mm6::encodeSaveContainer(container);
        }
    }
    throw std::runtime_error("missing synthetic MM6 raw HEAD section");
}

std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void write(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

} // namespace

#ifndef _WIN32
UNIT_TEST(MM6AtomicSaveFile, InitialReplaceAndRecoverPrevious) {
    TestProfile profile;
    std::vector<std::uint8_t> old = sample(1);
    std::vector<std::uint8_t> next = sample(2);
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), std::nullopt);

    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
    EXPECT_EQ(read(profile.save()), old);
    EXPECT_FALSE(std::filesystem::exists(profile.backup()));

    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", next);
    EXPECT_EQ(read(profile.save()), next);
    EXPECT_EQ(read(profile.backup()), old);

    {
        std::ofstream corrupt(profile.save(), std::ios::binary);
        std::vector<std::uint8_t> invalid = malformedKnownSave(3);
        corrupt.write(reinterpret_cast<const char *>(invalid.data()), invalid.size());
    }
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
    EXPECT_EQ(read(profile.save()), old);
    EXPECT_EQ(read(profile.backup()), old);

    std::filesystem::remove(profile.save());
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
    EXPECT_EQ(read(profile.save()), old);
}

UNIT_TEST(MM6AtomicSaveFile, FaultsPreservePreviousValidSave) {
    for (mm6::detail::AtomicSaveStep step : {
             mm6::detail::AtomicSaveStep::BeforeTempWrite,
             mm6::detail::AtomicSaveStep::AfterPartialTempWrite,
             mm6::detail::AtomicSaveStep::BeforeBackupReplace,
             mm6::detail::AtomicSaveStep::BeforePrimaryReplace}) {
        TestProfile profile;
        std::vector<std::uint8_t> old = sample(3);
        std::vector<std::uint8_t> next = sample(4);
        mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
        EXPECT_THROW(mm6::detail::writeAtomicSaveFile(profile.root(), "save001.ec6", next,
            [step](mm6::detail::AtomicSaveStep reached) {
                if (reached == step)
                    throw std::runtime_error("injected MM6 write failure");
            }), std::exception);
        EXPECT_EQ(read(profile.save()), old);
        EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
        if (std::filesystem::exists(profile.backup()))
            EXPECT_EQ(read(profile.backup()), old);
        for (const auto &entry : std::filesystem::directory_iterator(profile.root() / "saves"))
            EXPECT_FALSE(entry.path().filename().string().contains(".tmp."));
    }
}

UNIT_TEST(MM6AtomicSaveFile, RejectsInvalidCandidateAndDestination) {
    TestProfile profile;
    std::vector<std::uint8_t> old = sample(5);
    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
    std::vector<std::uint8_t> wrongTitle = sample(6);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", wrongTitle), std::exception);
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", malformedKnownSave(6)), std::exception);
    mm6::SaveContainer partial;
    partial.sections.push_back({{'H', 'E', 'A', 'D'}, true, {1}});
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6",
                                         mm6::encodeSaveContainer(partial)), std::exception);
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "../save001.ec6", old), std::exception);
    EXPECT_EQ(read(profile.save()), old);

    { std::ofstream corrupt(profile.save(), std::ios::binary); corrupt << "bad"; }
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", sample(7)), std::exception);
    EXPECT_EQ(read(profile.save()), std::vector<std::uint8_t>({'b', 'a', 'd'}));
    EXPECT_THROW((void) mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), std::exception);
}

UNIT_TEST(MM6AtomicSaveFile, ListsOnlyKnownMM6SlotsWithoutRecoveringThem) {
    TestProfile profile;
    EXPECT_TRUE(mm6::listKnownSaveSlots(profile.root()).empty());
    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", sample(1));
    mm6::writeAtomicSaveFile(profile.root(), "save002.ec6", sample(2));
    mm6::writeAtomicSaveFile(profile.root(), "save002.ec6", sample(3));
    std::filesystem::remove(profile.root() / "saves/save002.ec6");
    mm6::writeAtomicSaveFile(profile.root(), "save003.ec6", sample(4));
    mm6::writeAtomicSaveFile(profile.root(), "save003.ec6", sample(5));
    write(profile.root() / "saves/save003.ec6", malformedKnownSave(6));
    write(profile.root() / "saves/save004.ec6", malformedKnownSave(7));
    write(profile.root() / "saves/save005.mm7", sample(8));
    write(profile.root() / "saves/.save006.ec6.tmp.123456", sample(9));

    std::vector<mm6::SaveSlotInfo> slots = mm6::listKnownSaveSlots(profile.root());
    ASSERT_EQ(slots.size(), 3);
    EXPECT_EQ(slots[0].fileName, "save001.ec6");
    EXPECT_EQ(slots[0].header.displayName, "Sample 1");
    EXPECT_FALSE(slots[0].needsRecovery);
    EXPECT_EQ(slots[1].fileName, "save002.ec6");
    EXPECT_EQ(slots[1].header.displayName, "Sample 2");
    EXPECT_TRUE(slots[1].needsRecovery);
    EXPECT_EQ(slots[2].fileName, "save003.ec6");
    EXPECT_EQ(slots[2].header.displayName, "Sample 4");
    EXPECT_TRUE(slots[2].needsRecovery);
    EXPECT_FALSE(std::filesystem::exists(profile.root() / "saves/save002.ec6"));
    EXPECT_EQ(read(profile.root() / "saves/save003.ec6"), malformedKnownSave(6));

    std::filesystem::create_symlink(profile.save(), profile.root() / "saves/save006.ec6");
    EXPECT_THROW((void) mm6::listKnownSaveSlots(profile.root()), std::exception);
    std::filesystem::remove(profile.root() / "saves/save006.ec6");
    TestProfile redirected;
    std::filesystem::create_directory_symlink(profile.root() / "saves", redirected.root() / "saves");
    EXPECT_THROW((void) mm6::listKnownSaveSlots(redirected.root()), std::exception);
}

UNIT_TEST(MM6AtomicSaveFile, SemanticLoadPreflightSelectsBackupWithoutDiskMutation) {
    TestProfile profile;
    mm6::SaveCandidateValidator accept = [](std::span<const std::uint8_t>) {};
    EXPECT_FALSE(mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", accept).has_value());
    EXPECT_THROW((void) mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", {}), std::exception);

    std::vector<std::uint8_t> old = sample(10);
    std::vector<std::uint8_t> next = sample(11);
    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", next);
    std::optional<mm6::SaveFileCandidate> candidate =
        mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", accept);
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->bytes, next);
    EXPECT_FALSE(candidate->needsRecovery);
    mm6::SaveCandidateValidator rejectNext = [](std::span<const std::uint8_t> bytes) {
        if (mm6::decodeKnownSave(bytes).metadata.header.displayName == "Sample 11")
            throw std::runtime_error("synthetic semantic rejection");
    };
    candidate = mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", rejectNext);
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->bytes, old);
    EXPECT_TRUE(candidate->needsRecovery);
    EXPECT_EQ(read(profile.save()), next);
    EXPECT_EQ(read(profile.backup()), old);

    mm6::SaveCandidateValidator rejectAll = [](std::span<const std::uint8_t>) {
        throw std::runtime_error("synthetic semantic rejection");
    };
    EXPECT_THROW((void) mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", rejectAll),
                 std::exception);
    EXPECT_EQ(read(profile.save()), next);
    EXPECT_EQ(read(profile.backup()), old);

    std::filesystem::remove(profile.save());
    candidate = mm6::preflightRecoverableSaveFile(profile.root(), "save001.ec6", accept);
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->bytes, old);
    EXPECT_TRUE(candidate->needsRecovery);
    EXPECT_FALSE(std::filesystem::exists(profile.save()));
}

UNIT_TEST(MM6AtomicSaveFile, RawV2SlotsAreAtomicAndSchemaIsolated) {
    TestProfile legacy;
    const auto legacyBytes = sample(20);
    mm6::writeAtomicSaveFile(legacy.root(), "save001.ec6", legacyBytes);
    EXPECT_THROW(mm6::writeAtomicRawSaveFile(legacy.root(), "save001.ec6", sampleRaw(21)), std::exception);
    EXPECT_EQ(read(legacy.save()), legacyBytes);

    TestProfile profile;
    const auto first = sampleRaw(21);
    const auto second = sampleRaw(22);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", first);
    EXPECT_EQ(read(profile.save()), first);
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", sample(23)), std::exception);
    EXPECT_EQ(read(profile.save()), first);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", second);
    EXPECT_EQ(read(profile.save()), second);
    EXPECT_EQ(read(profile.backup()), first);
    EXPECT_THROW(mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", sample(24)), std::exception);
    EXPECT_EQ(read(profile.save()), second);

    const auto rawSlots = mm6::listRawSaveSlots(profile.root());
    ASSERT_EQ(rawSlots.size(), 1u);
    EXPECT_EQ(rawSlots[0].header.displayName, "Raw 22");
    EXPECT_TRUE(mm6::listKnownSaveSlots(profile.root()).empty());
    EXPECT_THROW((void) mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), std::exception);

    write(profile.save(), malformedRawSave(25));
    const auto selected = mm6::preflightRecoverableRawSaveFile(
        profile.root(), "save001.ec6", [](std::span<const std::uint8_t>) {});
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, first);
    EXPECT_EQ(read(profile.save()), malformedRawSave(25));
    EXPECT_EQ(mm6::loadRecoverableRawSaveFile(profile.root(), "save001.ec6"), first);
    EXPECT_EQ(read(profile.save()), first);
}

UNIT_TEST(MM6AtomicSaveFile, RawMenuPreflightUsesSameReadOnlyBackupSelection) {
    TestProfile profile;
    const auto first = sampleRaw(41);
    const auto second = sampleRaw(42);
    const auto excluded = sampleRaw(43);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", first);
    mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", second);
    mm6::writeAtomicRawSaveFile(profile.root(), "save002.ec6", excluded);

    EXPECT_THROW((void) mm6::listRawSaveSlotsWithPreflight(profile.root(), {}),
                 std::exception);
    const auto acceptOnlyFirst = [](std::span<const std::uint8_t> bytes) {
        if (mm6::decodeRawSave(bytes, GameTitle::MM6).metadata.header.displayName != "Raw 41")
            throw std::runtime_error("synthetic gameplay refusal");
    };
    const auto slots = mm6::listRawSaveSlotsWithPreflight(profile.root(), acceptOnlyFirst);
    ASSERT_EQ(slots.size(), 1u);
    EXPECT_EQ(slots[0].fileName, "save001.ec6");
    EXPECT_EQ(slots[0].header.displayName, "Raw 41");
    EXPECT_TRUE(slots[0].needsRecovery);
    EXPECT_EQ(read(profile.save()), second);
    EXPECT_EQ(read(profile.backup()), first);
    EXPECT_EQ(read(profile.root() / "saves/save002.ec6"), excluded);

    const auto all = mm6::listRawSaveSlotsWithPreflight(
        profile.root(), [](std::span<const std::uint8_t>) {});
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].header.displayName, "Raw 42");
    EXPECT_FALSE(all[0].needsRecovery);
    EXPECT_EQ(all[1].header.displayName, "Raw 43");
}

UNIT_TEST(MM6AtomicSaveFile, RawV2WriteFaultsKeepPriorSlot) {
    for (mm6::detail::AtomicSaveStep step : {
             mm6::detail::AtomicSaveStep::BeforeTempWrite,
             mm6::detail::AtomicSaveStep::AfterPartialTempWrite,
             mm6::detail::AtomicSaveStep::BeforeBackupReplace,
             mm6::detail::AtomicSaveStep::BeforePrimaryReplace}) {
        TestProfile profile;
        const auto first = sampleRaw(31);
        const auto second = sampleRaw(32);
        mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", first);
        EXPECT_THROW(mm6::detail::writeAtomicRawSaveFile(profile.root(), "save001.ec6", second,
            [step](mm6::detail::AtomicSaveStep reached) {
                if (reached == step)
                    throw std::runtime_error("injected MM6 raw write failure");
            }), std::exception);
        EXPECT_EQ(read(profile.save()), first);
        EXPECT_EQ(mm6::loadRecoverableRawSaveFile(profile.root(), "save001.ec6"), first);
        for (const auto &entry : std::filesystem::directory_iterator(profile.root() / "saves"))
            EXPECT_FALSE(entry.path().filename().string().contains(".tmp."));
    }
}
#endif
