#include "SaveLoad.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6NewGameSession.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"
#include "Engine/TurnEngine/TurnEngine.h"

#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Renderer/Renderer.h"

#include "Engine/Objects/SpriteObject.h"

#include "Engine/Snapshots/CompositeSnapshots.h"

#include "Engine/Spells/CastSpellInfo.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/MM6SaveLoadMenu.h"
#include "GUI/UI/UIGame.h"
#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"

#include "Library/Snapshots/SnapshotSerialization.h"
#include "Library/Image/Pcx.h"
#include "Library/Logger/Logger.h"
#include "Library/LodFormats/LodFormats.h"

std::unordered_map<std::string, Blob> pMapDeltas;

namespace {
std::function<void()> mm6NewGameSaveOverride;
std::function<void(std::string_view)> mm6LoadGameOverride;

void reportFailedMM7SaveAction() {
    if (engine->_statusBar)
        engine->_statusBar->setEvent(LSTR_ERROR);
    if (pAudioPlayer)
        pAudioPlayer->playUISound(SOUND_error);
}

bool writeMm7Save(std::string_view path, const Blob &bytes) {
    const auto profileRoot = mm7::startupNativeProfileRoot();
    constexpr std::string_view savesPrefix = "saves/";
    if (!profileRoot || !path.starts_with(savesPrefix) || path.size() == savesPrefix.size() ||
        !pGames_LOD || !pGames_LOD->isOpen()) {
        ufs->write(path, bytes);
        return true;
    }

    const std::string_view slotName = path.substr(savesPrefix.size());
    const auto validate = [](std::span<const std::uint8_t> candidate) {
        return mm7::preflightNativeSaveStructure(candidate, GameTitle::MM7, *pGames_LOD);
    };
    mm7::writeAtomicSaveSlot(*profileRoot, slotName,
                             std::span<const std::uint8_t>(
                                 static_cast<const std::uint8_t *>(bytes.data()), bytes.size()),
                             validate);
    return true;
}
} // namespace

void mm6::testing::setNewGameSaveOverride(std::function<void()> callback) {
    mm6NewGameSaveOverride = std::move(callback);
}

void mm6::testing::setLoadGameOverride(std::function<void(std::string_view)> callback) {
    mm6LoadGameOverride = std::move(callback);
}

bool loadGame(std::string_view fileName) {
    if (engine->gameTitle() == GameTitle::MM6) {
        if (mm6LoadGameOverride) {
            mm6LoadGameOverride(fileName);
        } else {
            mm6::loadFirstMapManualSlot(fileName);
        }
        return true;
    }

    std::string path = fmt::format("saves/{}", fileName);
    if (!ufs->exists(path)) {
        pAudioPlayer->playUISound(SOUND_error);
        MM_WARNING("loadGame: '{}' doesn't exist", fileName);
        return false;
    }

    // Deserialize and inspect a detached copy before altering any live state. The
    // load menu intentionally accepts a damaged thumbnail, so this is the first
    // point at which a valid-header save with an unsafe body can be refused.
    const Blob saveBytes = Blob::copy(ufs->read(path));
    if (!pGames_LOD || !pGames_LOD->isOpen() ||
        !mm7::preflightNativeSaveStructure(
            std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saveBytes.data()), saveBytes.size()),
            GameTitle::MM7, *pGames_LOD)) {
        MM_WARNING("loadGame: '{}' failed MM7 structural save admission", fileName);
        reportFailedMM7SaveAction();
        return false;
    }

    engine->_lastLoadedSaveFileName = fileName;

    // TODO(captainurist): remained from Party::Reset, doesn't really belong here (or in Party::Reset).
    CastSpellInfoHelpers::cancelSpellCastInProgress();
    current_character_screen_window = WINDOW_CHARACTER_STATS;
    if (pParty->bTurnBasedModeOn) {
        pTurnEngine->End(false);
        pParty->bTurnBasedModeOn = false;
    }

    SaveGame state;
    deserialize(saveBytes, &state, tags::via<SaveGame_MM7>);

    // Move loaded state to global variables.
    *pParty = std::move(state.party);
    *gameTimer = std::move(state.eventTimer);
    *pActiveOverlayList = std::move(state.overlays);
    pNPCStats->pNPCData = std::move(state.npcData);
    pNPCStats->pGroups = std::move(state.npcGroups);
    pMapDeltas = std::move(state.mapDeltas);

    // Patch up the game timer.
    gameTimer->setPaused(true); // We're loading the game now => game timer is paused.
    gameTimer->setTurnBased(false);

    // We always start in realtime after loading a game.
    pParty->bTurnBasedModeOn = false;

    // Vanilla MM7's evil temples and Reanimate could make a Lich a zombie. Only the temples swapped in a zombie face,
    // 23 or 24, and they kept the Lich face in uPrevFace.
    for (Character &character : pParty->pCharacters) {
        if (character.classType == CLASS_LICH && character.IsZombie()) {
            character.conditions.reset(CONDITION_ZOMBIE);
            if (character.uCurrentFace == 23 || character.uCurrentFace == 24) {
                character.uCurrentFace = character.uPrevFace;
                character.uVoiceID = character.uPrevVoiceID;
            }
        }
    }

    pParty->setActiveCharacterIndex(-1);
    pParty->setActiveToFirstCanAct();

