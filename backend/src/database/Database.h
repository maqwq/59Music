#pragma once

#include <string>
#include <vector>
#include "models/SongInfo.h"

struct sqlite3;

namespace Music {

class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    // 连接管理
    bool open();
    void close();
    bool isOpen() const;

    // 歌曲 CRUD
    bool insertSong(const SongInfo& song);
    SongInfo getSongByPath(const std::string& filePath);
    SongInfo getSongById(int id);
    std::vector<SongInfo> getSongsByPage(int page, int size, int& total);
    std::vector<SongInfo> searchSongs(const std::string& keyword, int page, int size, int& total);
    bool deleteSong(int id);

    // 统计
    int getTotalSongs();
    int getTotalDuration();
    int getTotalArtists();
    int getTotalAlbums();

private:
    void createTables();
    bool executeSql(const std::string& sql);

    std::string dbPath_;
    sqlite3* db_ = nullptr;
};

} // namespace Music
