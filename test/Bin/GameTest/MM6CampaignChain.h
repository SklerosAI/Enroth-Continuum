#pragma once

#include <filesystem>

class EngineController;

/**
 * Makes a validated disposable profile the MM6 save root of a campaign chain chapter. Must run on the game thread.
 *
 * @param root                          Canonical owner-only `mm6-restart-chain-*` profile directory.
 */
void setMM6CampaignChainProfile(const std::filesystem::path &root);

/**
 * @return                              Profile root of the running chapter, empty outside a chain run.
 */
const std::filesystem::path &mm6CampaignChainProfile();

/**
 * Checks that an MM6 campaign chain chapter runs headless against the marked disposable profile it was started with.
 */
void assertMM6CampaignChainProfile();

/**
 * Loads a chapter's save through the main menu's Load screen.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to load.
 */
void loadMM6CampaignChainSlot(EngineController &game, int chapter);

/**
 * Saves the game from the Save menu into a new slot, which must become the chapter's slot, and checks that the slot
 * lists and reads back as a loadable MM6 save.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to write. Slots of all earlier chapters must exist.
 */
void saveMM6CampaignChainSlot(EngineController &game, int chapter);
