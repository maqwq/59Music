#pragma once

#include <string>
#include <vector>
#include "models/SongInfo.h"

struct sqlite3;

namespace Music {

struct PlaylistInfo {
    int id = 0;
    std::string name;
    int songCount = 0;
    int64_t createdAt = 0;
    int64_t updatedAt = 0;
};

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
    bool updateSongMeta(int id, const std::string& title, const std::string& artist);

    // 排序
    bool reorderSongs(int from, int to);

    // 统计
    int getTotalSongs();
    int getTotalDuration();
    int getTotalArtists();
    int getTotalAlbums();

    // 歌单
    int  createPlaylist(const std::string& name);
    bool deletePlaylist(int id);
    bool renamePlaylist(int id, const std::string& name);
    std::vector<PlaylistInfo> getAllPlaylists();
    PlaylistInfo getPlaylistById(int id);
    std::vector<SongInfo> getPlaylistSongs(int playlistId);
    int  addSongsToPlaylist(int playlistId, const std::vector<int>& songIds);
    bool removeSongFromPlaylist(int playlistId, int songId);
    bool reorderPlaylistSongs(int playlistId, int from, int to);
    std::vector<SongInfo> getRandomSongs(int count);

private:
    void createTables();
    bool executeSql(const std::string& sql);

    std::string dbPath_;
    sqlite3* db_ = nullptr;
};

} // namespace Music
