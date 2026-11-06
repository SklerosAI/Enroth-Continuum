#pragma once

#include <optional>
#include <string>

#include "Engine/Evt/EvtContinuation.h"
#include "Engine/Evt/EvtEnums.h"

#include "GUI/GUIWindow.h"

struct MM6QuestionDialogue {
    std::string question;
    std::string answer1;
    std::string answer2;
    EvtContinuation success;
    EvtContinuation incorrect;
};

class GUIWindow_BranchlessDialogue : public GUIWindow {
 public:
    GUIWindow_BranchlessDialogue(EvtOpcode opcode, std::optional<EvtContinuation> continuation);
    virtual ~GUIWindow_BranchlessDialogue();

    virtual void Update() override;

    EvtOpcode opcode() const {
        return _opcode;
    }

    [[nodiscard]] std::optional<EvtContinuation> continuation() const {
        return _continuation;
    }

    void setMM6Question(MM6QuestionDialogue question);

 private:
    EvtOpcode _opcode = EVENT_Invalid;
    std::optional<EvtContinuation> _continuation;
    std::optional<MM6QuestionDialogue> _mm6Question;
};

/**
 * @param opcode                        Command that opens the dialogue, `EVENT_PressAnyKey` or `EVENT_InputString`, or `EVENT_Invalid` for an
 *                                      NPC's catchphrase.
 * @param continuation                  Event paused on the dialogue, or none for a catchphrase.
 */
void startBranchlessDialogue(EvtOpcode opcode, std::optional<EvtContinuation> continuation);

/** Opens an MM6 password prompt, resuming only after confirmation. */
void startMM6QuestionDialogue(MM6QuestionDialogue question);

/**
 * Closes the dialogue, returns to the game screen and resumes the event paused on the dialogue, if there is one.
 */
void releaseBranchlessDialogue();
