#include "MetadataParser.h"
#include <fileref.h>
#include <tag.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace Music {

MetadataParser::MetadataParser() {}

std::string MetadataParser::getFileTitle(const std::string& filePath) const {
    fs::path path(filePath);
    return path.stem().string();
}

SongInfo MetadataParser::parseFile(const std::string& filePath) {
    SongInfo song;
    song.filePath = filePath;
    song.addedTime = static_cast<int64_t>(std::time(nullptr));

    // 使用TagLib读取元数据
    TagLib::FileRef file(filePath.c_str());

    if (!file.isNull() && file.tag()) {
        TagLib::Tag* tag = file.tag();
        song.title = tag->title().toCString(true);
        song.artist = tag->artist().toCString(true);
        song.album = tag->album().toCString(true);

        if (file.audioProperties()) {
            song.duration = file.audioProperties()->lengthInSeconds();
        }
    }

    // 如果标题为空，使用文件名作为标题
    if (song.title.empty()) {
        song.title = getFileTitle(filePath);
    }

    return song;
}

} // namespace Music