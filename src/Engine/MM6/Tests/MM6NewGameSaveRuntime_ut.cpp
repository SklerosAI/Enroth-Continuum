#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6NewGameSaveRuntime.h"
#include "Engine/MM6/MM6FirstMapWorldTransaction.h"
#include "Engine/MM6/MM6FirstMapPlacementTransaction.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"

namespace {

std::vector<std::string> expectedNames() {
    std::vector<std::string> names;
    for (int i = 0; i < 52; ++i)
        names.push_back("d" + std::to_string(i) + ".dlv");
    names.push_back("oute3.ddm");
    for (int i = 1; i < 15; ++i)
        names.push_back("o" + std::to_string(i) + ".ddm");
    return names;
}

mm6::KnownSaveFields seed(std::int64_t ticks) {
    mm6::KnownSaveFields fields;
    fields.core.party.playingTicks = ticks;
    fields.core.party.activeCharacter = 1;
    fields.core.party.position = {-10178, -11206, 340};
    fields.core.party.fallStartZ = 340;
    for (mm6::CoreCharacterFields &character : fields.core.characters)
        character.name = "Hero";
    fields.metadata.header.displayName = "New game";
    fields.metadata.header.mapFileName = std::string(mm6::NEW_GAME_FIRST_MAP);
    fields.metadata.header.playingTicks = ticks;
    fields.metadata.ui.activeCharacter = 1;
    fields.recovery.playingTicks = ticks;
    fields.chest = {17, {}, "oute3.ddm", {1}}; // Synthetic seed; builder replaces the sidecar.
    fields.shopGuildClocks.ready = true;
    return fields;
}

std::vector<std::uint8_t> defaultDelta(std::string_view name) {
    return name.ends_with(".dlv")
        ? std::vector<std::uint8_t>(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, 0)
        : std::vector<std::uint8_t>(8 + 2 * 88 * 11 + 3 * 4 + 200 + 56, 0);
}

mm6::DefaultDeltaReader defaultReader() {
    return [](std::string_view name) {
        const std::vector<std::uint8_t> bytes = defaultDelta(name);
        return Blob::copy(bytes.data(), bytes.size());
    };
}

mm6::MapBaseReader baseReader() {
    return [](std::string_view name) {
        std::vector<std::uint8_t> bytes = name.ends_with(".blv")
            ? std::vector<std::uint8_t>(136 + 10 * 4, 0)
            : std::vector<std::uint8_t>(160 + 16 + 3 * 128 * 128 + 4 + 4 + 4 + 128 * 128 * 4 + 4, 0);
        if (name.ends_with(".blv"))
            bytes[136 + 4 * 4] = 200;
        return Blob::copy(bytes.data(), bytes.size());
    };
}

#ifndef _WIN32
class TestProfile {
 public:
    TestProfile() {
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-newgame-XXXXXX").string();
        char *result = ::mkdtemp(pattern.data());
        if (!result)
            throw std::runtime_error("cannot create MM6 new-game test profile");
        _root = result;
    }

    ~TestProfile() {
        std::error_code error;
        std::filesystem::remove_all(_root, error);
    }

    const std::filesystem::path &root() const { return _root; }
    std::filesystem::path primary() const { return _root / "saves" / "autosave.ec6"; }

 private:
    std::filesystem::path _root;
};

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

UNIT_TEST(MM6NewGameSaveRuntime, WritesAndPreflightsOnlyMM6FirstMapAutosave) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    int deltaReads = 0;
    auto readDefault = [&](std::string_view name) {
        ++deltaReads;
        return defaultReader()(name);
    };
    std::vector<std::uint8_t> bytes = mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(42), names, readDefault, baseReader(),
        [](std::int32_t id) { return id >= 1 && id <= 580; }, GameTitle::MM6);
    EXPECT_EQ(deltaReads, 67);
    EXPECT_EQ(readFile(profile.primary()), bytes);
    EXPECT_EQ(mm6::decodeKnownSave(bytes).metadata.header.mapFileName, "oute3.odm");

    auto selected = mm6::preflightRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT,
        [&](std::span<const std::uint8_t> candidate) {
            (void) mm6::decodeKnownSaveWithActiveMapCounts(candidate, names, baseReader(),
                [](std::int32_t id) { return id >= 1 && id <= 580; });
        });
    ASSERT_TRUE(selected);
    EXPECT_FALSE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, bytes);

    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(43), names, readDefault, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM7), std::exception);
    mm6::KnownSaveFields wrongMap = seed(43);
    wrongMap.metadata.header.mapFileName = "d0.blv";
    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), wrongMap, names, readDefault, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6), std::exception);
    mm6::KnownSaveFields wrongFall = seed(43);
    wrongFall.core.party.fallStartZ = 0;
    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), wrongFall, names, readDefault, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6), std::exception);
    EXPECT_EQ(deltaReads, 67);
    EXPECT_EQ(readFile(profile.primary()), bytes);
}

