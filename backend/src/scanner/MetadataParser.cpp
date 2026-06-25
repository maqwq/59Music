#include "MetadataParser.h"
#include <fileref.h>
#include <tag.h>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
static std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    return wide;
}
static std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}
#endif

namespace fs = std::filesystem;

namespace Music {

MetadataParser::MetadataParser() {}

std::string MetadataParser::getFileTitle(const std::string& filePath) const {
#ifdef _WIN32
    fs::path path(utf8ToWide(filePath));
    return wideToUtf8(path.stem().wstring());
#else
    fs::path path(filePath);
    return path.stem().string();
#endif
}

SongInfo MetadataParser::parseFile(const std::string& filePath) {
    SongInfo song;
    song.filePath = filePath;
    song.addedTime = static_cast<int64_t>(std::time(nullptr));

    // 使用TagLib读取元数据
#ifdef _WIN32
    TagLib::FileRef file(utf8ToWide(filePath).c_str());
#else
    TagLib::FileRef file(filePath.c_str());
#endif

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