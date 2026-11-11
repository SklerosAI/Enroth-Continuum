#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6V5FirstMapSaveWriter.h"
#include "Engine/MM6/MM6V5FirstMapNativeSlotPolicy.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6KnownSaveFields.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6SaveContainer.h"
#include "Engine/MM6/MM6SessionSave.h"
#include "Engine/MM6/MM6ShopTitleState.h"
#include "Engine/MM6/MM6V3SaveFields.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Objects/Actor.h"

namespace {

class TestProfile {
 public:
    explicit TestProfile(std::string_view prefix = "mm6-atomic-") {
#ifndef _WIN32
        std::string pattern = (std::filesystem::temp_directory_path() /
                               (std::string(prefix) + "XXXXXX")).string();
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

std::vector<std::uint8_t> sampleV3(std::uint8_t marker) {
    mm6::V3SaveFields fields;
    fields.raw = mm6::decodeRawSave(sampleRaw(marker), GameTitle::MM6);
    fields.raw.metadata.header.displayName = "V3 " + std::to_string(marker);
    const std::array<Actor, 0> actors{};
    fields.activeActorPayload = mm6::encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs.mapName = fields.raw.metadata.header.mapFileName;
    fields.gameplayRng.type = RANDOM_ENGINE_SEQUENTIAL;
    fields.gameplayRng.sequentialState = marker;
    return mm6::encodeV3Save(fields, GameTitle::MM6);
}

std::vector<std::uint8_t> sampleV4(std::uint8_t marker) {
    mm6::V4SaveFields fields;
    fields.v3 = mm6::decodeV3Save(sampleV3(marker), GameTitle::MM6);
    mm6::EventRuntimeMapEntry entry;
    entry.mapName = "d0.blv";
    entry.respawnCount = 1;
    entry.lastRespawnDay = 1;
    fields.eventRuntime.maps.push_back(entry);
    return mm6::encodeV4Save(fields, GameTitle::MM6);
}

std::vector<std::uint8_t> sampleV5(std::uint8_t marker) {
    mm6::V5SaveFields fields;
    fields.v4.v3 = mm6::decodeV3Save(sampleV3(marker), GameTitle::MM6);
    std::vector<std::uint8_t> native(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56);
    fields.v4.v3.raw.metadata.header.mapFileName = "o0.odm";
    fields.v4.v3.raw.chest.activeDeltaName = "o0.ddm";
    fields.v4.v3.raw.chest.rawActiveDelta = native;
    fields.v4.v3.generatedNpcs.mapName = "o0.odm";
    for (auto &entry : fields.v4.v3.raw.maps.deltas)
        if (entry.name == "o0.ddm")
            entry.bytes = native;
    mm6::EventRuntimeMapEntry event;
    event.mapName = "o0.odm";
    event.respawnCount = 1;
    event.lastRespawnDay = 1;
    fields.v4.eventRuntime.maps.push_back(event);
    fields.sprites = mm6::captureSpriteRuntimeFields(
        "o0.odm", native, std::span<const SpriteObject>{}, GameTitle::MM6);
    fields.actors = mm6::captureActorRuntimeFields(
        "o0.odm", fields.v4.v3.activeActorPayload,
        std::span<const Actor>{}, GameTitle::MM6);
    fields.decorations.mapName = "o0.odm";
    return mm6::encodeV5Save(fields, GameTitle::MM6);
}

std::vector<std::uint8_t> sampleFirstMapV5(std::uint8_t marker) {
    auto fields = mm6::decodeV5Save(sampleV5(marker), GameTitle::MM6);
    fields.v4.v3.raw.metadata.header.mapFileName = "oute3.odm";
    fields.v4.v3.raw.chest.activeDeltaName = "oute3.ddm";
    fields.v4.v3.generatedNpcs.mapName = "oute3.odm";
    for (auto &entry : fields.v4.v3.raw.maps.deltas)
        if (entry.name == "o0.ddm")
            entry.name = "oute3.ddm";
    fields.v4.eventRuntime.maps[0].mapName = "oute3.odm";
    fields.sprites.mapName = "oute3.odm";
    fields.actors.mapName = "oute3.odm";
    fields.decorations.mapName = "oute3.odm";
    return mm6::encodeV5Save(fields, GameTitle::MM6);
}

std::vector<std::uint8_t> sampleV7(std::uint8_t marker) {
    mm6::SessionSaveFields fields;
    fields.raw = mm6::decodeRawSave(sampleRaw(marker), GameTitle::MM6);
    fields.raw.metadata.header.displayName = "V7 " + std::to_string(marker);
    mm6::ParkedMapRuntime active;
    active.mapName = "d0.blv";
    active.variables.mapVars.fill(marker);
    active.variables.decorVars.fill(0);
    fields.maps.push_back(active);
    return mm6::encodeSessionSave(fields, GameTitle::MM6);
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
UNIT_TEST(MM6AtomicSaveFile, NativeFirstMapSlotRefusesRootAndCollision) {
    TestProfile profile;
    const auto root = std::filesystem::canonical(profile.root());
    const mm6::V5FirstMapNativeRoot binding{root, root};
    const auto first = sampleFirstMapV5(120);
    const auto otherMap = sampleV5(121);
    const auto accept = [](std::span<const std::uint8_t>) {};
    const auto slot = root / "saves/save001.ec6";
    const auto backup = root / "saves/save001.ec6.bak";
    const auto stage = [&](const mm6::V5FirstMapNativeRoot &source,
                           std::string_view name = "save001.ec6",
                           GameTitle title = GameTitle::MM6) {
        return mm6::preflightV5FirstMapNativeWriteSlot(
            source, name, first, title, accept);
    };
    EXPECT_FALSE(stage(binding).primary);
    EXPECT_THROW((void) stage({root, root / "other"}), std::exception);
    EXPECT_THROW((void) stage(binding, "quicksave.ec6"), std::exception);
    EXPECT_THROW((void) stage(binding, "save001.mm7"), std::exception);
    EXPECT_THROW((void) stage(binding, "save001.ec6", GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::preflightV5FirstMapNativeWriteSlot(
        binding, "save001.ec6", first, GameTitle::MM6, {}), std::exception);
    EXPECT_FALSE(std::filesystem::exists(root / "saves"));

    std::filesystem::create_directory(root / "saves");
    write(slot, sampleV4(122));
    EXPECT_THROW((void) stage(binding), std::exception);
    EXPECT_EQ(read(slot), sampleV4(122));
    write(slot, otherMap);
    EXPECT_THROW((void) stage(binding), std::exception);
    EXPECT_EQ(read(slot), otherMap);
    write(slot, {1, 2, 3});
    EXPECT_THROW((void) stage(binding), std::exception);
    EXPECT_EQ(read(slot), (std::vector<std::uint8_t>{1, 2, 3}));
    std::filesystem::remove(slot);
    write(backup, first);
    EXPECT_THROW((void) stage(binding), std::exception);
    EXPECT_EQ(read(backup), first);
    std::filesystem::remove(backup);
    write(slot, first);
    EXPECT_EQ(stage(binding).primary, first);
    EXPECT_FALSE(stage(binding).backup);
    write(backup, {1, 2, 3});
    EXPECT_THROW((void) stage(binding), std::exception);
    EXPECT_EQ(read(slot), first);
    EXPECT_EQ(read(backup), (std::vector<std::uint8_t>{1, 2, 3}));
    std::filesystem::remove(backup);
    std::filesystem::remove(slot);
    std::filesystem::remove(root / "saves");
    std::filesystem::create_directory_symlink(root, root / "saves");
    EXPECT_THROW((void) stage(binding), std::exception);
}

UNIT_TEST(MM6AtomicSaveFile, NativeFirstMapSlotClassifiesAtomicFaults) {
    TestProfile profile;
    const auto root = std::filesystem::canonical(profile.root());
    const mm6::V5FirstMapNativeRoot binding{root, root};
    const auto first = sampleFirstMapV5(123);
    const auto next = sampleFirstMapV5(124);
    const auto uncertain = sampleFirstMapV5(125);
    const auto accept = [](std::span<const std::uint8_t>) {};
    const auto slot = root / "saves/save001.ec6";
    const auto backup = root / "saves/save001.ec6.bak";
    using Outcome = mm6::V5FirstMapSaveOutcome;
    EXPECT_EQ(mm6::writeV5FirstMapNativeCandidateForTest(
        binding, "save001.ec6", first, GameTitle::MM6, accept),
        Outcome::Committed);
    EXPECT_EQ(read(slot), first);
    EXPECT_FALSE(std::filesystem::exists(backup));

    EXPECT_EQ(mm6::writeV5FirstMapNativeCandidateForTest(
        binding, "save001.ec6", next, GameTitle::MM6, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::BeforePrimaryReplace)
                throw std::runtime_error("injected precommit fault");
        }), Outcome::NoNewPrimaryNeedsPreflight);
    EXPECT_EQ(read(slot), first);
    EXPECT_EQ(read(backup), first);

    EXPECT_EQ(mm6::writeV5FirstMapNativeCandidateForTest(
        binding, "save001.ec6", next, GameTitle::MM6, accept),
        Outcome::Committed);
    EXPECT_EQ(read(slot), next);
    EXPECT_EQ(read(backup), first);

    EXPECT_EQ(mm6::writeV5FirstMapNativeCandidateForTest(
        binding, "save001.ec6", uncertain, GameTitle::MM6, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync)
                throw std::runtime_error("injected post-rename fault");
        }), Outcome::IndeterminateNeedsPreflight);
    EXPECT_EQ(read(slot), uncertain);
    EXPECT_EQ(read(backup), next);
}

UNIT_TEST(MM6AtomicSaveFile, NumberedSlotRotatesFirstMapAndAnyMapSaves) {
    // A save on any map goes over a save on the first map in the same numbered slot and back, each keeping the other as
    // the backup, while a slot of another EC6 version is still refused.
    TestProfile profile;
    const auto root = std::filesystem::canonical(profile.root());
    const mm6::V5FirstMapNativeRoot binding{root, root};
    const auto firstMap = sampleFirstMapV5(130);
    const auto dungeon = sampleV7(131);
    const auto accept = [](std::span<const std::uint8_t>) {};
    const auto slot = root / "saves/save001.ec6";
    const auto backup = root / "saves/save001.ec6.bak";
    using Outcome = mm6::V5FirstMapSaveOutcome;
    EXPECT_EQ(mm6::writeNumberedSlotCandidate(binding, "save001.ec6", firstMap, GameTitle::MM6, accept), Outcome::Committed);
    EXPECT_EQ(mm6::writeNumberedSlotCandidate(binding, "save001.ec6", dungeon, GameTitle::MM6, accept), Outcome::Committed);
    EXPECT_EQ(read(slot), dungeon);
    EXPECT_EQ(read(backup), firstMap);
    const auto slots = mm6::listV5FirstMapNativeSlots(binding, GameTitle::MM6, accept);
    ASSERT_EQ(slots.size(), 1);
    EXPECT_EQ(slots[0].header.mapFileName, "d0.blv");
    EXPECT_EQ(slots[0].header.displayName, "V7 131");
    EXPECT_EQ(mm6::readV5FirstMapNativeLoadSlot(binding, "save001.ec6", GameTitle::MM6, accept), dungeon);
    EXPECT_EQ(mm6::writeNumberedSlotCandidate(binding, "save001.ec6", firstMap, GameTitle::MM6, accept), Outcome::Committed);
    EXPECT_EQ(read(slot), firstMap);
    EXPECT_EQ(read(backup), dungeon);

    write(backup, sampleV4(132));
    EXPECT_THROW((void) mm6::writeNumberedSlotCandidate(binding, "save001.ec6", dungeon, GameTitle::MM6, accept), std::exception);
    EXPECT_EQ(read(slot), firstMap);
    std::filesystem::remove(backup);
    std::vector<std::uint8_t> future = dungeon;
    future[8] = 8;
    EXPECT_THROW((void) mm6::writeNumberedSlotCandidate(binding, "save001.ec6", future, GameTitle::MM6, accept), std::exception);
    EXPECT_THROW((void) mm6::decodeSessionSave(sampleV5(133), GameTitle::MM6), std::exception);
    EXPECT_EQ(read(slot), firstMap);
}

UNIT_TEST(MM6AtomicSaveFile, AutosaveAndQuickSaveKeepTheirBackups) {
    // The autosave and a quick save are written atomically and keep the previous save as the backup, as numbered slots
    // do.
    TestProfile profile;
    const auto root = std::filesystem::canonical(profile.root());
    const mm6::V5FirstMapNativeRoot binding{root, root};
    const auto firstMap = sampleFirstMapV5(134);
    const auto dungeon = sampleV7(135);
    const auto accept = [](std::span<const std::uint8_t>) {};
    using Outcome = mm6::V5FirstMapSaveOutcome;
    for (std::string_view name : {"autosave.ec6", "quicksave3.ec6"}) {
        const auto slot = root / "saves" / name;
        const auto backup = root / "saves" / (std::string(name) + ".bak");
        EXPECT_EQ(mm6::writeNumberedSlotCandidate(binding, name, firstMap, GameTitle::MM6, accept), Outcome::Committed);
        EXPECT_EQ(mm6::writeNumberedSlotCandidate(binding, name, dungeon, GameTitle::MM6, accept), Outcome::Committed);
        EXPECT_EQ(read(slot), dungeon);
        EXPECT_EQ(read(backup), firstMap);
    }
    write(root / "saves/autosave.ec6", sampleV4(136));
    EXPECT_THROW((void) mm6::writeNumberedSlotCandidate(binding, "autosave.ec6", dungeon, GameTitle::MM6, accept), std::exception);
    EXPECT_EQ(read(root / "saves/autosave.ec6"), sampleV4(136));
}

UNIT_TEST(MM6AtomicSaveFile, NativeFirstMapSlotListShowsOnlyLoadableProfileSlots) {
    // The MM6 load menu lists the numbered slots, the autosave and the quick saves it can load as they are.
    TestProfile profile;
    const auto root = std::filesystem::canonical(profile.root());
    const mm6::V5FirstMapNativeRoot binding{root, root};
    const auto first = sampleFirstMapV5(126);
    const auto rejectMarker = sampleFirstMapV5(127);
    const auto validate = [&](std::span<const std::uint8_t> bytes) {
        if (std::ranges::equal(bytes, rejectMarker))
            throw std::runtime_error("semantic refusal");
    };
    EXPECT_TRUE(mm6::listV5FirstMapNativeSlots(binding, GameTitle::MM6, validate).empty());

    std::filesystem::create_directory(root / "saves");
    write(root / "saves/save000.ec6", first);
    write(root / "saves/save001.mm7", first);
    write(root / "saves/save002.ec6", {1, 2, 3});
    write(root / "saves/save003.ec6", sampleV5(128));
    write(root / "saves/save004.ec6.bak", first);
    write(root / "saves/save005.ec6", rejectMarker);
    write(root / "saves/autosave.ec6", first);
    write(root / "saves/quicksave2.ec6", first);
    write(root / "saves/quicksave.ec6", first);
    write(root / "saves/autosave.mm7", first);
    write(root / "saves/save006.ec6", first);
    write(root / "saves/save006.ec6.bak", {1, 2, 3});

    const auto slots = mm6::listV5FirstMapNativeSlots(binding, GameTitle::MM6, validate);
    ASSERT_EQ(slots.size(), 3);
    EXPECT_EQ(slots[0].fileName, "autosave.ec6");
    EXPECT_EQ(slots[1].fileName, "quicksave2.ec6");
    EXPECT_EQ(slots[2].fileName, "save000.ec6");
    EXPECT_EQ(slots[2].header.mapFileName, "oute3.odm");
    EXPECT_FALSE(slots[2].needsRecovery);
    EXPECT_EQ(mm6::readV5FirstMapNativeLoadSlot(binding, "save000.ec6", GameTitle::MM6, validate), first);
    EXPECT_EQ(mm6::readV5FirstMapNativeLoadSlot(binding, "autosave.ec6", GameTitle::MM6, validate), first);
    for (std::string_view refused : {"save002.ec6", "save003.ec6", "save004.ec6", "save005.ec6", "save006.ec6",
                                     "save007.ec6", "quicksave.ec6", "autosave.mm7", "save001.mm7"})
        EXPECT_THROW((void) mm6::readV5FirstMapNativeLoadSlot(binding, refused, GameTitle::MM6, validate),
                     std::exception) << refused;
    EXPECT_THROW((void) mm6::listV5FirstMapNativeSlots(binding, GameTitle::MM7, validate), std::exception);
    EXPECT_THROW((void) mm6::listV5FirstMapNativeSlots({root, root / "saves"}, GameTitle::MM6, validate),
                 std::exception);
    EXPECT_EQ(read(root / "saves/save000.ec6"), first);
    EXPECT_EQ(read(root / "saves/save002.ec6"), (std::vector<std::uint8_t>{1, 2, 3}));
    EXPECT_EQ(read(root / "saves/save004.ec6.bak"), first);
    EXPECT_FALSE(std::filesystem::exists(root / "saves/save004.ec6"));
}

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

UNIT_TEST(MM6AtomicSaveFile, V3SemanticPreflightAndListingStayReadOnly) {
    TestProfile profile;
    const auto first = sampleV3(41);
    const auto second = sampleV3(42);
    const auto excluded = sampleV3(43);
    const mm6::SaveCandidateValidator acceptAll = [](std::span<const std::uint8_t>) {};
    mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", first, acceptAll);
    mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", second, acceptAll);
    mm6::writeAtomicV3SaveFile(profile.root(), "save002.ec6", excluded, acceptAll);
    EXPECT_THROW((void) mm6::preflightRecoverableV3SaveFile(
        profile.root(), "save001.ec6", {}), std::exception);
    EXPECT_THROW((void) mm6::listV3SaveSlotsWithPreflight(profile.root(), {}),
                 std::exception);
    const mm6::SaveCandidateValidator acceptOnlyFirst = [](std::span<const std::uint8_t> bytes) {
        if (mm6::decodeV3Save(bytes, GameTitle::MM6).raw.metadata.header.displayName != "V3 41")
            throw std::runtime_error("synthetic semantic refusal");
    };
    const auto selected = mm6::preflightRecoverableV3SaveFile(
        profile.root(), "save001.ec6", acceptOnlyFirst);
    ASSERT_TRUE(selected);
    EXPECT_EQ(selected->bytes, first);
    EXPECT_TRUE(selected->needsRecovery);
    const auto slots = mm6::listV3SaveSlotsWithPreflight(profile.root(), acceptOnlyFirst);
    ASSERT_EQ(slots.size(), 1u);
    EXPECT_EQ(slots[0].fileName, "save001.ec6");
    EXPECT_EQ(slots[0].header.displayName, "V3 41");
    EXPECT_TRUE(slots[0].needsRecovery);
    EXPECT_EQ(read(profile.save()), second);
    EXPECT_EQ(read(profile.backup()), first);
    EXPECT_EQ(read(profile.root() / "saves/save002.ec6"), excluded);
    EXPECT_TRUE(mm6::listRawSaveSlotsWithPreflight(profile.root(), acceptAll).empty());

    auto damaged = second;
    damaged.back() ^= 1; // Same v3 header, invalid section checksum.
    write(profile.save(), damaged);
    const auto recoveredCandidate = mm6::preflightRecoverableV3SaveFile(
        profile.root(), "save001.ec6", acceptAll);
    ASSERT_TRUE(recoveredCandidate);
    EXPECT_EQ(recoveredCandidate->bytes, first);
    EXPECT_TRUE(recoveredCandidate->needsRecovery);
    EXPECT_EQ(read(profile.save()), damaged); // No automatic repair during preflight.
}

UNIT_TEST(MM6AtomicSaveFile, V3ForeignVersionsRefuseBeforeDiskMutation) {
    const mm6::SaveCandidateValidator accept = [](std::span<const std::uint8_t>) {};
    const auto v3 = sampleV3(51);
    const auto v2 = sampleRaw(52);
    const auto v1 = sample(53);
    for (const auto &foreign : {v1, v2}) {
        TestProfile profile;
        std::filesystem::create_directories(profile.root() / "saves");
        write(profile.save(), foreign);
        EXPECT_THROW(mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", v3, accept),
                     std::exception);
        EXPECT_THROW((void) mm6::preflightRecoverableV3SaveFile(
            profile.root(), "save001.ec6", accept), std::exception);
        EXPECT_EQ(read(profile.save()), foreign);
    }
    TestProfile mixed;
    mm6::writeAtomicV3SaveFile(mixed.root(), "save001.ec6", v3, accept);
    write(mixed.backup(), v2);
    EXPECT_THROW(mm6::writeAtomicV3SaveFile(mixed.root(), "save001.ec6", sampleV3(54), accept),
                 std::exception);
    EXPECT_THROW((void) mm6::preflightRecoverableV3SaveFile(
        mixed.root(), "save001.ec6", accept), std::exception);
    EXPECT_EQ(read(mixed.save()), v3);
    EXPECT_EQ(read(mixed.backup()), v2);
}

UNIT_TEST(MM6AtomicSaveFile, V3FaultLeavesPriorSlotAndRejectsOldWriter) {
    TestProfile profile;
    const mm6::SaveCandidateValidator accept = [](std::span<const std::uint8_t>) {};
    const auto first = sampleV3(61);
    const auto second = sampleV3(62);
    mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", first, accept);
    EXPECT_THROW(mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", second, {}),
                 std::exception);
    EXPECT_THROW(mm6::writeAtomicV3SaveFile(profile.root(), "save001.ec6", second,
        [](std::span<const std::uint8_t>) {
            throw std::runtime_error("synthetic semantic refusal");
        }), std::exception);
    EXPECT_EQ(read(profile.save()), first);
    EXPECT_THROW(mm6::writeAtomicRawSaveFile(profile.root(), "save001.ec6", sampleRaw(63)),
                 std::exception);
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", sample(64)),
                 std::exception);
    EXPECT_THROW(mm6::detail::writeAtomicV3SaveFile(
        profile.root(), "save001.ec6", second, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::BeforePrimaryReplace)
                throw std::runtime_error("injected v3 write failure");
        }), std::exception);
    EXPECT_EQ(read(profile.save()), first);
    EXPECT_EQ(read(profile.backup()), first);
}

