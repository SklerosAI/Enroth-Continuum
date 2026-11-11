#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "GUI/UI/UIHouseEnums.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/MM6HouseServiceData.h"

class GraphicsImage;
class GUIButton;
struct NPCData;

/**
 * MM6 house screen. Lists the people in the house and the house's exit to another map. When one of the people is
 * selected it shows the proprietor's services or the NPC's topics, and when the exit is selected it asks whether to
 * take it.
 */
class GUIWindow_MM6House : public GUIWindow_House {
 public:
    GUIWindow_MM6House(HouseId houseId, const mm6::HousePresentationState &presentation);

    virtual void Update() override;

    /**
     * @param index                     Index of the person in the house to talk to.
     */
    void selectResident(int index);

    /**
     * @param index                     Index into the proprietor's service list.
     */
    virtual void selectService(int index);

    /**
     * Rebuilds the topic buttons of the NPC the party is talking to. A topic's event can change them.
     */
    void refreshTopics();

    /**
     * @return                          NPC the party is talking to, or `nullptr` if it isn't talking to an NPC.
     */
    NPCData *selectedNpc() const;

    /**
     * Backs out of the conversation the way escape does.
     *
     * @return                          Whether the party stays in the house.
     */
    virtual bool goBack();

    /**
     * Takes the selected exit: autosaves, leaves the house and starts the trip to the exit's map.
     */
    void confirmExit();

    /**
     * Lets a looping room movie finish its current pass instead of starting another.
     */
    void stopRoomMovieLoop();

    /**
     * @return                          House the party moves on to now that the room movie has played out, empty while
     *                                  it plays or for a room that stays.
     */
    std::optional<HouseId> roomAfterMovie() const;

 protected:
    bool isProprietorSelected() const;
    void clearOptionButtons();
    void showServiceButtons();
    void drawOptionLabels(const std::vector<std::string> &labels);

    /**
     * @param index                     Index into the proprietor's service list.
     * @return                          Line the service shows on the menu, empty to hide it.
     */
    virtual std::string serviceLabel(int index) const;

    const mm6::HouseServiceRow &_house;
    std::vector<mm6::HouseServiceOption> _services;

 private:
    struct Resident {
        NPCData *npc = nullptr; // `nullptr` for the proprietor and the exit.
        GraphicsImage *icon = nullptr;
        std::string name;
        std::string title;
        bool exit = false;
    };

    bool isExitSelected() const;
    void showResidentButtons();
    void showExitButtons();
    void drawExit();
    void healActiveCharacter();

    GraphicsImage *_panel = nullptr;
    GraphicsImage *_exit = nullptr;
    std::vector<Resident> _residents;
    std::vector<GUIButton *> _optionButtons;
    int _selected = -1;
    bool _trainingTimePassed = false;
    std::string _roomMovie;
    int64_t _roomMovieStart = 0; // Platform tick count at which the room movie started.
    std::optional<int64_t> _roomMovieEnd; // Platform tick count at which the room movie stops, empty while it loops.
};

/**
 * MM6 counterpart of `enterHouse`. Refuses with a status line, without touching the party or the UI, when the house
 * has no room, its room media can't be loaded, or it would show nothing at all: no movie, no proprietor and no NPC.
 *
 * @param houseId                       MM6 house id from a `SpeakInHouse` event.
 * @return                              Whether the house was entered, and `mm6CreateHouseUI` should follow.
 */
bool mm6EnterHouse(HouseId houseId);

/**
 * MM6 counterpart of `createHouseUI`.
 *
 * @param houseId                       House that `mm6EnterHouse` accepted.
 */
void mm6CreateHouseUI(HouseId houseId);

/**
 * Runs the event behind one of the selected NPC's topics, then rebuilds the topic list if the party is still in the
 * house.
 *
 * @param topic                         Topic the player clicked.
 */
void mm6SelectHouseTopic(DialogueId topic);

/**
 * @return                              The MM6 house window, or `nullptr` if the party isn't in an MM6 house.
 */
GUIWindow_MM6House *mm6HouseWindow();

/**
 * Called once a frame. Once the room movie of the house the party is in has played out, the party moves on to the
 * house that follows it.
 */
void mm6UpdateHouseRoom();

/**
 * Called after a script moved an NPC. In the room where Archibald waits it lets the room movie play out.
 */
void mm6OnNpcMoved();

/**
 * Plays one of the room's proprietor sounds. Rooms without a sound group stay silent.
 *
 * @param houseId                       MM6 house id.
 * @param type                          Which of the room's sounds to play.
 */
void mm6PlayHouseSound(HouseId houseId, HouseSoundType type);
