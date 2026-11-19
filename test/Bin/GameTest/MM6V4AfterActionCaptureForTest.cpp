#include "MM6V4AfterActionCaptureForTest.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/MM6/MM6ActiveActorOutdoorDeltaCapture.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6V5FirstMapSaveWriter.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6EventRuntimeSaveFields.h"
#include "Engine/MM6/MM6GameplayRngSaveFields.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6NpcSaveFields.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6RawV2Admission.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/MM6/MM6V4OutdoorDecorationReplay.h"
#include "Engine/MM6/MM6V4SaveFields.h"
#include "Engine/MM6/MM6V5FullSessionPreflight.h"
#include "Engine/MM6/MM6V5SaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIMessageQueue.h"
#include "Library/Platform/Application/PlatformApplication.h"

namespace mm6::testing {
namespace {

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool hasItemPayload(const Item &item) {
    return item.itemId != ITEM_NULL || item.potionPower || item.goldAmount ||
           item.standardEnchantment || item.standardEnchantmentStrength ||
           item.specialEnchantment != ITEM_ENCHANTMENT_NULL || item.numCharges ||
           item.maxCharges || item.flags || item.lichJarCharacterIndex != -1 ||
           item.enchantmentExpirationTime.ticks() || item.mm6RawRecord;
}

void requireEmptyInheritedCommerce(const Party &party) {
    for (HouseId id : party.standartItemsInShops.indices()) {
        if (party.PartyTimes.shopNextRefreshTime[id].ticks() ||
            party.PartyTimes.shopBanTimes[id].ticks() || party.InTheShopFlags[id])
            throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.standartItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.specialItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
    }
    for (HouseId id : party.spellBooksInGuilds.indices()) {
        if (party.PartyTimes.guildNextRefreshTime[id].ticks())
            throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.spellBooksInGuilds[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
    }
    for (HouseId id : party.PartyTimes.bountyHuntNextGenTime.indices())
        if (party.PartyTimes.bountyHuntNextGenTime[id].ticks() ||
            party.monster_id_for_hunting[id] != MONSTER_INVALID ||
            party.monster_for_hunting_killed[id])
            throw std::runtime_error("unrepresented inherited commerce");
}

[[gnu::noinline]] void requireSecondMovement(
    std::span<const std::uint8_t> priorBytes,
    const CorePartyFields &current) {
    auto prior = std::make_unique<V5SaveFields>(
        decodeV5Save(priorBytes, GameTitle::MM6));
    if (current.position == prior->v4.v3.raw.core.party.position ||
        current.viewYaw == prior->v4.v3.raw.core.party.viewYaw)
        throw std::runtime_error("MM6 v5 second capture lacks reloaded input movement");
}

} // namespace

static void writeAfterActionCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> sourceCandidate,
    std::span<const std::uint8_t> previousV5 = {}) {
    if (engine->gameTitle() != GameTitle::MM6 ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
        engine->_pendingTransition || pPartyActionQueue->uNumActions != 0 ||
        gameTimer->isPaused())
        throw std::runtime_error("MM6 v4 after-action capture lacks stable first-map input");
    const auto seedBefore = readFile(profile / "saves/autosave.ec6");
    const auto sourceBefore = readFile(profile / "saves/candidate-v4.ec6");
    const bool afterReloadedInput = !previousV5.empty();
    const auto previousBefore = afterReloadedInput ?
        readFile(profile / "saves/candidate-v5-after-action.ec6") :
        std::vector<std::uint8_t>{};
    if (seedBefore.empty() || !std::ranges::equal(sourceBefore, sourceCandidate))
        throw std::runtime_error("MM6 v4 after-action capture lacks original disposable slots");
    if (afterReloadedInput && (previousBefore.empty() ||
        !std::ranges::equal(previousBefore, previousV5)))
        throw std::runtime_error("MM6 v5 second capture lacks exact prior disposable slot");

    V4SaveFields saved = decodeV4Save(sourceCandidate, GameTitle::MM6);
    if (saved.v3.raw.metadata.header.mapFileName != "oute3.odm" ||
        saved.eventRuntime.maps.size() != 1 ||
        saved.eventRuntime.maps[0].mapName != "oute3.odm")
        throw std::runtime_error("MM6 v4 after-action capture lacks first-map source");
    requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    requireEmptyInheritedCommerce(*pParty);
    (void) stageRawCursorImagePlan(*pParty, rawHeldCursorState(GameTitle::MM6),
                                   *pItemTable, GameTitle::MM6);
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 v4 after-action map cache shape changed");

    V3SaveFields &fields = saved.v3;
    const auto oldPosition = fields.raw.core.party.position;
    const auto oldYaw = fields.raw.core.party.viewYaw;
    fields.raw.core = captureCoreFields(*pParty, GameTitle::MM6);
    if (fields.raw.core.party.position == oldPosition ||
        fields.raw.core.party.viewYaw == oldYaw)
        throw std::runtime_error("MM6 v4 after-action movement was not captured");
    if (afterReloadedInput)
        requireSecondMovement(previousBefore, fields.raw.core.party);
    const std::string_view slot = afterReloadedInput ?
        "candidate-v5-after-reload-input.ec6" : "candidate-v5-after-action.ec6";
    const auto attempt = writeV5FirstMapLiveSaveForTest(
        profile, slot, sourceCandidate, GameTitle::MM6);
    if (attempt.outcome != V5FirstMapSaveOutcome::Committed)
        throw std::runtime_error("MM6 v5 first-map save is not durably confirmed");
    const auto &bytes = attempt.candidate;
    if (readFile(profile / "saves" / slot) != bytes ||
        readFile(profile / "saves/candidate-v4.ec6") != sourceBefore ||
        readFile(profile / "saves/autosave.ec6") != seedBefore ||
        (afterReloadedInput &&
         readFile(profile / "saves/candidate-v5-after-action.ec6") != previousBefore))
        throw std::runtime_error("MM6 v5 atomic write changed another slot");
}

void writeV5AfterActionCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> sourceCandidate) {
    writeAfterActionCandidateForTest(profile, sourceCandidate);
}

void writeV5AfterReloadedInputCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> originalV4,
    std::span<const std::uint8_t> previousV5) {
    if (previousV5.empty())
        throw std::invalid_argument("MM6 v5 second capture requires prior candidate");
    writeAfterActionCandidateForTest(profile, originalV4, previousV5);
}

} // namespace mm6::testing
