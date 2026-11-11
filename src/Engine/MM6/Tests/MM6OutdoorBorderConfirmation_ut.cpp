#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorBorderConfirmation.h"

static std::unique_ptr<Party> partyAtEastEdge(int food) {
    auto party = std::make_unique<Party>();
    party->pos = Vec3f(22529, 77, 300);
    party->uNumFoodRations = food;
    party->days_played_without_rest = 2;
    party->GetPlayingTime() = Time() + Duration::fromDays(10);
    return party;
}

static mm6::BorderTravelCandidate offeredFrom(const Party &party) {
    return *mm6::assessOutdoorBorderTravel(GameTitle::MM6, "outc2.odm", party.pos.x, party.pos.y, {}).candidate;
}

UNIT_TEST(MM6OutdoorBorderConfirmation, FoodDecidesRestAndWeakness) {
    // The party rests once more when it has any food, and gets Weak when the food doesn't last the whole trip.
    EXPECT_EQ(mm6::borderConfirmationEffects(0, 5), (mm6::BorderConfirmationEffects{5, false, true, 0}));
    EXPECT_EQ(mm6::borderConfirmationEffects(3, 5), (mm6::BorderConfirmationEffects{5, true, true, 0}));
    EXPECT_EQ(mm6::borderConfirmationEffects(5, 5), (mm6::BorderConfirmationEffects{5, true, false, 0}));
    EXPECT_EQ(mm6::borderConfirmationEffects(9, 2), (mm6::BorderConfirmationEffects{2, true, false, 7}));
    EXPECT_EQ(mm6::borderConfirmationEffects(1, 1), (mm6::BorderConfirmationEffects{1, true, false, 0}));
}

UNIT_TEST(MM6OutdoorBorderConfirmation, ZeroFoodStillHeals) {
    // The trip rests the party even without food, then the hungry party gets Weak.
    const auto live = partyAtEastEdge(0);
    const Time weakSince = Time() + Duration::fromDays(3);
    live->pCharacters[0].conditions.set(CONDITION_WEAK, weakSince);
    live->pCharacters[1].health = 1;
    live->pHirelings[1].hasUsedAbility = 1;
    int rests = 0;
    const auto rest = [&](Party &party) {
        ++rests;
        for (Character &character : party.pCharacters) {
            character.conditions.reset(CONDITION_WEAK);
            character.health = 50;
        }
        party.days_played_without_rest = 0;
    };
    const auto confirmed = mm6::stageBorderConfirmation(GameTitle::MM6, "outc2.odm", offeredFrom(*live), *live, {},
                                                        rest);
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    ASSERT_NE(confirmed.party, nullptr);
    Party &party = *confirmed.party;
    const Time arrived = live->GetPlayingTime() + Duration::fromDays(5);
    EXPECT_EQ(rests, 1);
    EXPECT_EQ(party.GetPlayingTime(), arrived);
    EXPECT_EQ(party.pCharacters[1].health, 50);
    EXPECT_EQ(party.pHirelings[1].hasUsedAbility, 0);
    for (const Character &character : party.pCharacters)
        EXPECT_EQ(character.conditions.get(CONDITION_WEAK), arrived);
    EXPECT_EQ(confirmed.weakReactions, (std::vector<int>{0, 1, 2, 3}));
    EXPECT_EQ(party.days_played_without_rest, 1);
    EXPECT_EQ(party.uNumFoodRations, 0);
    EXPECT_EQ(party.pos.x, -22527);
    EXPECT_EQ(party.pos.y, 77);
    EXPECT_EQ(party.pos.z, 300);
    EXPECT_EQ(confirmed.destination->destinationMap, "outd2.odm");

    EXPECT_EQ(live->pCharacters[0].conditions.get(CONDITION_WEAK), weakSince);
    EXPECT_FALSE(live->pCharacters[1].conditions.has(CONDITION_WEAK));
    EXPECT_EQ(live->pCharacters[1].health, 1);
    EXPECT_EQ(live->pHirelings[1].hasUsedAbility, 1);
    EXPECT_EQ(live->GetPlayingTime(), Time() + Duration::fromDays(10));
    EXPECT_EQ(live->pos.x, 22529);
    EXPECT_EQ(live->days_played_without_rest, 2);
}