UNIT_TEST(MM6AtomicSaveFile, V4TestWriterRequiresDisposablePathAndSemanticCallback) {
    TestProfile profile("mm6-restart-");
    const auto candidate = sampleV4(70);
    const auto slot = profile.root() / "saves/candidate-v4.ec6";
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV4Save(bytes, GameTitle::MM6);
    };
    EXPECT_THROW(mm6::writeAtomicV4SaveFileForTest(
        profile.root(), "candidate-v4.ec6", candidate, {}), std::exception);
    EXPECT_THROW(mm6::writeAtomicV4SaveFileForTest(
        profile.root(), "autosave.ec6", candidate, accept), std::exception);
    EXPECT_THROW(mm6::writeAtomicV4SaveFileForTest(
        profile.root(), "candidate-v4.ec6", candidate,
        [](std::span<const std::uint8_t>) { throw std::runtime_error("semantic refusal"); }),
        std::exception);
    EXPECT_FALSE(std::filesystem::exists(slot));
    TestProfile ordinary;
    EXPECT_THROW(mm6::writeAtomicV4SaveFileForTest(
        ordinary.root(), "candidate-v4.ec6", candidate, accept), std::exception);
    mm6::writeAtomicV4SaveFileForTest(profile.root(), "candidate-v4.ec6", candidate, accept);
    EXPECT_EQ(read(slot), candidate);
    const auto replacement = sampleV4(76);
    mm6::writeAtomicV4SaveFileForTest(profile.root(), "candidate-v4.ec6", replacement, accept);
    EXPECT_EQ(read(slot), replacement);
    EXPECT_EQ(read(slot.string() + ".bak"), candidate);
    const auto afterAction = profile.root() / "saves/candidate-v4-after-action.ec6";
    mm6::writeAtomicV4SaveFileForTest(
        profile.root(), "candidate-v4-after-action.ec6", candidate, accept);
    EXPECT_EQ(read(afterAction), candidate);
    EXPECT_EQ(read(slot), replacement); // Separate test slots cannot overwrite each other.
}

