#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"
#include "Engine/MM6/MM6VoiceRules.h"
#include "Engine/Snapshots/TableSerialization.h"

#include "Library/Random/SequentialRandomEngine.h"
#include "Library/Snd/SndReader.h"

#include "Media/Audio/SoundList.h"

UNIT_TEST(MM6VoiceRules, SpeechLines) {
    // MM6 voices begging, threats and bribes and both NPC greetings, but not the coach or boat departure.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::VoiceRules rules = mm6::decodeVoiceRules(mm6::Executable(data->read("mm6.exe")));
    using Lines = std::array<std::uint8_t, 2>;
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_GOOD_DAY)], Lines({31, 32}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_GOOD_EVENING)], Lines({4, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_BEG)], Lines({41, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_BEG_FAIL)], Lines({37, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_THREAT)], Lines({3, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_THREAT_FAIL)], Lines({11, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_BRIBE)], Lines({7, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_BRIBE_FAIL)], Lines({6, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_TRAVEL_HORSE)], Lines({0, 0}));
    EXPECT_EQ(rules.speechLines[std::to_underlying(SPEECH_TRAVEL_BOAT)], Lines({0, 0}));

    SequentialRandomEngine rng;
    EXPECT_EQ(mm6::speechSound(rules, SPEECH_GOOD_DAY, 0, &rng), static_cast<SoundId>(5064)); // Line 32, MaleA32a.
    EXPECT_EQ(mm6::speechSound(rules, SPEECH_GOOD_EVENING, 11, &rng), static_cast<SoundId>(6108)); // Line 4, GirlD04a.
    EXPECT_EQ(mm6::speechSound(rules, SPEECH_TRAVEL_HORSE, 0, &rng), SOUND_Invalid);
    EXPECT_EQ(mm6::speechSound(rules, SPEECH_AWAKEN, 0, &rng), SOUND_Invalid); // Past MM6's speech table.
    EXPECT_EQ(mm6::speechSound(rules, SPEECH_GOOD_DAY, 12, &rng), SOUND_Invalid);
}

UNIT_TEST(MM6VoiceRules, EveryTakeHasASample) {
    // Every take of every voice line has a sample in dsounds.bin and Audio.snd, except nine that the shipped data lacks.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::VoiceRules rules = mm6::decodeVoiceRules(mm6::Executable(data->read("mm6.exe")));
    SoundList sounds;
    deserialize(data->iconsData("dsounds.bin"), &sounds, GameTitle::MM6);
    const SndReader audio(data->read("sounds/audio.snd"));

    std::vector<int> missing;
    for (int line = 1; line < mm6::kVoiceLineCount; line++) {
        for (int voice = 0; voice < mm6::kVoiceCount; voice++) {
            for (int take = 0; take < rules.takes[line][voice]; take++) {
                const int id = 5000 + 100 * voice + 2 * line + take;
                const SoundInfo *info = sounds.soundInfo(static_cast<SoundId>(id));
                if (!info || !audio.exists(info->name))
                    missing.push_back(id);
            }
        }
    }
    std::ranges::sort(missing);
    EXPECT_EQ(missing, std::vector<int>({5135, 5282, 5578, 5629, 5635, 5674, 5956, 6033, 6074}));
}