/*
    for (int i = 0; i < 4; ++i) {
        if (pParty->pCharacters[i].uQuickSpell) {
            AA1058_PartyQuickSpellSound[i].AddPartySpellSound(
                pParty->pCharacters[i].uQuickSpell, i + 1);
        }

        for (int j = 0; j < 2; ++j) {
            int uEquipIdx = pParty->pCharacters[i].pEquipment[j];
            if (uEquipIdx) {
                int pItemID = pParty->pCharacters[i]
                                  .pInventoryItemList[uEquipIdx - 1]
                                  .uItemID;
                if (pItemTable->pItems[pItemID].uEquipType == ITEM_TYPE_WAND &&
                    pItemID) {
                    assert(false);  // looks like offset in player's inventory
                                     // and wand_lut much like case in 0042ECB5
                    stru_A750F8[i].AddPartySpellSound(
                        wand_spell_ids[pItemID], i + 9);
                }
            }
        }
    }
*/
    current_screen_type = SCREEN_GAME;

    SetUserInterface(pParty->alignment);

    if (!pGames_LOD->exists(state.header.locationName)) {
        MM_ERROR("Unable to find: {}!", state.header.locationName);
    }

    // TODO(captainurist): the start point is a placeholder, the save carries the party's position and the loaders
    //                     skip placement when loading. MapDestination has no way to say that.
    engine->_pendingTransition = MapDestination(pMapTable->GetMapInfo(state.header.locationName), MAP_START_POINT_PARTY);

    engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;

    // pAudioPlayer->SetMusicVolume(engine->config->music_level);
    // pAudioPlayer->SetMasterVolume(engine->config->sound_level);

    MM7Initialization();

    // TODO: disable flashing for all books until we save state to savegame file
    bFlashQuestBook = false;
    bFlashAutonotesBook = false;
    bFlashHistoryBook = false;
    return true;
}

namespace {
struct StagedSaveData {
    SaveGameHeader header;
    Blob blob;
    std::unordered_map<std::string, Blob> mapDeltas;
    Time *liveVisitTime = nullptr;
    Time visitTime;

