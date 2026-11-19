#pragma once

#include "Engine/Pid.h"

void ItemInteraction(int item_id);
bool CanInteractWithActor(int id);
void InteractWithActor(int id);
void DecorationInteraction(int id, Pid pid);

/**
 * @return                              Whether a click or a Space press on an event face or decoration runs its event.
 *                                      MM6 runs it with nobody active, MM6.exe's click handler never looks at the
 *                                      active member.
 */
bool canRunInteractionEvent();

void DoInteractionWithTopmostZObject(Pid pid);
