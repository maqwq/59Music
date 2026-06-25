#pragma once

#include <string>
#include <vector>
#include <memory>
#include "scanner/FileScanner.h"
#include "scanner/MetadataParser.h"
#include "database/Database.h"

namespace Music {

class LibraryManager {
public:
    LibraryManager(std::shared_ptr<Database> db);

    // 扫描文件夹并入库，返回新增歌曲数量
    int scanAndImport(const std::string& folderPath);

    // 分页查询
    std::vector<SongInfo> getSongsByPage(int page, int size, int& total);

    // 关键词搜索
    std::vector<SongInfo> searchSongs(const std::string& keyword, int page, int size, int& total);

    // 删除歌曲（不删除原文件）
    bool deleteSong(int id);

    // 调整歌曲顺序
    bool reorderSongs(int from, int to);

    // 获取单首歌曲
    SongInfo getSongById(int id);

    // 统计信息
    int getTotalSongs();
    int getTotalDuration();
    int getTotalArtists();
    int getTotalAlbums();

private:
    std::shared_ptr<Database> db_;
    FileScanner scanner_;
    MetadataParser parser_;
};

} // namespace Music