UNIT_TEST(MM6NewGameSaveRuntime, StagesFreshOwnersBeforeWritingFirstMap) {
    TestProfile profile;
    const auto names = expectedNames();
    mm6::KnownSaveFields source = seed(42);
    source.core.party.gold = 729;
    source.npcs.activeCount = 1;
    source.npcs.npcData[0].name = "New Sorpigal NPC";
    Party party;
    party.uNumGold = 18;
    NPCStats npcs{};
    npcs.pNPCData[0].name = "Prior NPC";
    Timer timer;
    ActiveOverlayList overlays;
    int seedCalls = 0;
    const mm6::ChestSeedEntropy entropy = [&] {
        ++seedCalls;
        return std::optional<std::uint32_t>(0x1234abcd);
    };
    const mm6::ChestTitleState priorTitleChest = mm6::chestTitleState();

    auto prepared = mm6::writeAndStageOfflineNewGameAutosave(
        profile.root(), source, party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; },
        5, entropy, GameTitle::MM6);
    EXPECT_EQ(seedCalls, 1);
    EXPECT_EQ(readFile(profile.primary()), prepared.bytes);
    EXPECT_EQ(prepared.staged.party.uNumGold, 729);
    EXPECT_EQ(prepared.staged.worldAndVisuals.world.npcs.pNPCData[0].name, "New Sorpigal NPC");
    EXPECT_EQ(prepared.staged.worldAndVisuals.world.mapDeltas.size(), 67);
    EXPECT_EQ(prepared.staged.worldAndVisuals.chestTitle.lcgState, 0x1234abcdu);
    EXPECT_TRUE(prepared.staged.worldAndVisuals.chestTitle.ready);
    EXPECT_EQ(prepared.staged.worldAndVisuals.rawActiveDelta, defaultDelta("oute3.ddm"));
    EXPECT_TRUE(prepared.staged.shopGuildClocks.ready);
    EXPECT_EQ(party.uNumGold, 18);
    EXPECT_EQ(npcs.pNPCData[0].name, "Prior NPC");
    EXPECT_EQ(mm6::chestTitleState().ready, priorTitleChest.ready);
    EXPECT_EQ(mm6::chestTitleState().lcgState, priorTitleChest.lcgState);
}

UNIT_TEST(MM6NewGameSaveRuntime, LateOwnerFailurePreservesAutosaveAndProcessOwners) {
    TestProfile profile;
    const auto names = expectedNames();
    Party party;
    party.uNumGold = 18;
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    const mm6::ChestSeedEntropy entropy = [] {
        return std::optional<std::uint32_t>(0x98765432);
    };
    auto write = [&](const mm6::KnownSaveFields &source) {
        return mm6::writeAndStageOfflineNewGameAutosave(
            profile.root(), source, party, npcs, timer, overlays, names,
            defaultReader(), baseReader(), [](std::int32_t) { return true; },
            5, entropy, GameTitle::MM6);
    };
    auto prior = write(seed(42));
    party.bTurnBasedModeOn = true; // Final Party adapter rejects after all map reads.
    EXPECT_THROW((void) write(seed(43)), std::exception);
    EXPECT_EQ(readFile(profile.primary()), prior.bytes);
    EXPECT_EQ(party.uNumGold, 18);
    EXPECT_TRUE(party.bTurnBasedModeOn);

    party.bTurnBasedModeOn = false;
    mm6::KnownSaveFields staleShop = seed(43);
    staleShop.shopGuildClocks.deadlineWords[0] = 1;
    int seedCalls = 0;
    EXPECT_THROW((void) mm6::prepareOfflineNewGameSession(
        staleShop, party, npcs, timer, overlays, names, defaultReader(), baseReader(),
        [](std::int32_t) { return true; }, 5,
        [&] { ++seedCalls; return std::optional<std::uint32_t>(1); }, GameTitle::MM6),
        std::exception);
    EXPECT_EQ(seedCalls, 0);
    EXPECT_THROW((void) mm6::prepareOfflineNewGameSession(
        seed(43), party, npcs, timer, overlays, names, defaultReader(), baseReader(),
        [](std::int32_t) { return true; }, 5,
        [&] { ++seedCalls; return std::optional<std::uint32_t>(1); }, GameTitle::MM7),
        std::exception);
    EXPECT_EQ(seedCalls, 0);
    EXPECT_EQ(readFile(profile.primary()), prior.bytes);
}

