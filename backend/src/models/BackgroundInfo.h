#pragma once

#include <string>
#include <cstdint>

namespace Music {

struct BackgroundInfo {
    int         id        = 0;
    std::string filePath;
    std::string name;
    std::string thumbnail;
    int64_t     addedTime = 0;
    bool        isDefault = false;
};

} // namespace Music