UNIT_TEST(MM6OutdoorBorderConfirmation, ShortFoodRestsThenWeakens) {
    const auto live = partyAtEastEdge(3);
    const auto rest = [](Party &party) {
        for (Character &character : party.pCharacters) {
            character.conditions.reset(CONDITION_WEAK);
            character.health = 50;
        }
        party.days_played_without_rest = 0;
    };
    live->pCharacters[2].conditions.set(CONDITION_WEAK, Time() + Duration::fromDays(3));
    const auto confirmed = mm6::stageBorderConfirmation(GameTitle::MM6, "outc2.odm", offeredFrom(*live), *live, {},
                                                        rest);
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    Party &party = *confirmed.party;
    const Time arrived = live->GetPlayingTime() + Duration::fromDays(5);
    for (const Character &character : party.pCharacters) {
        EXPECT_EQ(character.health, 50);
        EXPECT_EQ(character.conditions.get(CONDITION_WEAK), arrived);
    }
    EXPECT_EQ(confirmed.weakReactions, (std::vector<int>{0, 1, 2, 3}));
    EXPECT_EQ(party.days_played_without_rest, 1);
    EXPECT_EQ(party.uNumFoodRations, 0);
}

UNIT_TEST(MM6OutdoorBorderConfirmation, EnoughFoodRestsWithoutWeakness) {
    const auto live = partyAtEastEdge(10);
    int rests = 0;
    const mm6::BorderTravelHirelings hirelings{.tracker = true};
    const auto confirmed = mm6::stageBorderConfirmation(GameTitle::MM6, "outc2.odm", offeredFrom(*live), *live,
                                                        hirelings, [&](Party &) { ++rests; });
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    Party &party = *confirmed.party;
    EXPECT_EQ(rests, 2);
    EXPECT_EQ(confirmed.effects.travelDays, 3);
    EXPECT_EQ(party.GetPlayingTime(), live->GetPlayingTime() + Duration::fromDays(3));
    EXPECT_EQ(party.uNumFoodRations, 7);
    EXPECT_TRUE(confirmed.weakReactions.empty());
    for (const Character &character : party.pCharacters)
        EXPECT_FALSE(character.conditions.has(CONDITION_WEAK));
    EXPECT_EQ(party.days_played_without_rest, 2);
}

UNIT_TEST(MM6OutdoorBorderConfirmation, RefusesWithoutCopyingTheParty) {
    const auto live = partyAtEastEdge(10);
    const auto offered = offeredFrom(*live);
    int rests = 0;
    const auto rest = [&](Party &) { ++rests; };
    const auto refusal = [&](GameTitle title, std::string_view map, const mm6::BorderTravelCandidate &candidate) {
        const auto confirmed = mm6::stageBorderConfirmation(title, map, candidate, *live, {}, rest);
        EXPECT_EQ(confirmed.party, nullptr);
        return confirmed.refusal;
    };

    EXPECT_EQ(refusal(GameTitle::MM7, "outc2.odm", offered), mm6::BorderConfirmationRefusal::WrongTitle);
    live->bFlying = 1;
    EXPECT_EQ(refusal(GameTitle::MM6, "outc2.odm", offered), mm6::BorderConfirmationRefusal::Flying);
    live->bFlying = 0;

    auto elsewhere = offered;
    elsewhere.destinationMap = "outb2.odm";
    EXPECT_EQ(refusal(GameTitle::MM6, "outc2.odm", elsewhere), mm6::BorderConfirmationRefusal::RouteChanged);
    live->pos.x = 22528;
    EXPECT_EQ(refusal(GameTitle::MM6, "outc2.odm", offered), mm6::BorderConfirmationRefusal::RouteChanged);
    EXPECT_EQ(rests, 0);
    EXPECT_EQ(live->uNumFoodRations, 10);
}

UNIT_TEST(MM6OutdoorBorderConfirmation, CornerWrapsBothAxes) {
    // Crossing at a corner leads to the diagonal neighbour, with both axes wrapped.
    const auto live = partyAtEastEdge(10);
    live->pos = Vec3f(22529, 22530, 300);
    const auto confirmed = mm6::stageBorderConfirmation(GameTitle::MM6, "outc2.odm", offeredFrom(*live), *live, {},
                                                        [](Party &) {});
    ASSERT_EQ(confirmed.refusal, mm6::BorderConfirmationRefusal::None);
    EXPECT_EQ(confirmed.destination->destinationMap, "outd1.odm");
    EXPECT_EQ(confirmed.party->pos.x, -22527);
    EXPECT_EQ(confirmed.party->pos.y, -22526);
}