UNIT_TEST(MM6NewGameSaveRuntime, CapturesAndPublishesFirstMapCacheOnlyAfterWrite) {
    TestProfile profile;
    const auto names = expectedNames();
    Party party;
    party.uNumGold = 457;
    party.pos = Vec3f(8, 9, 10);
    for (Character &character : party.pCharacters)
        character.name = "Hero";
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    mm6::KnownSaveFields source = mm6::captureFirstMapKnownSeed(
        party, npcs, timer, overlays, "Fresh MM6", 5, GameTitle::MM6);
    EXPECT_EQ(source.core.party.position[0], -10178);
    EXPECT_EQ(source.core.party.position[1], -11206);
    EXPECT_EQ(source.core.party.position[2], 340);
    EXPECT_EQ(source.core.party.gold, 457);
    EXPECT_EQ(party.pos.x, 8);
    EXPECT_EQ(source.metadata.header.mapFileName, "oute3.odm");

    std::unordered_map<std::string, Blob> cache;
    cache.emplace("old.ddm", Blob::fromString("old map"));
    mm6::ChestTitleState titleChest{true, 71, {}};
    const mm6::ChestSeedEntropy entropy = [] {
        return std::optional<std::uint32_t>(0xfeedbeef);
    };
    EXPECT_THROW((void) mm6::writeAndPublishOfflineNewGameAutosave(
        "relative-profile", source, party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; },
        5, entropy, &cache, &titleChest, GameTitle::MM6), std::exception);
    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.at("old.ddm").str(), "old map");
    EXPECT_EQ(titleChest.lcgState, 71u);

    auto published = mm6::writeAndPublishOfflineNewGameAutosave(
        profile.root(), source, party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; },
        5, entropy, &cache, &titleChest, GameTitle::MM6);
    EXPECT_EQ(readFile(profile.primary()), published->prepared.bytes);
    EXPECT_EQ(cache.size(), 67);
    EXPECT_TRUE(cache.contains("oute3.ddm"));
    EXPECT_EQ(published->priorMapDeltas.at("old.ddm").str(), "old map");
    EXPECT_EQ(published->priorChest.lcgState, 71u);
    EXPECT_TRUE(titleChest.ready);
    EXPECT_EQ(titleChest.lcgState, 0xfeedbeefu);
    EXPECT_EQ(party.pos.x, 8);
    EXPECT_EQ(party.uNumGold, 457);
    EXPECT_FALSE(mm6::rollbackPublishedFirstMapCache(
        published.get(), &cache, &titleChest, GameTitle::MM7));
    EXPECT_EQ(cache.size(), 67);
    EXPECT_TRUE(mm6::rollbackPublishedFirstMapCache(
        published.get(), &cache, &titleChest, GameTitle::MM6));
    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.at("old.ddm").str(), "old map");
    EXPECT_EQ(titleChest.lcgState, 71u);
    EXPECT_FALSE(mm6::rollbackPublishedFirstMapCache(
        published.get(), &cache, &titleChest, GameTitle::MM6));
}