    void commit() noexcept {
        static_assert(std::is_nothrow_copy_assignable_v<SpriteObject>);
        static_assert(std::is_nothrow_destructible_v<SpriteObject>);
        if (liveVisitTime) {
            *liveVisitTime = visitTime;
            CompactLayingItemsList();
        }
        pMapDeltas.swap(mapDeltas);
    }
};

StagedSaveData stageSaveData(bool resetWorld, std::string_view title) {
    StagedSaveData staged;

    std::string currentMapName = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;

    // Populate SaveGameState from global variables.
    SaveGame state;
    state.header.name = title;
    state.header.locationName = currentMapName;
    state.header.playingTime = pParty->GetPlayingTime();
    state.party = *pParty;
    state.eventTimer = *gameTimer;
    state.overlays = *pActiveOverlayList;
    state.npcData = pNPCStats->pNPCData;
    state.npcGroups = pNPCStats->pGroups;

    // Populate map deltas.
    if (resetWorld) {
        // New game - copy ddm & dlv files from games.lod.
        for (const std::string &name : pGames_LOD->ls())
            if (name.ends_with(".ddm") || name.ends_with(".dlv"))
                state.mapDeltas[name] = pGames_LOD->read(name);
    } else {
        // Location change - copy from current save & serialize current location delta.
        // Blob is move-only, so we need explicit copies.
        for (const auto &[key, value] : pMapDeltas)
            state.mapDeltas[key] = Blob::share(value);

        staged.visitTime = pParty->GetPlayingTime();
        Blob uncompressed;
        if (uCurrentlyLoadedLevelType == LEVEL_INDOOR) {
            staged.liveVisitTime = &pIndoor->lastVisitTime;
            IndoorDelta_MM7 delta{};
            snapshot(*pIndoor, &delta);
            delta.lastVisitTime = staged.visitTime.ticks();
            std::erase_if(delta.spriteObjects, [](const SpriteObject_MM7 &sprite) { return !sprite.objectDescId; });
            serialize(delta, &uncompressed);
        } else {
            assert(uCurrentlyLoadedLevelType == LEVEL_OUTDOOR);
            staged.liveVisitTime = &pOutdoor->lastVisitTime;
            OutdoorDelta_MM7 delta{};
            snapshot(*pOutdoor, &delta);
            delta.lastVisitTime = staged.visitTime.ticks();
            std::erase_if(delta.spriteObjects, [](const SpriteObject_MM7 &sprite) { return !sprite.objectDescId; });
            serialize(delta, &uncompressed);
        }

        std::string deltaName = currentMapName;
        size_t pos = deltaName.find_last_of('.');
        deltaName[pos + 1] = 'd';
        state.mapDeltas[deltaName] = lod::encodeCompressed(uncompressed);
    }

    // Capture thumbnail.
    state.thumbnail = pcx::encode(render->MakeViewportScreenshot(150, 112));

    Blob blob;
    serialize(state, &blob, tags::via<SaveGame_MM7>);

    staged.header = std::move(state.header);
    staged.blob = std::move(blob);
    staged.mapDeltas = std::move(state.mapDeltas);
    return staged;
}
} // namespace

std::pair<SaveGameHeader, Blob> createSaveData(bool resetWorld, std::string_view title) {
    if (engine->gameTitle() == GameTitle::MM6)
        throw std::runtime_error("MM6 save encoding is unavailable until the EC6 transaction is complete");

    StagedSaveData staged = stageSaveData(resetWorld, title);
    return {std::move(staged.header), std::move(staged.blob)};
}

SaveGameHeader saveGame(bool isAutoSave, bool resetWorld, std::string_view path, std::string_view title) {
    if (engine->gameTitle() == GameTitle::MM6)
        throw std::runtime_error("MM6 save writing is unavailable until the EC6 transaction is complete");

    assert(isAutoSave || !title.empty());
    assert(engine->_currentLoadedMapId != MAP_ARENA || isAutoSave); // No manual saves in Arena.

    if (engine->_currentLoadedMapId == MAP_ARENA) {
        return {};
    }

    StagedSaveData staged = stageSaveData(resetWorld, title);

    try {
        writeMm7Save(path, staged.blob);
    } catch (const std::exception &e) {
        MM_WARNING("saveGame: failed to write {}: {}", isAutoSave ? "autosave" : "save", e.what());
        return {};
    }

    staged.commit();
    return std::move(staged.header);
}

void autoSave() {
    if (engine->gameTitle() == GameTitle::MM6) {
        mm6AutoSave();
        return;
    }
    saveGame(true, false, fmt::format("saves/{}", autosaveFileName), localization->str(LSTR_AUTOSAVE));
}

void doSavegame(std::string fileName, std::string_view title) {
    if (engine->gameTitle() == GameTitle::MM6) {
        mm6SaveFromMenu(fileName, title);
        return;
    }
    assert(engine->_currentLoadedMapId != MAP_ARENA); // Not Arena.

    if (fileName.empty()) {
        for (int i = 0;; i++) { // New save, pick the first free numbered name.
            fileName = fmt::format("save{:03}.mm7", i);
            if (!ufs->exists(fmt::format("saves/{}", fileName)))
                break;
        }
    }

    if (saveGame(false, false, fmt::format("saves/{}", fileName), title).locationName.empty()) {
        reportFailedMM7SaveAction();
        return;
    }

    GUI_UpdateWindows();
    pGUIWindow_CurrentMenu = nullptr;
    current_screen_type = SCREEN_GAME;

    gameTimer->setPaused(false);
    engine->_statusBar->setEvent(LSTR_GAME_SAVED);
}

