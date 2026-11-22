#include "MM6VoiceRules.h"

#include <array>
#include <utility>

#include "Engine/MM6/MM6Executable.h"

#include "Library/Random/RandomEngine.h"

namespace mm6 {

VoiceRules decodeVoiceRules(const Executable &exe) {
    VoiceRules result;
    for (int speech = 0; speech < kSpeechRowCount; speech++)
        for (int slot = 0; slot < 2; slot++)
            result.speechLines[speech][slot] = exe.u8(0x4C22F0 + 8 * speech + slot); // Bytes 3 to 7 are the portrait reactions.
    for (int line = 0; line < kVoiceLineCount; line++)
        for (int voice = 0; voice < kVoiceCount; voice++)
            result.takes[line][voice] = exe.u8(0x4C20D0 + kVoiceCount * line + voice);
    return result;
}

const VoiceRules &voiceRules() {
    static const VoiceRules rules = decodeVoiceRules(executable());
    return rules;
}

SoundId speechSound(const VoiceRules &rules, SpeechId speech, int voice, RandomEngine *rng) {
    const int row = std::to_underlying(speech);
    if (row >= kSpeechRowCount || voice < 0 || voice >= kVoiceCount)
        return SOUND_Invalid;
    std::array<int, 2> lines{};
    int lineCount = 0;
    for (int line : rules.speechLines[row])
        if (line)
            lines[lineCount++] = line;
    if (!lineCount)
        return SOUND_Invalid;
    const int line = lines[rng->random(lineCount)];
    const int takes = rules.takes[line][voice];
    if (!takes)
        return SOUND_Invalid;
    return static_cast<SoundId>(5000 + 100 * voice + 2 * line + rng->random(takes));
}

} // namespace mm6