UNIT_TEST(MM6NewGameSaveRuntime, ScopedEntryPrecommitFailureNeverPublishesSlotOrOwners) {
    TestProfile profile;
    const auto names = expectedNames();
    Party party;
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    std::unordered_map<std::string, Blob> cache;
    cache.emplace("prior.ddm", Blob::fromString("prior cache"));
    mm6::ChestTitleState chest{true, 75, {}};
    bool observed = false;
    int seedCalls = 0;
    mm6::KnownSaveFields stale = seed(51);
    stale.shopGuildClocks.deadlineWords[0] = 1;
    EXPECT_THROW((void) mm6::runScopedFirstMapEntryForTest(
        profile.root(), stale, party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; }, 5,
        [&] { ++seedCalls; return std::optional<std::uint32_t>(1); }, &cache, &chest,
        [](const Party &, const std::unordered_map<std::string, Blob> &,
           const mm6::ChestTitleState &) {}, GameTitle::MM6), std::exception);
    EXPECT_EQ(seedCalls, 0);
    EXPECT_FALSE(std::filesystem::exists(profile.primary()));
    EXPECT_THROW((void) mm6::runScopedFirstMapEntryForTest(
        profile.root(), seed(51), party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; }, 5,
        [&] { ++seedCalls; return std::optional<std::uint32_t>(0xabc12345); }, &cache, &chest,
        [&](const Party &stagedParty, const std::unordered_map<std::string, Blob> &published,
            const mm6::ChestTitleState &publishedChest) {
            observed = true;
            EXPECT_EQ(stagedParty.pos.z, 340);
            EXPECT_EQ(published.size(), 67);
            EXPECT_EQ(publishedChest.lcgState, 0xabc12345u);
            EXPECT_FALSE(std::filesystem::exists(profile.primary()));
            EXPECT_EQ(cache.size(), 1);
            EXPECT_EQ(cache.at("prior.ddm").str(), "prior cache");
            EXPECT_EQ(chest.lcgState, 75u);
            throw std::runtime_error("synthetic map entry failure");
        }, GameTitle::MM6), std::exception);
    EXPECT_TRUE(observed);
    EXPECT_EQ(seedCalls, 1);
    EXPECT_FALSE(std::filesystem::exists(profile.primary()));
    EXPECT_FALSE(std::filesystem::exists(profile.primary().string() + ".bak"));
    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.at("prior.ddm").str(), "prior cache");
    EXPECT_EQ(chest.lcgState, 75u);
}

UNIT_TEST(MM6NewGameSaveRuntime, ScopedEntryPrecommitFailurePreservesPriorPrimaryAndBackup) {
    TestProfile profile;
    const auto names = expectedNames();
    const auto write = [&](std::int64_t ticks) {
        return mm6::writeOfflineNewGameAutosave(profile.root(), seed(ticks), names,
            defaultReader(), baseReader(), [](std::int32_t) { return true; }, GameTitle::MM6);
    };
    const auto oldBackup = write(51);
    const auto oldPrimary = write(52);
    const auto backupPath = std::filesystem::path(profile.primary().string() + ".bak");
    EXPECT_EQ(readFile(backupPath), oldBackup);
    Party party;
    NPCStats npcs{};
    Timer timer;
    ActiveOverlayList overlays;
    std::unordered_map<std::string, Blob> cache;
    cache.emplace("prior.ddm", Blob::fromString("prior cache"));
    mm6::ChestTitleState chest{true, 75, {}};
    EXPECT_THROW((void) mm6::runScopedFirstMapEntryForTest(
        profile.root(), seed(53), party, npcs, timer, overlays, names,
        defaultReader(), baseReader(), [](std::int32_t) { return true; }, 5,
        [] { return std::optional<std::uint32_t>(0xabc12345); }, &cache, &chest,
        [&](const Party &, const std::unordered_map<std::string, Blob> &,
            const mm6::ChestTitleState &) {
            EXPECT_EQ(readFile(profile.primary()), oldPrimary);
            EXPECT_EQ(readFile(backupPath), oldBackup);
            EXPECT_EQ(cache.size(), 1);
            EXPECT_EQ(chest.lcgState, 75u);
            throw std::runtime_error("precommit entry failure");
        },
        GameTitle::MM6), std::exception);
    EXPECT_EQ(readFile(profile.primary()), oldPrimary);
    EXPECT_EQ(readFile(backupPath), oldBackup);
    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.at("prior.ddm").str(), "prior cache");
    EXPECT_EQ(chest.lcgState, 75u);
}

UNIT_TEST(MM6NewGameSaveRuntime, ScopedRuntimeCallsiteRequiresMM6EngineBeforePrivateReads) {
    TestProfile profile;
    EXPECT_THROW((void) mm6::saveNewGameMM6ScopedForTest(
        profile.root(), profile.root() / "missing-private-map-source",
        [](const Party &, const std::unordered_map<std::string, Blob> &,
           const mm6::ChestTitleState &) {}), std::exception);
    EXPECT_FALSE(std::filesystem::exists(profile.primary()));
}