void saveNewGame() {
    if (engine->gameTitle() == GameTitle::MM6 && !mm6NewGameSaveOverride) {
        engine->_lastLoadedSaveFileName.clear();
        mm6::startNewGameSession();
        return;
    }

    if (engine->gameTitle() == GameTitle::MM6) {
        const MapId priorMap = engine->_currentLoadedMapId;
        const auto priorPosition = pParty->pos;
        const auto priorFallStartZ = pParty->uFallStartZ;
        const auto priorPitch = pParty->_viewPitch;
        const auto priorYaw = pParty->_viewYaw;
        std::string priorSaveName = engine->_lastLoadedSaveFileName;
        try {
            engine->_currentLoadedMapId = pMapTable->GetMapInfo("oute3.odm");
            // New Sorpigal's arrival point in the original MM6 coordinate system.
            pParty->pos.x = -10178;
            pParty->pos.y = -11206;
            pParty->pos.z = 340;
            pParty->uFallStartZ = 340;
            pParty->_viewPitch = 0;
            pParty->_viewYaw = 128;
            engine->_lastLoadedSaveFileName.clear();
            mm6NewGameSaveOverride();
        } catch (...) {
            engine->_currentLoadedMapId = priorMap;
            pParty->pos = priorPosition;
            pParty->uFallStartZ = priorFallStartZ;
            pParty->_viewPitch = priorPitch;
            pParty->_viewYaw = priorYaw;
            engine->_lastLoadedSaveFileName.swap(priorSaveName);
            throw;
        }
        return;
    }

    engine->_currentLoadedMapId = MAP_EMERALD_ISLAND;
    pParty->pos.x = 12552;
    pParty->pos.y = 800;
    pParty->pos.z = 193;
    pParty->uFallStartZ = 193;
    pParty->_viewPitch = 0;
    pParty->_viewYaw = 512;
    engine->_lastLoadedSaveFileName.clear(); // Otherwise the save menu would pre-select a save from a previous playthrough.
    saveGame(true, true, fmt::format("saves/{}", autosaveFileName), localization->str(LSTR_AUTOSAVE));
}

void quickSaveGame() {
    if (engine->gameTitle() == GameTitle::MM6) {
        mm6QuickSave();
        return;
    }
    assert(engine->_currentLoadedMapId != MAP_ARENA); // Not Arena.

    auto &quickSaveIndex = engine->config->gameplay.QuickSavesCount;
    const int previousIndex = quickSaveIndex.value();
    quickSaveIndex.cycleIncrement();
    try {
        if (saveGame(false, false, fmt::format("saves/{}", getCurrentQuickSave()),
                     localization->str(LSTR_QUICKSAVE)).locationName.empty()) {
            quickSaveIndex.setValue(previousIndex);
            reportFailedMM7SaveAction();
            return;
        }
    } catch (...) {
        quickSaveIndex.setValue(previousIndex);
        throw;
    }
    engine->_statusBar->setEvent(LSTR_GAME_SAVED);
    pAudioPlayer->playUISound(SOUND_StartMainChoice02);
}

void quickLoadGame() {
    std::string fileName = getCurrentQuickSave();
    if (engine->gameTitle() == GameTitle::MM6) {
        if (!mm6LoadOverGame(fileName))
            return;
        uGameState = GAME_STATE_LOADING_GAME;
        pAudioPlayer->playUISound(SOUND_StartMainChoice02);
        return;
    }

    if (!ufs->exists(fmt::format("saves/{}", fileName))) {
        MM_ERROR("QuickLoadGame:: No quick save could be found!");
        pAudioPlayer->playUISound(SOUND_error);
        return;
    }

    if (!loadGame(fileName))
        return;
    uGameState = GAME_STATE_LOADING_GAME;
    pAudioPlayer->playUISound(SOUND_StartMainChoice02);
}

std::string getCurrentQuickSave() {
    const std::string_view extension = engine->gameTitle() == GameTitle::MM6 ? "ec6" : "mm7";
    return fmt::format("{}{}.{}", quickSaveFileNamePrefix,
                       engine->config->gameplay.QuickSavesCount.value(), extension);
}
