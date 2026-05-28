#pragma once

#include <string>
#include "models/SongInfo.h"

namespace Music {

class MetadataParser {
public:
    MetadataParser();

    // 解析音频文件元数据
    SongInfo parseFile(const std::string& filePath);

private:
    std::string getFileTitle(const std::string& filePath) const;
};

} // namespace Music