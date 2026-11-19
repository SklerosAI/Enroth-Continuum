#include "MM6Transport.h"

#include <algorithm>
#include <ranges>
#include <string>
#include <utility>

#include "Engine/AssetsManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Localization.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIFont.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/Houses/MM6ShopPrices.h"

#include "Library/Platform/Interface/Platform.h"

static constexpr QuestBit QBIT_MM6_PRICE_FIXING_TOPIC = static_cast<QuestBit>(116); // Loretta Fleise's topic sets it.
static constexpr QuestBit QBIT_MM6_ALL_STABLE_PRICES_FIXED = static_cast<QuestBit>(117); // Her reward topic reads it.

GUIWindow_MM6Transport::GUIWindow_MM6Transport(HouseId houseId, const mm6::HousePresentationState &presentation)
    : GUIWindow_MM6House(houseId, presentation) {}

void GUIWindow_MM6Transport::Update() {
    if (_departureTime && platform->tickCount() >= *_departureTime)
        depart();

    GUIWindow_MM6House::Update();
    if (_route || !isProprietorSelected() || !pParty->hasActiveCharacter())
        return;

    const Character &character = pParty->activeCharacter();
    if (!character.CanAct()) {
        const std::string refusal = localization->format(LSTR_S_IS_IN_NO_CONDITION_TO_S, character.name, localization->str(LSTR_DO_ANYTHING));
        drawPanelText(refusal, (212 - panelTextHeight(refusal)) / 2 + 101, colorTable.PaleCanary);
        return;
    }
    const std::array<const mm6::TransportRoute *, 3> routes = offeredRoutes();
    const bool anyRoute = std::ranges::any_of(routes, [](const mm6::TransportRoute *route) { return route != nullptr; });
    if (anyRoute || offersToFixPrices()) {
        drawPanelText(priceLine(), 146, colorTable.White);
    } else {
        const std::string sorry = localization->str(LSTR_SORRY_COME_BACK_ANOTHER_DAY);
        drawPanelText(sorry, (174 - panelTextHeight(sorry)) / 2 + 138, colorTable.White);
    }
}

void GUIWindow_MM6Transport::selectService(int index) {
    if (_route || !isProprietorSelected() || !pParty->hasActiveCharacter() || !pParty->activeCharacter().CanAct() || index < 0 ||
        index >= std::ssize(_services))
        return;
    if (_services[index] == mm6::HouseServiceOption::FixPrices) {
        if (offersToFixPrices())
            fixPrices();
        return;
    }
    const mm6::TransportRoute *route = offeredRoutes()[index];
    if (!route)
        return;

    const int cost = price();
    if (pParty->GetGold() < cost) {
        engine->_statusBar->setEvent(LSTR_YOU_DONT_HAVE_ENOUGH_GOLD);
        mm6PlayHouseSound(houseId(), HOUSE_SOUND_TRANSPORT_NOT_ENOUGH_GOLD);
        return;
    }

    const bool stable = mm6::isTransportStable(std::to_underlying(houseId()));
    _route = route;
    _fare = cost;
    _tripDays = travelDays(*route);
    _paysOnDeparture = engine->config->gameplay.FixTravelAutosave.value();
    if (!_paysOnDeparture)
        payForTrip();
    mm6PlayHouseSound(houseId(), HOUSE_SOUND_TRANSPORT_TRAVEL);
    pParty->activeCharacter().playReaction(stable ? SPEECH_TRAVEL_HORSE : SPEECH_TRAVEL_BOAT);
    _departureTime = platform->tickCount() + (stable ? 1500 : 2500);
}

bool GUIWindow_MM6Transport::goBack() {
    if (_departureTime)
        return true;
    if (_departed)
        return false;
    return GUIWindow_MM6House::goBack();
}