UNIT_TEST(MM6NewGameSaveRuntime, WorldCollectionScratchRollsBackLateFault) {
    std::deque<Actor> actors(1);
    actors[0].id = 77;
    std::vector<SpriteObject> sprites(2);
    std::vector<Chest> chests(3);
    std::vector<LevelDecoration> decorations(4);
    EXPECT_THROW((void) mm6::ScopedFirstMapWorldCollections(
        GameTitle::MM7, &actors, &sprites, &chests, &decorations), std::exception);
    EXPECT_EQ(actors[0].id, 77);
    try {
        mm6::ScopedFirstMapWorldCollections scratch(
            GameTitle::MM6, &actors, &sprites, &chests, &decorations);
        EXPECT_TRUE(actors.empty());
        EXPECT_TRUE(sprites.empty());
        EXPECT_TRUE(chests.empty());
        EXPECT_TRUE(decorations.empty());
        actors.resize(2);
        sprites.resize(3);
        chests.resize(4);
        decorations.resize(5); // Late fault after all four families change.
        throw std::runtime_error("synthetic outdoor collection fault");
    } catch (const std::runtime_error &) {
    }
    EXPECT_EQ(actors.size(), 1);
    EXPECT_EQ(actors[0].id, 77);
    EXPECT_EQ(sprites.size(), 2);
    EXPECT_EQ(chests.size(), 3);
    EXPECT_EQ(decorations.size(), 4);

    {
        mm6::ScopedFirstMapWorldCollections scratch(
            GameTitle::MM6, &actors, &sprites, &chests, &decorations);
        actors.resize(6);
        sprites.resize(7);
        chests.resize(8);
        decorations.resize(9);
        scratch.commit();
    }
    EXPECT_EQ(actors.size(), 6);
    EXPECT_EQ(sprites.size(), 7);
    EXPECT_EQ(chests.size(), 8);
    EXPECT_EQ(decorations.size(), 9);
}

UNIT_TEST(MM6NewGameSaveRuntime, OutdoorDeltaAndCollectionsRollBackAsOneScope) {
    std::deque<Actor> actors(1);
    actors[0].id = 77;
    std::vector<SpriteObject> sprites(2);
    std::vector<Chest> chests(3);
    std::vector<LevelDecoration> decorations(4);
    LocationInfo location;
    location.lastRespawnDay = 9;
    std::vector<uint8_t> wire = {1, 2, 3};
    Time lastVisit = Time::fromTicks(123);
    MapWeather weather;
    weather.skyTextureName = "prior sky";
    mm6::FirstMapDiscoveryGrid fully = {};
    fully[2][3] = 11;
    mm6::FirstMapDiscoveryGrid partially = {};
    partially[4][5] = 22;
    size_t actorReuseCursor = 31;
    mm6::FirstMapOutdoorDeltaOwners delta{&location, &wire, &lastVisit, &weather,
                                            &fully, &partially, &actorReuseCursor};

    mm6::FirstMapOutdoorDeltaOwners incomplete = delta;
    incomplete.weather = nullptr;
    EXPECT_THROW((void) mm6::ScopedFirstMapWorldCollections(
        GameTitle::MM6, &actors, &sprites, &chests, &decorations, &incomplete), std::exception);
    EXPECT_EQ(actors[0].id, 77);
    EXPECT_EQ(wire.size(), 3);

    try {
        mm6::ScopedFirstMapWorldCollections scratch(
            GameTitle::MM6, &actors, &sprites, &chests, &decorations, &delta);
        EXPECT_TRUE(actors.empty());
        EXPECT_TRUE(wire.empty());
        EXPECT_TRUE(weather.skyTextureName.empty());
        actors.resize(5);
        sprites.resize(6);
        chests.resize(7);
        decorations.resize(8);
        location.lastRespawnDay = 99;
        wire = {9, 8};
        lastVisit = Time::fromTicks(999);
        weather.skyTextureName = "new sky";
        fully[2][3] = 33;
        partially[4][5] = 44;
        actorReuseCursor = 0;
        throw std::runtime_error("synthetic late outdoor DDM fault");
    } catch (const std::runtime_error &) {
    }
    EXPECT_EQ(actors.size(), 1);
    EXPECT_EQ(actors[0].id, 77);
    EXPECT_EQ(sprites.size(), 2);
    EXPECT_EQ(chests.size(), 3);
    EXPECT_EQ(decorations.size(), 4);
    EXPECT_EQ(location.lastRespawnDay, 9);
    EXPECT_EQ(wire, (std::vector<uint8_t>{1, 2, 3}));
    EXPECT_EQ(lastVisit, Time::fromTicks(123));
    EXPECT_EQ(weather.skyTextureName, "prior sky");
    EXPECT_EQ(fully[2][3], 11);
    EXPECT_EQ(partially[4][5], 22);
    EXPECT_EQ(actorReuseCursor, 31);

    {
        mm6::ScopedFirstMapWorldCollections scratch(
            GameTitle::MM6, &actors, &sprites, &chests, &decorations, &delta);
        actors.resize(6);
        location.lastRespawnDay = 12;
        wire = {7};
        weather.skyTextureName = "committed sky";
        actorReuseCursor = 4;
        scratch.commit();
    }
    EXPECT_EQ(actors.size(), 6);
    EXPECT_EQ(location.lastRespawnDay, 12);
    EXPECT_EQ(wire, (std::vector<uint8_t>{7}));
    EXPECT_EQ(weather.skyTextureName, "committed sky");
    EXPECT_EQ(actorReuseCursor, 4);
}

