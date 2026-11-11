#pragma once

#include <utility>

#include "Library/Geometry/Vec.h"

class EngineController;
struct BLVFace;

/**
 * @param face                          Clickable door face.
 * @return                              Position 150 units in front of `face` and the yaw that looks at it.
 */
std::pair<Vec3f, int> approachCampaignChainFace(const BLVFace &face);

/**
 * Checks that a campaign chain chapter runs headless against the marked disposable profile it was started with.
 */
void assertCampaignChainProfile();

/**
 * Loads a chapter's save through the main menu's Load screen.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to load.
 */
void loadCampaignChainSlot(EngineController &game, int chapter);

/**
 * Saves the game to a chapter's slot and checks that the slot is a structurally valid MM7 save.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to write.
 */
void saveCampaignChainSlot(EngineController &game, int chapter);