UNIT_TEST(MM6AtomicSaveFile, V4ForeignPrimaryAndBackupRefuseBeforeMutation) {
    const auto candidate = sampleV4(71);
    const auto foreign = sampleRaw(72);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV4Save(bytes, GameTitle::MM6);
    };
    for (bool foreignBackup : {false, true}) {
        TestProfile profile("mm6-restart-");
        const auto slot = profile.root() / "saves/candidate-v4.ec6";
        std::filesystem::create_directories(slot.parent_path());
        if (foreignBackup) {
            write(slot, candidate);
            write(slot.string() + ".bak", foreign);
        } else {
            write(slot, foreign);
        }
        const auto oldPrimary = read(slot);
        const auto oldBackup = read(slot.string() + ".bak");
        EXPECT_THROW(mm6::writeAtomicV4SaveFileForTest(
            profile.root(), "candidate-v4.ec6", candidate, accept), std::exception);
        EXPECT_EQ(read(slot), oldPrimary);
        EXPECT_EQ(read(slot.string() + ".bak"), oldBackup);
    }
}

UNIT_TEST(MM6AtomicSaveFile, V4FaultPreservesCandidateAndV2Seed) {
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV4Save(bytes, GameTitle::MM6);
    };
    for (mm6::detail::AtomicSaveStep step : {
             mm6::detail::AtomicSaveStep::BeforeTempWrite,
             mm6::detail::AtomicSaveStep::AfterPartialTempWrite,
             mm6::detail::AtomicSaveStep::BeforeBackupReplace,
             mm6::detail::AtomicSaveStep::BeforePrimaryReplace}) {
        TestProfile profile("mm6-restart-");
        const auto slot = profile.root() / "saves/candidate-v4.ec6";
        const auto seed = sampleRaw(73);
        const auto first = sampleV4(74);
        const auto next = sampleV4(75);
        std::filesystem::create_directories(slot.parent_path());
        write(profile.root() / "saves/autosave.ec6", seed);
        mm6::writeAtomicV4SaveFileForTest(profile.root(), "candidate-v4.ec6", first, accept);
        EXPECT_THROW(mm6::detail::writeAtomicV4SaveFileForTest(
            profile.root(), "candidate-v4.ec6", next, accept,
            [step](mm6::detail::AtomicSaveStep reached) {
                if (reached == step)
                    throw std::runtime_error("injected v4 write failure");
            }), std::exception);
        EXPECT_EQ(read(slot), first);
        EXPECT_EQ(read(profile.root() / "saves/autosave.ec6"), seed);
        if (std::filesystem::exists(slot.string() + ".bak"))
            EXPECT_EQ(read(slot.string() + ".bak"), first);
    }
}