UNIT_TEST(MM6NewGameSaveRuntime, PlacementAndLocalEventsRollBackLateFault) {
    Party live;
    live.pos = Vec3f(10, 20, 30);
    live.velocity = Vec3f(4, 5, 6);
    live.uFallStartZ = 11;
    live._viewYaw = 99;
    live.floor_face_id = 7;
    Party staged = live;
    staged.pos = Vec3f(-10178, -11206, 340);
    staged.velocity = Vec3f(0, 0, -17);
    staged.uFallStartZ = 340;
    staged._viewYaw = 128;
    MapId currentMap = MAP_HARMONDALE;
    std::optional<MapDestination> pending = MapDestination(
        MAP_HARMONDALE, PartyPlacement(Vec3f(1, 2, 3), 44, 55, 66));
    EvtProgram events;
    events.add(77, EvtInstruction{});
    std::vector<std::string> strings = {"prior map"};
    std::array<unsigned char, 50> transient = {};
    transient[0] = 9;
    std::string lastSave = "prior.ec6";
    LevelType levelType = LEVEL_INDOOR;
    EngineFlags flags = {};
    flags |= ENGINE_LOADING_SAVEGAME;
    mm6::FirstMapPlacementOwners owners{&live, &currentMap, &pending, &events,
        &strings, &transient, &lastSave, &levelType, &flags};
    const MapDestination firstMap(static_cast<MapId>(15), MAP_START_POINT_PARTY);

    EXPECT_THROW((void) mm6::ScopedFirstMapPlacementAndEvents(
        GameTitle::MM7, owners, staged, firstMap), std::exception);
    EXPECT_EQ(live.pos.x, 10);
    try {
        mm6::ScopedFirstMapPlacementAndEvents scratch(GameTitle::MM6, owners, staged, firstMap);
        EXPECT_EQ(live.pos.z, 340);
        EXPECT_EQ(live.velocity.z, -17);
        EXPECT_EQ(currentMap, static_cast<MapId>(15));
        EXPECT_EQ(pending->map(), static_cast<MapId>(15));
        EXPECT_FALSE(events.hasEvent(77));
        EXPECT_TRUE(strings.empty());
        EXPECT_TRUE(lastSave.empty());
        live.pos.z = 999;
        live.floor_face_id = 22;
        currentMap = MAP_INVALID;
        pending.reset();
        events.add(88, EvtInstruction{});
        strings.push_back("failed map");
        transient[0] = 42;
        lastSave = "failed.ec6";
        levelType = LEVEL_OUTDOOR;
        flags = {};
        throw std::runtime_error("synthetic late first-map event fault");
    } catch (const std::runtime_error &) {
    }
    EXPECT_EQ(live.pos, Vec3f(10, 20, 30));
    EXPECT_EQ(live.velocity, Vec3f(4, 5, 6));
    EXPECT_EQ(live.uFallStartZ, 11);
    EXPECT_EQ(live._viewYaw, 99);
    EXPECT_EQ(live.floor_face_id, 7);
    EXPECT_EQ(currentMap, MAP_HARMONDALE);
    EXPECT_EQ(pending->map(), MAP_HARMONDALE);
    ASSERT_TRUE(pending->resolvePlacement());
    EXPECT_EQ(pending->resolvePlacement()->pos, Vec3f(1, 2, 3));
    EXPECT_EQ(pending->resolvePlacement()->yaw, 44);
    EXPECT_EQ(pending->resolvePlacement()->pitch, 55);
    EXPECT_EQ(pending->resolvePlacement()->zSpeed, 66);
    EXPECT_TRUE(events.hasEvent(77));
    EXPECT_FALSE(events.hasEvent(88));
    EXPECT_EQ(strings, std::vector<std::string>({"prior map"}));
    EXPECT_EQ(transient[0], 9);
    EXPECT_EQ(lastSave, "prior.ec6");
    EXPECT_EQ(levelType, LEVEL_INDOOR);
    EXPECT_TRUE(flags & ENGINE_LOADING_SAVEGAME);
}

