#include "FileScanner.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

namespace Music {

FileScanner::FileScanner() {
    initSupportedExtensions();
}

void FileScanner::initSupportedExtensions() {
    supportedExtensions_ = {".mp3", ".flac", ".wav"};
}

std::string FileScanner::getFileExtension(const std::string& filePath) const {
    fs::path path(filePath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool FileScanner::isSupportedAudioFile(const std::string& filePath) const {
    std::string ext = getFileExtension(filePath);
    return supportedExtensions_.count(ext) > 0;
}

std::vector<std::string> FileScanner::scanFolder(const std::string& folderPath) {
    std::vector<std::string> results;

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

    return results;
}

} // namespace Music