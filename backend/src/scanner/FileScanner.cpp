#include "FileScanner.h"
#include <filesystem>
#include <algorithm>
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

FileScanner::FileScanner() {
    initSupportedExtensions();
}

void FileScanner::initSupportedExtensions() {
    supportedExtensions_ = {".mp3", ".flac", ".wav"};
}

std::string FileScanner::getFileExtension(const std::string& filePath) const {
#ifdef _WIN32
    fs::path path(utf8ToWide(filePath));
    std::string ext = wideToUtf8(path.extension().wstring());
#else
    fs::path path(filePath);
    std::string ext = path.extension().string();
#endif
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool FileScanner::isSupportedAudioFile(const std::string& filePath) const {
    std::string ext = getFileExtension(filePath);
    return supportedExtensions_.count(ext) > 0;
}

std::vector<std::string> FileScanner::scanFolder(const std::string& folderPath) {
    std::vector<std::string> results;

#ifdef _WIN32
    std::wstring wFolder = utf8ToWide(folderPath);
    std::error_code ec;
    if (!fs::exists(wFolder, ec) || !fs::is_directory(wFolder, ec)) {
        std::cerr << "文件夹不存在或不是目录: " << folderPath << std::endl;
        return results;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(wFolder)) {
            if (entry.is_regular_file()) {
                std::string filePath = wideToUtf8(entry.path().wstring());
                if (isSupportedAudioFile(filePath)) {
                    results.push_back(filePath);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "扫描文件夹出错: " << e.what() << std::endl;
    }
#else
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        std::cerr << "文件夹不存在或不是目录: " << folderPath << std::endl;
        return results;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();
                if (isSupportedAudioFile(filePath)) {
                    results.push_back(filePath);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "扫描文件夹出错: " << e.what() << std::endl;
    }
#endif

    return results;
}

} // namespace Music