UNIT_TEST(MM6AtomicSaveFile, V5AfterActionSlotKeepsOtherVersionsAndBackup) {
    TestProfile profile("mm6-restart-");
    const auto slot = profile.root() / "saves/candidate-v5-after-action.ec6";
    const auto seed = sampleRaw(80);
    const auto priorV4 = sampleV4(81);
    const auto first = sampleV5(82);
    const auto second = sampleV5(83);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    std::filesystem::create_directories(slot.parent_path());
    write(profile.root() / "saves/autosave.ec6", seed);
    write(profile.root() / "saves/candidate-v4.ec6", priorV4);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v4.ec6", first, accept), std::exception);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", first, {}), std::exception);
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", first, accept);
    EXPECT_EQ(read(slot), first);
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", second, accept);
    EXPECT_EQ(read(slot), second);
    EXPECT_EQ(read(slot.string() + ".bak"), first);
    EXPECT_EQ(read(profile.root() / "saves/autosave.ec6"), seed);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v4.ec6"), priorV4);
    write(slot.string() + ".bak", priorV4);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", first, accept), std::exception);
    EXPECT_EQ(read(slot), second);
    EXPECT_EQ(read(slot.string() + ".bak"), priorV4);
}

UNIT_TEST(MM6AtomicSaveFile, V5FaultPreservesPreviousCandidate) {
    TestProfile profile("mm6-restart-");
    const auto first = sampleV5(84);
    const auto second = sampleV5(85);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", first, accept);
    EXPECT_THROW(mm6::detail::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", second, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::BeforePrimaryReplace)
                throw std::runtime_error("v5 injected before replacement");
        }), std::exception);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-after-action.ec6"), first);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-after-action.ec6.bak"), first);
}