UNIT_TEST(MM6NewGameSaveRuntime, LateFinalMapFailurePreservesPriorAutosave) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    std::vector<std::uint8_t> prior = mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(42), names, defaultReader(), baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6);
    int reads = 0;
    auto malformedLast = [&](std::string_view name) {
        ++reads;
        if (name == "o14.ddm") {
            const std::uint8_t byte = 1;
            return Blob::copy(&byte, 1);
        }
        return defaultReader()(name);
    };
    EXPECT_THROW((void) mm6::writeOfflineNewGameAutosave(
        profile.root(), seed(43), names, malformedLast, baseReader(),
        [](std::int32_t) { return true; }, GameTitle::MM6), std::exception);
    EXPECT_EQ(reads, 67);
    EXPECT_EQ(readFile(profile.primary()), prior);
}

UNIT_TEST(MM6NewGameSaveRuntime, BackupPreflightAndRecoveryAfterCorruptPrimary) {
    TestProfile profile;
    const std::vector<std::string> names = expectedNames();
    const auto write = [&](std::int64_t ticks) {
        return mm6::writeOfflineNewGameAutosave(profile.root(), seed(ticks), names,
            defaultReader(), baseReader(), [](std::int32_t) { return true; }, GameTitle::MM6);
    };
    std::vector<std::uint8_t> prior = write(42);
    std::vector<std::uint8_t> next = write(43);
    EXPECT_NE(next, prior);
    {
        std::ofstream corrupt(profile.primary(), std::ios::binary | std::ios::trunc);
        corrupt << "broken";
    }
    auto selected = mm6::preflightRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT,
        [&](std::span<const std::uint8_t> candidate) {
            (void) mm6::decodeKnownSaveWithActiveMapCounts(candidate, names, baseReader(),
                [](std::int32_t) { return true; });
        });
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, prior);
    EXPECT_NE(readFile(profile.primary()), prior); // Preflight never mutates disk.
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT), prior);
    EXPECT_EQ(readFile(profile.primary()), prior);
}

UNIT_TEST(MM6NewGameSaveRuntime, StagesFreshDiskSlotWithoutPublishingAnyOwner) {
    TestProfile profile;
    const auto names = expectedNames();
    Party writerParty;
    writerParty.uNumGold = 917;
    NPCStats writerNpcs{};
    Timer writerTimer;
    ActiveOverlayList writerOverlays;
    const auto entropy = [] { return std::optional<std::uint32_t>(0x76543210); };
    mm6::KnownSaveFields source = seed(123);
    source.core.party.gold = 917;
    auto prepared = mm6::writeAndStageOfflineNewGameAutosave(
        profile.root(), source, writerParty, writerNpcs, writerTimer, writerOverlays,
        names, defaultReader(), baseReader(), [](std::int32_t) { return true; },
        5, entropy, GameTitle::MM6);

    Party freshParty;
    freshParty.uNumGold = 19;
    NPCStats freshNpcs{};
    freshNpcs.pNPCData[0].name = "fresh process";
    Timer freshTimer;
    ActiveOverlayList freshOverlays;
    const auto diskBefore = readFile(profile.primary());
    auto selected = mm6::stageOfflineSaveSlotForTest(
        profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
        freshTimer, freshOverlays, names, baseReader(),
        [](std::int32_t) { return true; }, 5, GameTitle::MM6);
    ASSERT_TRUE(selected);
    EXPECT_FALSE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, prepared.bytes);
    EXPECT_EQ(selected->staged.party.pos.z, 340);
    EXPECT_EQ(selected->staged.party.uNumGold, 917);
    EXPECT_EQ(selected->staged.worldAndVisuals.world.mapDeltas.size(), 67);
    EXPECT_EQ(selected->staged.worldAndVisuals.rawActiveDelta, defaultDelta("oute3.ddm"));
    EXPECT_TRUE(selected->staged.worldAndVisuals.chestTitle.ready);
    EXPECT_EQ(selected->staged.worldAndVisuals.chestTitle.lcgState, 0x76543210u);
    EXPECT_TRUE(selected->staged.shopGuildClocks.ready);
    EXPECT_EQ(freshParty.uNumGold, 19);
    EXPECT_EQ(freshNpcs.pNPCData[0].name, "fresh process");
    EXPECT_EQ(readFile(profile.primary()), diskBefore);

    int baseReads = 0;
    const auto countingBase = [&](std::string_view name) {
        ++baseReads;
        return baseReader()(name);
    };
    EXPECT_THROW((void) mm6::stageOfflineSaveSlotForTest(
        profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
        freshTimer, freshOverlays, names, countingBase,
        [](std::int32_t) { return true; }, 5, GameTitle::MM7), std::exception);
    EXPECT_EQ(baseReads, 0);
    EXPECT_EQ(readFile(profile.primary()), diskBefore);
}

