#pragma once

#include <string>
#include <memory>
#include <optional>

#include "Library/Vid/VidReader.h"
#include "Library/Geometry/Size.h"

#include "Media/Movie.h"

// MOVIE_3DOLogo  "3dologo"
// MOVIE_NWCLogo  "new world logo"
// MOVIE_JVC      "jvc"
// MOVIE_Intro    "Intro"
// MOVIE_Emerald  "Intro Post"
// MOVIE_Death    "losegame"
// MOVIE_Outro    "end_seq1"

class VideoList;
class FFmpegLogProxy;

class MPlayer {
 public:
    MPlayer();
    virtual ~MPlayer();

    void Initialize();
    void Unload();

    // TODO(Gerark) Remove this method once we move all the Video to be played in the Fsm
    void PlayFullscreenMovie(std::string_view pMovieName);

    void OpenHouseMovie(std::string_view pMovieName, bool bLoop);
    bool tryOpenHouseMovie(std::string_view movieName, bool loop);
    // Resolves the active title's movie bytes without starting playback.
    bool hasHouseMoviePayload(std::string_view movieName);
    Blob houseMoviePayload(std::string_view movieName);
    void HouseMovieLoop();

    bool IsMoviePlaying() const;
    bool StopMovie();

    std::unique_ptr<IMovie> loadFullScreenMovie(std::string_view pFilename);

 protected:
    std::unique_ptr<FFmpegLogProxy> logProxy;
    VidReader might_list;
    VidReader magic_list;
    std::string sInHouseMovie;

    Blob LoadMovie(std::string_view video_name);
};

extern MPlayer *pMediaPlayer;
extern PMovie pMovie_Track;

namespace mm6 {

struct RoomMovieFrameProbe {
    Sizei size;
    size_t decodedBytes = 0;
};

// Decodes one frame through the existing FFmpeg/Smacker video path without
// opening audio, creating a window or starting the game loop.
std::optional<RoomMovieFrameProbe> probeRoomMovieFirstFrame(const Blob &movie);

} // namespace mm6