UNIT_TEST(MM6AtomicSaveFile, V5FirstMapResultPolicyNeverConfusesFailedRenameWithSuccess) {
    const auto first = sampleFirstMapV5(101);
    const auto next = sampleFirstMapV5(102);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    mm6::V5FirstMapSlotState before;
    before.primary = first;
    mm6::V5FirstMapSlotState noNew = before;
    noNew.backup = first;
    mm6::V5FirstMapSlotState newPrimary;
    newPrimary.primary = next;
    newPrimary.backup = first;
    using Result = mm6::V5FirstMapWriteResult;
    EXPECT_EQ(mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "candidate-v5-after-action.ec6", next,
        before, noNew, false, accept), Result::NoNewPrimaryNeedsPreflight);
    EXPECT_EQ(mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "candidate-v5-after-action.ec6", next,
        before, newPrimary, false, accept),
        Result::NewPrimaryUnconfirmedNeedsPreflight);
    EXPECT_EQ(mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "candidate-v5-after-action.ec6", next,
        before, newPrimary, true, accept), Result::Committed);
    EXPECT_THROW((void) mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM7, "candidate-v5-after-action.ec6", next,
        before, newPrimary, true, accept), std::exception);
    EXPECT_THROW((void) mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "autosave.ec6", next,
        before, newPrimary, true, accept), std::exception);
    EXPECT_THROW((void) mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "candidate-v5-after-action.ec6", sampleV5(103),
        before, newPrimary, true, accept), std::exception);
    newPrimary.backup = next;
    EXPECT_THROW((void) mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, "candidate-v5-after-action.ec6", next,
        before, newPrimary, true, accept), std::exception);
}