UNIT_TEST(MM6NewGameSaveRuntime, DiskStageFallsBackOnlyAfterFullCandidateValidation) {
    TestProfile profile;
    const auto names = expectedNames();
    Party writerParty;
    NPCStats writerNpcs{};
    Timer writerTimer;
    ActiveOverlayList writerOverlays;
    const auto entropy = [] { return std::optional<std::uint32_t>(0x11223344); };
    auto write = [&](std::int64_t ticks) {
        return mm6::writeAndStageOfflineNewGameAutosave(
            profile.root(), seed(ticks), writerParty, writerNpcs,
            writerTimer, writerOverlays, names, defaultReader(), baseReader(),
            [](std::int32_t) { return true; }, 5, entropy, GameTitle::MM6);
    };
    auto first = write(42);
    auto second = write(43);
    const auto backupPath = std::filesystem::path(profile.primary().string() + ".bak");
    ASSERT_EQ(readFile(backupPath), first.bytes);
    ASSERT_EQ(readFile(profile.primary()), second.bytes);

    Party freshParty;
    freshParty.uNumGold = 23;
    NPCStats freshNpcs{};
    Timer freshTimer;
    ActiveOverlayList freshOverlays;
    const auto stage = [&] {
        return mm6::stageOfflineSaveSlotForTest(
            profile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
            freshTimer, freshOverlays, names, baseReader(),
            [](std::int32_t) { return true; }, 5, GameTitle::MM6);
    };
    freshParty.bTurnBasedModeOn = true; // Late Party stage rejects both otherwise valid files.
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_EQ(readFile(profile.primary()), second.bytes);
    EXPECT_EQ(readFile(backupPath), first.bytes);
    freshParty.bTurnBasedModeOn = false;

    {
        std::ofstream corrupt(profile.primary(), std::ios::binary | std::ios::trunc);
        corrupt << "broken";
    }
    const auto corruptPrimary = readFile(profile.primary());
    auto selected = stage();
    ASSERT_TRUE(selected);
    EXPECT_TRUE(selected->needsRecovery);
    EXPECT_EQ(selected->bytes, first.bytes);
    EXPECT_EQ(selected->staged.worldAndVisuals.world.mapDeltas.size(), 67);
    EXPECT_EQ(selected->staged.worldAndVisuals.chestTitle.lcgState, 0x11223344u);
    EXPECT_EQ(readFile(profile.primary()), corruptPrimary);
    EXPECT_EQ(readFile(backupPath), first.bytes);

    {
        std::ofstream corrupt(backupPath, std::ios::binary | std::ios::trunc);
        corrupt << "also broken";
    }
    EXPECT_THROW((void) stage(), std::exception);
    EXPECT_EQ(freshParty.uNumGold, 23);
    EXPECT_EQ(readFile(profile.primary()), corruptPrimary);
    TestProfile emptyProfile;
    EXPECT_FALSE(mm6::stageOfflineSaveSlotForTest(
        emptyProfile.root(), mm6::NEW_GAME_AUTOSAVE_SLOT, freshParty, freshNpcs,
        freshTimer, freshOverlays, names, baseReader(),
        [](std::int32_t) { return true; }, 5, GameTitle::MM6).has_value());
}
#endif

} // namespace