std::string GUIWindow_MM6Transport::serviceLabel(int index) const {
    if (_route || !pParty->hasActiveCharacter() || !pParty->activeCharacter().CanAct())
        return {};
    if (_services[index] == mm6::HouseServiceOption::FixPrices)
        return offersToFixPrices() ? pNPCTopics[99].pTopic : std::string();
    const mm6::TransportRoute *route = offeredRoutes()[index];
    if (!route)
        return {};
    const MapId destination = pMapTable->GetMM6TransportMapInfo(*pGames_LOD, route->mapIndex);
    return localization->format(mm6::houseServiceOptionLabel(_services[index]), travelDays(*route), pMapTable->pInfos[destination].name);
}

int GUIWindow_MM6Transport::stackedMenuTop() const {
    return 146 + panelTextHeight(priceLine()) + assets->pFontArrus->GetHeight() - 3; // A blank line under the price.
}

std::string GUIWindow_MM6Transport::priceLine() const {
    return localization->format(LSTR_TRAVEL_COST_D_GOLD, price());
}

std::array<const mm6::TransportRoute *, 3> GUIWindow_MM6Transport::offeredRoutes() const {
    return mm6::transportRules().offeredRoutes(std::to_underlying(houseId()), pParty->uCurrentDayOfMonth % 7,
                                               [](int bit) { return pParty->_questBits[static_cast<QuestBit>(bit)]; });
}

int GUIWindow_MM6Transport::price() const {
    return mm6::transportPrice(std::to_underlying(houseId()), _house.value.value_or(0), mm6::merchantPercent(pParty->activeCharacter()));
}

int GUIWindow_MM6Transport::travelDays(const mm6::TransportRoute &route) const {
    int days = route.duration;
    if (mm6::isTransportStable(std::to_underlying(houseId()))) {
        if (CheckHiredNPCSpeciality(NPC_PROFESSION_HORSEMAN))
            days -= 2;
    } else {
        if (CheckHiredNPCSpeciality(NPC_PROFESSION_SAILOR))
            days -= 2;
        if (CheckHiredNPCSpeciality(NPC_PROFESSION_NAVIGATOR))
            days -= 3;
        if (CheckHiredNPCSpeciality(NPC_PROFESSION_PIRATE))
            days -= 2;
    }
    if (CheckHiredNPCSpeciality(NPC_PROFESSION_EXPLORER))
        days -= 1;
    return std::max(days, 1);
}

bool GUIWindow_MM6Transport::offersToFixPrices() const {
    return std::ranges::contains(_services, mm6::HouseServiceOption::FixPrices) && pParty->_questBits[QBIT_MM6_PRICE_FIXING_TOPIC] &&
           !pParty->_questBits[stablePricesFixedBit()];
}

void GUIWindow_MM6Transport::fixPrices() {
    pParty->_questBits.set(stablePricesFixedBit());
    current_npc_text = pNPCTopics[135].pText; // Text 136 of npctext.txt.
    const bool allFixed = std::ranges::all_of(std::views::iota(48, 57), [](int stable) {
        return pParty->_questBits[static_cast<QuestBit>(stable + 99)];
    });
    if (allFixed)
        pParty->_questBits.set(QBIT_MM6_ALL_STABLE_PRICES_FIXED);
}

QuestBit GUIWindow_MM6Transport::stablePricesFixedBit() const {
    return static_cast<QuestBit>(std::to_underlying(houseId()) + 99);
}

void GUIWindow_MM6Transport::depart() {
    _departureTime.reset();
    _departed = true;
    const Vec3f arrival(_route->position[0], _route->position[1], _route->position[2]);
    const MapId destination = pMapTable->GetMM6TransportMapInfo(*pGames_LOD, _route->mapIndex);
    if (destination != engine->_currentLoadedMapId) {
        startMapTransition(MapDestination(destination, PartyPlacement(arrival, _route->yaw, 0, 0)));
    } else {
        pParty->pos = arrival;
        pParty->uFallStartZ = arrival.z;
        pParty->_viewYaw = _route->yaw;
        pParty->_viewPitch = 0;
    }
    if (_paysOnDeparture)
        payForTrip(); // After the autosave of the map change.
    engine->_messageQueue->addMessageCurrentFrame(UIMSG_Escape, 0, 0);
}

void GUIWindow_MM6Transport::payForTrip() {
    pParty->TakeGold(_fare);
    restAndHeal(Duration::fromDays(_tripDays));
}
