#pragma once

#include <string_view>

#include "Engine/Objects/CharacterEnums.h"

#include "GUI/GUIDialogues.h"

class GraphicsImage;
class GUIButton;
class GUIWindow;

namespace mm6 {

/**
 * Creates the exit button of MM6's NPC conversation window. `drawNpcDialogueOptions` draws its picture.
 *
 * @param window                        Conversation window.
 * @return                              The new button.
 */
GUIButton *createNpcDialogueExitButton(GUIWindow *window);

/**
 * Creates a topic button of MM6's NPC conversation window. `drawNpcDialogueOptions` moves it every frame.
 *
 * @param window                        Conversation window.
 * @param index                         Position of the topic in the list.
 * @param topic                         Topic the button selects.
 * @param y                             Top of the button until the first frame is drawn.
 * @param height                        Height of the button until the first frame is drawn.
 * @return                              The new button.
 */
GUIButton *createNpcDialogueOptionButton(GUIWindow *window, int index, DialogueId topic, int y, int height);

/**
 * Draws the right panel of MM6's NPC conversation window: the panel background, the NPC's portrait and name.
 *
 * @offset 0x43AB60
 * @param portrait                      NPC's portrait.
 * @param name                          NPC's name and profession.
 */
void drawNpcDialoguePanel(GraphicsImage *portrait, std::string_view name);

/**
 * Stacks the topic buttons of MM6's NPC conversation window in the middle of the right panel, draws their labels and
 * then the exit picture.
 *
 * @param window                        Conversation window, its keyboard control group holds the topic buttons.
 */
void drawNpcDialogueOptions(GUIWindow *window);

/**
 * @offset 0x43C075
 * @param reputation                    Effective party reputation, positive is good.
 * @return                              Speech the active character greets an NPC with when a conversation starts.
 */
SpeechId npcGreetingSpeech(int reputation);

} // namespace mm6
