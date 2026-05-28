#pragma once

#include <string>
#include <ctime>

namespace Music {

struct SongInfo {
    int id = 0;
    std::string filePath;
    std::string title;
    std::string artist;
    std::string album;
    int duration = 0;
    time_t addedTime = 0;
};

} // namespace Music