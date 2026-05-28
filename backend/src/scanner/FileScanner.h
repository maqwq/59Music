#pragma once

#include <string>
#include <vector>
#include <set>

namespace Music {

class FileScanner {
public:
    FileScanner();

    // 扫描文件夹，返回找到的音频文件路径列表
    std::vector<std::string> scanFolder(const std::string& folderPath);

    // 检查文件是否为支持的音频格式
    bool isSupportedAudioFile(const std::string& filePath) const;

private:
    std::set<std::string> supportedExtensions_;

    void initSupportedExtensions();
    std::string getFileExtension(const std::string& filePath) const;
};

} // namespace Music