UNIT_TEST(MM6AtomicSaveFile, V5FirstMapPostRenameFaultNeedsReadOnlyPreflight) {
    TestProfile profile("mm6-restart-");
    constexpr std::string_view slotName = "candidate-v5-after-action.ec6";
    const auto slot = profile.root() / "saves" / slotName;
    const auto first = sampleFirstMapV5(104);
    const auto next = sampleFirstMapV5(105);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    mm6::writeAtomicV5SaveFileForTest(profile.root(), slotName, first, accept);
    mm6::V5FirstMapSlotState before;
    before.primary = read(slot);
    EXPECT_THROW(mm6::detail::writeAtomicV5SaveFileForTest(
        profile.root(), slotName, next, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync)
                throw std::runtime_error("injected post-rename sync uncertainty");
        }), std::exception);
    mm6::V5FirstMapSlotState after;
    after.primary = read(slot);
    after.backup = read(slot.string() + ".bak");
    EXPECT_EQ(mm6::assessV5FirstMapWriteResultForTest(
        GameTitle::MM6, slotName, next, before, after, false, accept),
        mm6::V5FirstMapWriteResult::NewPrimaryUnconfirmedNeedsPreflight);
    const auto selected = mm6::preflightRecoverableV5FirstMapSaveFileForTest(
        profile.root(), slotName, GameTitle::MM6, accept);
    ASSERT_TRUE(selected);
    EXPECT_FALSE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, next);
    EXPECT_EQ(read(slot), next);
    EXPECT_EQ(read(slot.string() + ".bak"), first);

    const std::vector<std::uint8_t> damaged{1, 2, 3};
    write(slot, damaged);
    const auto backupSelected = mm6::preflightRecoverableV5FirstMapSaveFileForTest(
        profile.root(), slotName, GameTitle::MM6, accept);
    ASSERT_TRUE(backupSelected);
    EXPECT_TRUE(backupSelected->needsRecovery);
    EXPECT_EQ(backupSelected->bytes, first);
    EXPECT_EQ(read(slot), damaged);
    EXPECT_EQ(read(slot.string() + ".bak"), first);

    write(slot.string() + ".bak", sampleV5(106));
    EXPECT_THROW((void) mm6::preflightRecoverableV5FirstMapSaveFileForTest(
        profile.root(), slotName, GameTitle::MM6, accept), std::exception);
    EXPECT_EQ(read(slot), damaged);
}

