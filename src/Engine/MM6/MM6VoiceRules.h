#pragma once

#include <array>
#include <cstdint>

#include "Engine/Data/SoundEnums.h"
#include "Engine/Objects/CharacterEnums.h"

class RandomEngine;

namespace mm6 {

class Executable;

inline constexpr int kVoiceCount = 12;
inline constexpr int kSpeechRowCount = 103;
inline constexpr int kVoiceLineCount = 45;

/**
 * Which recorded lines a character says a speech with, read from `MM6.exe`. MM6 numbers speeches as MM7 does up to
 * speech 102, but its lines and their sound ids are its own.
 */
struct VoiceRules {
    std::array<std::array<std::uint8_t, 2>, kSpeechRowCount> speechLines{}; // Up to two lines by speech, 0 for none.
    std::array<std::array<std::uint8_t, kVoiceCount>, kVoiceLineCount> takes{}; // Recordings of a line by voice.
};

/**
 * @param exe                           `MM6.exe` to read the speech table and the take counts from.
 * @return                              The voice rules.
 */
VoiceRules decodeVoiceRules(const Executable &exe);

/**
 * @return                              Voice rules of `executable()`, decoded on first use.
 */
const VoiceRules &voiceRules();

/**
 * Picks the sound a character says a speech with, one of the speech's lines, then one take of it.
 *
 * @offset 0x488D12
 * @param rules                         Voice rules.
 * @param speech                        Speech to say.
 * @param voice                         Character's voice, 0 to 11.
 * @param rng                           Random engine for the line and the take.
 * @return                              Sound id `5000 + 100 * voice + 2 * line + take`, or `SOUND_Invalid` when the
 *                                      speech has no line.
 */
SoundId speechSound(const VoiceRules &rules, SpeechId speech, int voice, RandomEngine *rng);

} // namespace mm6
