#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"

#include "Media/MediaPlayer.h"

#include "Utility/String/Transformations.h"

namespace {

constexpr int killMovieEventId = 2;

class ProbeMovie final : public IMovie {
 public:
    unsigned int GetWidth() const override { return 1; }
    unsigned int GetHeight() const override { return 1; }
    bool Play(bool loop = false) override {
        _playing = true;
        return true;
    }
    bool Stop() override {
        _playing = false;
        return true;
    }
    bool IsPlaying() const override { return _playing; }
    Blob GetFrame() override { return {}; }
    std::string GetFormat() override { return {}; }
    void PlayBink() override {}
    bool prepare() override { return true; }
    bool renderFrame() override { return true; }

 private:
    bool _playing = true;
};

bool originalKillMovieSourcePresent() {
    if (!engine->_localEventMap.hasEvent(killMovieEventId))
        return false;
    const std::vector<EvtInstruction> &instructions = engine->_localEventMap.function(killMovieEventId);
    return instructions.size() == 6 &&
           std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
               return instruction.step == 0 && instruction.opcode == EVENT_OnMapLeave;
           }) &&
           std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
               return instruction.step == 1 && instruction.target_step == 3 &&
                      instruction.opcode == EVENT_IsActorKilled &&
                      instruction.data.actor_descr.policy == KILL_CHECK_ACTORID &&
                      instruction.data.actor_descr.param == 0 && instruction.data.actor_descr.num == 0;
           }) &&
           std::ranges::any_of(instructions, [](const EvtInstruction &instruction) {
               return instruction.step == 4 && instruction.opcode == EVENT_ShowMovie &&
                      trimRemoveQuotes(instruction.str) == "family reunion";
           });
}

void killXenofex() {
    ASSERT_FALSE(pActors.empty());
    ASSERT_EQ(pActors[0].monsterInfo.id, MONSTER_DEVIL_C);
    ASSERT_FALSE(pActors[0].IsNotAlive());
    Actor::Die(0);
    ASSERT_TRUE(pActors[0].IsNotAlive());
}

} // namespace

GAME_TEST(MM7ColonyZod, XenofexFamilyReunionMoviePlaysOnlyOnce) {
    game.startNewGame();
    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
    ASSERT_TRUE(originalKillMovieSourcePresent());
    ASSERT_FALSE(pParty->_questBits[QBIT_XENOFEX_KILLED]);
    killXenofex();

    std::shared_ptr<ProbeMovie> firstProbe = std::make_shared<ProbeMovie>();
    pMovie_Track = firstProbe;
    onMapLeave();
    ASSERT_TRUE(pParty->_questBits[QBIT_XENOFEX_KILLED]);
    EXPECT_FALSE(firstProbe->IsPlaying());
    EXPECT_EQ(pMovie_Track, nullptr); // The first ShowMovie unloaded the playing probe.

    std::shared_ptr<ProbeMovie> repeatProbe = std::make_shared<ProbeMovie>();
    pMovie_Track = repeatProbe;
    onMapLeave();
    EXPECT_EQ(pMovie_Track, repeatProbe);
    EXPECT_TRUE(repeatProbe->IsPlaying()); // The already-completed event never reached ShowMovie.
    pMovie_Track = nullptr;
}

GAME_TEST(MM7ColonyZod, RepeatMovieCompatibilityDisengagesForPatchedShape) {
    game.startNewGame();
    game.teleportTo(MAP_COLONY_ZOD, Vec3f(-1849, 6726, 934), 0);
    ASSERT_TRUE(originalKillMovieSourcePresent());
    killXenofex();
    pParty->_questBits.set(QBIT_XENOFEX_KILLED);

    EvtInstruction patchedExit(EVENT_Exit);
    patchedExit.step = 6;
    engine->_localEventMap.add(killMovieEventId, patchedExit);
    std::shared_ptr<ProbeMovie> probe = std::make_shared<ProbeMovie>();
    pMovie_Track = probe;
    onMapLeave();

    EXPECT_FALSE(probe->IsPlaying());
    EXPECT_EQ(pMovie_Track, nullptr); // The changed event remained authoritative and reached ShowMovie.
}
