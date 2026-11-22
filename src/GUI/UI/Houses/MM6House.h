#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Data/SoundEnums.h"

#include "GUI/UI/UIHouseEnums.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/MM6HouseServiceData.h"

#include "Library/Color/Color.h"

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
     * The NPC the party is talking to tells what joining a guild costs, and offers to join.
     *
     * @param guild                     Guild of the NPC's membership topic, 0 for the Elemental Guild through 16 for
     *                                  the Dark Guild, in npctopic.txt order.
     */
    void offerGuildMembership(int guild);

    /**
     * Takes the offered membership: the party pays, every character joins the guild with the award chime, and the NPC
     * drops its third topic. Does nothing when the active character can't act, is a member already, or the party
     * can't pay.
     *
     * @offset 0x496A96 (MM6)
     */
    void joinGuild();

    /**
     * Shows a teacher's offer with its one learn line in place of the NPC's topics.
     *
     * @offset 0x499610 (MM6)
     *
     * @param topic                     NPC topic of the teacher, from 200 to 259. An even one teaches Expert, an odd one
     *                                  Master.
     */
    void offerTeacherTraining(int topic);

    /**
     * Makes the active character an expert or a master in the skill on offer when the teacher's checks pass and the
     * party has the gold.
     *
     * @offset 0x4969C4 (MM6)
     */
    void learnFromTeacher();

    /**
     * Shows the town hall's bounty for this month in place of the NPC's topics, rolling a new monster when the month
     * has turned. When the party has killed the monster since, pays 100 gold per monster level, gives every character
     * the bounties collected award and adds the level to the reputation.
     *
     * @offset 0x4A30B0 (MM6)
     */
    void huntBounty();

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

    /**
     * Places the option buttons the way MM6 spaces them on the right panel, and draws their lines.
     *
     * @param labels                    Line of each option button, empty to hide the button.
     */
    void drawOptionLabels(const std::vector<std::string> &labels);

    /**
     * @param index                     Index into the proprietor's service list.
     * @return                          Line the service shows on the menu, empty to hide it.
     */
    virtual std::string serviceLabel(int index) const;

    /**
     * @return                          Top of the first line of a menu that stacks its lines with a blank line between
     *                                  them, the way a tavern or a stable lists its services.
     */
    virtual int stackedMenuTop() const;

    /**
     * @return                          Whether the proprietor's service menu is up and this house hides it from an
     *                                  active character who can't act.
     */
    virtual bool menuNeedsActingCharacter() const;

    /**
     * @return                          Whether the service menu is hidden behind the no condition line because the
     *                                  active character can't act.
     */
    bool turnsAwayActiveCharacter() const;

    /**
     * @param text                      Text drawn in the right panel.
     * @return                          Height of the text as MM6 counts it, the font height minus 3 for every line.
     */
    static int panelTextHeight(std::string_view text);

    /**
     * Draws text in the right panel's text column, centred in it.
     *
     * @param text                      Text to draw, in the Arrus font.
     * @param y                         Top of the first line.
     * @param color                     Text color.
     */
    static void drawPanelText(std::string_view text, int y, Color color);

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

    /**
     * @return                          Color of the option line the mouse last pointed at.
     */
    Color optionFocusColor() const;

    /**
     * @return                          Line of the offered membership's button: its price, or why the active character
     *                                  can't join.
     */
    std::string membershipLabel() const;

    /**
     * @offset 0x496C90 (MM6)
     *
     * @param[out] approved             Whether the active character may learn from the teacher on offer.
     * @return                          Line of the learn button, or why the character can't learn.
     */
    std::string teacherLabel(bool *approved) const;

    void showResidentButtons();
    void showExitButtons();
    void drawResidents();
    void drawExit();
    void healActiveCharacter();

    /**
     * Takes the temple's `Val` in gold and raises the party's reputation by 200, to no more than 200. Temple Baa takes
     * the 200 back while the reputation is above -800.
     *
     * @offset 0x49DD99 (MM6)
     */
    void donate();

    /**
     * Takes the room price, then leaves the house and puts the party to bed until 6 am.
     *
     * @offset 0x49F319 (MM6)
     */
    void rentRoom();

    /**
     * Takes the food price and fills the party's food up to the tavern's `Val`, unless the packs already hold that
     * much.
     *
     * @offset 0x49F85F (MM6)
     */
    void fillPacks();

    /**
     * Sells the active character a drink for 1 gold. Half the drinks get the got drunk reaction, and a third of those
     * leave the character drunk. A quarter of the others add 5 to 10 to the bonus of a random stat.
     *
     * @offset 0x49F45C (MM6)
     */
    void haveDrink();

    /**
     * Takes 1 gold for the barkeep, who tells a piece of the region's news. The party has to have had a drink at the
     * tavern first, and each tavern keeps telling the news it picked the first time.
     *
     * @offset 0x49F716 (MM6)
     */
    void tipBarkeep();

    /**
     * Shows the bank's how much prompt while the player types an amount, and moves the gold once the amount is
     * confirmed. An amount above what the party has, or holds in the bank, moves all of it.
     */
    void updateBankTransfer();

    GraphicsImage *_panel = nullptr;
    std::vector<Resident> _residents;
    std::vector<GUIButton *> _optionButtons;
    int _focusedOption = 0; // Index into `_optionButtons` of the line the mouse last pointed at, the first one at the start.
    int _selected = -1;
    std::optional<int> _membershipOffer; // Guild the selected NPC offers to join, see `offerGuildMembership`.
    std::optional<int> _teacherOffer; // Topic of the teacher whose offer is shown.
    bool _bountyShown = false; // The NPC tells the town hall's bounty, see `huntBounty`.
    bool _trainingTimePassed = false;
    std::optional<mm6::HouseServiceOption> _bankTransfer; // Deposit or withdraw while the player types the amount.
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
 * @return                              The sound, or `SOUND_Invalid` for a room without a sound group.
 */
SoundId mm6PlayHouseSound(HouseId houseId, HouseSoundType type);
