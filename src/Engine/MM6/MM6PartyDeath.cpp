#include "Engine/MM6/MM6PartyDeath.h"

#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtEnums.h"
#include "Engine/Evt/MM6EventVariableDispatch.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/MM6/MM6Finale.h"
#include "Engine/MM6/MM6MapTransition.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "Engine/Spells/SpellBuff.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Timer.h"
#include "Engine/TurnEngine/TurnEngine.h"

#include "GUI/UI/UIStatusBar.h"

#include "Media/Audio/AudioPlayer.h"
#include "Media/MediaPlayer.h"

namespace mm6 {

static constexpr int deathsAward = 82;

/**
 * MM6 counterpart of `Actor::InitializeActors`. Every monster takes its hostility back from the monster table, where
 * MM7 makes them all friendly, and peasants stay aggressors.
 */
static void resetActorsAfterPartyDeath() {
    for (size_t i = 0; i < pActors.size(); ++i) {
        Actor &actor = pActors[i];
        if (actor.CanAct()) {
            actor.pos = actor.initialPosition;
            actor.hp = actor.monsterInfo.hp;
            Actor::AI_Stand(i, Pid(OBJECT_Character, 0), actor.monsterInfo.recoveryTime, nullptr);
        }
        actor.monsterInfo.hostilityType = pMonsterStats->infos[actor.monsterInfo.id].hostilityType;
    }
}

void applyPartyDeath(Party *party, const std::function<int(const Character &)> &maxHealth) {
    party->pHirelings.fill(NPCData());
    ++party->uNumDeaths;
    party->days_played_without_rest = 0;
    party->GetPlayingTime() += Duration::fromDays(7);
    party->last_regenerated = party->GetPlayingTime(); // No regeneration over the skipped week.
    party->uFlags &= ~PARTY_FLAG_WATER_DAMAGE;

    const SpellBuff guardianAngel = party->pPartyBuffs[static_cast<PartyBuff>(8)]; // MM6 buffs keep their original slots, 8 is Guardian Angel.
    party->SetGold(guardianAngel.Active() ? party->uNumGold / 2 : 0, true);
    party->pPartyBuffs.fill(SpellBuff());
    for (Character &character : party->pCharacters) {
        character.conditions.resetAll();
        character.pCharacterBuffs.fill(SpellBuff());
        character.health = 1;
        if (guardianAngel.Active() && guardianAngel.skillMastery == MASTERY_EXPERT)
            character.health = maxHealth(character) / 2;
        else if (guardianAngel.Active() && guardianAngel.skillMastery == MASTERY_MASTER)
            character.health = maxHealth(character);
    }
    party->setActiveCharacterIndex(0);

    party->pos = Vec3f(-9728, -11319, 160);
    party->uFallStartZ = party->pos.z;
    party->_viewYaw = 512;
    party->_viewPitch = 0;
    party->velocity = Vec3f();
}

void respawnDeadParty() {
    pAudioPlayer->stopSounds();
    if (isHiveFinaleUnderway()) {
        playFinaleEnding(FINALE_ENDING_LOSE);
        return;
    }

    for (int i = 0; i < pNPCStats->uNumNewNPCs; ++i)
        if (pNPCStats->pNPCData[i].canJoin)
            pNPCStats->pNPCData[i].flags &= ~NPC_HIRED;
    pMediaPlayer->PlayFullscreenMovie("losegame");
    if (pMovie_Track)
        pMediaPlayer->Unload();

    for (Character &character : pParty->pCharacters)
        writeCharacterEventVariable(character, EVENT_Set, std::to_underlying(VAR_Award), deathsAward);
    applyPartyDeath(pParty, [](const Character &character) { return character.GetMaxHealth(); });
    pAudioPlayer->playUISound(SOUND_gold01); // The original's gold write always plays the coin sound.
    pActiveOverlayList->Reset();
    if (pParty->bTurnBasedModeOn) {
        pTurnEngine->End(true);
        pParty->bTurnBasedModeOn = false;
    }

    const MapId respawnMap = pMapTable->GetMapInfo("oute3.odm");
    if (engine->_currentLoadedMapId != respawnMap) {
        leaveMap(respawnMap);
        engine->_pendingTransition = MapDestination(respawnMap, PartyPlacement(pParty->pos, pParty->_viewYaw, pParty->_viewPitch, 0));
        PrepareWorld(1);
    }
    animTimer->setPaused(false);
    gameTimer->setPaused(false);
    resetActorsAfterPartyDeath();

    const int speaker = pParty->getRandomActiveCharacterId(vrng);
    if (speaker != -1)
        pParty->pCharacters[speaker].playReaction(SPEECH_CHEATED_DEATH);
    engine->_statusBar->setEvent(LSTR_ONCE_AGAIN_YOUVE_CHEATED_DEATH);
    uGameState = GAME_STATE_PLAYING;
}

} // namespace mm6
