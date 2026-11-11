#include "Engine/Evt/MM6EventVariableDispatch.h"

#include <stdexcept>

#include "Engine/Evt/MM6CircusPrizeEvent.h"
#include "Engine/Evt/MM6ConditionVariables.h"
#include "Engine/Evt/MM6DayCounterEvent.h"
#include "Engine/Evt/MM6EventVariables.h"
#include "Engine/Evt/MM6HirelingEvent.h"
#include "Engine/Evt/MM6ReputationEvent.h"
#include "Engine/Evt/MM6ResistanceEvent.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

namespace mm6 {

/**
 * Plays the sound and the portrait reaction that follow an MM6 variable write. The original also sparkles over the
 * portrait, but the shared `SpellFxRenderer::SetPlayerBuffAnim` only knows MM7 icon animations, which MM6 data lacks.
 *
 * @param character                     Character whose portrait reacts.
 * @param speech                        Reaction, `SPEECH_NONE` for writes that only play the sound.
 */
static void playWriteFeedback(Character &character, SpeechId speech) {
    if (speech != SPEECH_NONE)
        character.playReaction(speech);
    character.PlayAwardSound();
}

static void recountHirelings() {
    pParty->hirelingScrollPosition = 0;
    pParty->CountHirelings();
}

bool isPartyEventVariable(int rawId) {
    return rawId == 214 || isDayCounterVariable(rawId) || rawId == 223 || rawId == 224;
}

bool comparePartyEventVariable(int rawId, int value) {
    if (rawId == 214)
        return isEventNpcHired(*pNPCStats, value);
    if (rawId == 223) {
        if (!isValidEventProfession(value))
            throw std::runtime_error("MM6 hired profession event operand is outside the profession table");
        return CheckHiredNPCSpeciality(static_cast<NpcProfession>(value));
    }
    if (rawId == 224)
        return circusPrizeScore(pParty->pCharacters) >= value;
    return compareDayCounter(pParty->PartyTimes.CounterEventValues[dayCounterIndex(rawId)], pParty->GetPlayingTime(), value);
}

void writePartyEventVariable(EvtOpcode opcode, int rawId, int value) {
    if (rawId == 214) {
        setEventNpcHired(*pNPCStats, value, opcode != EVENT_Subtract);
        recountHirelings();
    } else if (rawId == 223 && opcode == EVENT_Subtract) {
        dismissEventProfession(*pNPCStats, *pParty, value);
        recountHirelings();
    } else if (isDayCounterVariable(rawId)) {
        pParty->PartyTimes.CounterEventValues[dayCounterIndex(rawId)] = opcode == EVENT_Subtract ? Time() : pParty->GetPlayingTime();
    } else {
        throw std::runtime_error("MM6 party event variable write is absent from shipped scripts");
    }
}

bool isCharacterEventVariable(EvtOpcode opcode, int rawId) {
    if (isResistanceVariable(rawId) || isConditionVariable(rawId) || rawId == 215 || rawId == 225)
        return true;
    return rawId == 12 && (opcode == EVENT_Set || opcode == EVENT_Add);
}

bool compareCharacterEventVariable(const Character &character, int rawId, int value) {
    if (isResistanceVariable(rawId))
        return compareEventResistance(character, rawId, value);
    if (isConditionVariable(rawId))
        return compareCondition(character.conditions, rawId);
    if (rawId == 215)
        return compareReputation(effectiveReputation(pParty->mm6Reputation, &CheckHiredNPCSpeciality), value);
    return character.uSkillPoints >= value;
}

void writeCharacterEventVariable(Character &character, EvtOpcode opcode, int rawId, int value) {
    if (rawId == 12) {
        // Unlike Character::giveAward, the reaction plays every time, even for an award the character already has.
        if (!isValidMM6SharedCandidateValue(rawId, value))
            throw std::runtime_error("MM6 award event operand is outside the award table");
        character._achievedAwardsBits.set(static_cast<AwardId>(value));
        playWriteFeedback(character, SPEECH_AWARD_GOT);
    } else if (rawId == 215) {
        pParty->mm6Reputation = reputationAfterEventWrite(opcode, pParty->mm6Reputation, value);
    } else if (isResistanceVariable(rawId)) {
        if (opcode == EVENT_Set)
            setEventResistance(character, rawId, value);
        else if (opcode == EVENT_Add)
            addEventResistance(character, rawId, value);
        else
            subtractEventResistance(character, rawId, value);
        playWriteFeedback(character, eventResistanceReaction(rawId));
    } else if (isConditionVariable(rawId)) {
        if (opcode != EVENT_Subtract) {
            startCondition(character.conditions, rawId, pParty->GetPlayingTime());
            playWriteFeedback(character, SPEECH_NONE);
        } else if (clearCondition(character.conditions, rawId)) {
            playWriteFeedback(character, SPEECH_NONE);
        }
    } else if (opcode == EVENT_Set) {
        character.uSkillPoints = value;
    } else if (opcode == EVENT_Add) {
        character.uSkillPoints += value;
    } else {
        character.uSkillPoints = static_cast<unsigned>(value) > static_cast<unsigned>(character.uSkillPoints) ? 0 : character.uSkillPoints - value;
    }
}

} // namespace mm6