UNIT_TEST(MM6AtomicSaveFile, V5ReloadInputSlotIsSeparateAndAtomic) {
    TestProfile profile("mm6-restart-");
    const auto seed = sampleRaw(86);
    const auto priorV4 = sampleV4(87);
    const auto priorV5 = sampleV5(88);
    const auto first = sampleV5(89);
    const auto second = sampleV5(90);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    std::filesystem::create_directories(profile.root() / "saves");
    write(profile.root() / "saves/autosave.ec6", seed);
    write(profile.root() / "saves/candidate-v4.ec6", priorV4);
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-action.ec6", priorV5, accept);
    const auto slot = profile.root() / "saves/candidate-v5-after-reload-input.ec6";
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-reload-input.ec6", first, accept);
    EXPECT_EQ(read(slot), first);
    EXPECT_THROW(mm6::detail::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-reload-input.ec6", second, accept,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::BeforePrimaryReplace)
                throw std::runtime_error("v5 second-slot injected before replacement");
        }), std::exception);
    EXPECT_EQ(read(slot), first);
    EXPECT_EQ(read(slot.string() + ".bak"), first);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-after-action.ec6"), priorV5);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v4.ec6"), priorV4);
    EXPECT_EQ(read(profile.root() / "saves/autosave.ec6"), seed);
    write(slot.string() + ".bak", priorV4);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-after-reload-input.ec6", second, accept),
        std::exception);
    EXPECT_EQ(read(slot), first);
    EXPECT_EQ(read(slot.string() + ".bak"), priorV4);
}

UNIT_TEST(MM6AtomicSaveFile, V5LaterOutdoorSlotIsSeparateAndRejectsForeignSlots) {
    TestProfile profile("mm6-restart-");
    const auto seed = sampleRaw(91);
    const auto priorV4 = sampleV4(92);
    const auto later = sampleV5(93);
    const auto accept = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    std::filesystem::create_directories(profile.root() / "saves");
    write(profile.root() / "saves/autosave.ec6", seed);
    write(profile.root() / "saves/candidate-v4.ec6", priorV4);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "autosave.ec6", later, accept), std::exception);
    EXPECT_THROW(mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-oute3.ec6", later, accept), std::exception);
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-oute2.ec6", later, accept);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-oute2.ec6"), later);
    mm6::writeAtomicV5SaveFileForTest(
        profile.root(), "candidate-v5-oute2-after-input.ec6", later, accept);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-oute2-after-input.ec6"), later);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v5-oute2.ec6"), later);
    EXPECT_EQ(read(profile.root() / "saves/autosave.ec6"), seed);
    EXPECT_EQ(read(profile.root() / "saves/candidate-v4.ec6"), priorV4);
}

UNIT_TEST(MM6AtomicSaveFile, FirstMapWriterClassifiesAtomicOutcomes) {
    TestProfile profile("mm6-restart-");
    constexpr std::string_view slotName = "candidate-v5-after-action.ec6";
    const auto slot = profile.root() / "saves" / slotName;
    const auto first = sampleFirstMapV5(111);
    const auto next = sampleFirstMapV5(112);
    const auto uncertain = sampleFirstMapV5(113);
    const auto validate = [](std::span<const std::uint8_t> bytes) {
        (void) mm6::decodeV5Save(bytes, GameTitle::MM6);
    };
    using Outcome = mm6::V5FirstMapSaveOutcome;

    EXPECT_EQ(mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, first, GameTitle::MM6, validate, {}),
        Outcome::Committed);
    EXPECT_EQ(read(slot), first);
    const auto backup = slot.string() + ".bak";
    EXPECT_FALSE(std::filesystem::exists(backup));

    EXPECT_EQ(mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, next, GameTitle::MM6, validate,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::BeforeTempWrite)
                throw std::runtime_error("injected precommit refusal");
        }), Outcome::NoNewPrimaryNeedsPreflight);
    EXPECT_EQ(read(slot), first);
    EXPECT_FALSE(std::filesystem::exists(backup));

    EXPECT_EQ(mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, next, GameTitle::MM6, validate, {}),
        Outcome::Committed);
    EXPECT_EQ(read(slot), next);
    EXPECT_EQ(read(backup), first);

    EXPECT_EQ(mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, uncertain, GameTitle::MM6, validate,
        [](mm6::detail::AtomicSaveStep step) {
            if (step == mm6::detail::AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync)
                throw std::runtime_error("injected post-rename uncertainty");
        }), Outcome::IndeterminateNeedsPreflight);
    EXPECT_EQ(read(slot), uncertain);
    EXPECT_EQ(read(backup), next);
    EXPECT_THROW((void) mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, next, GameTitle::MM7, validate, {}),
        std::exception);
    EXPECT_THROW((void) mm6::detail::writeV5FirstMapCandidateForTest(
        profile.root(), slotName, sampleV5(114), GameTitle::MM6,
        validate, {}), std::exception);
    EXPECT_EQ(read(slot), uncertain);
    EXPECT_EQ(read(backup), next);
}

#endif
