#include "database/Database.h"
#include <sqlite3.h>
#include <iostream>
#include <algorithm>
#include <random>

namespace Music {

Database::Database(const std::string& dbPath)
    : dbPath_(dbPath) {}

Database::~Database() {
    close();
}

bool Database::open() {
    if (db_) return true;

    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db_) << std::endl;
        close();
        return false;
    }

    // 启用 WAL 模式，提高并发读取性能
    executeSql("PRAGMA journal_mode=WAL");
    executeSql("PRAGMA foreign_keys=ON");

    createTables();
    return true;
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::isOpen() const {
    return db_ != nullptr;
}

void Database::createTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS songs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path   TEXT NOT NULL UNIQUE,
            title       TEXT NOT NULL DEFAULT '',
            artist      TEXT NOT NULL DEFAULT '',
            album       TEXT NOT NULL DEFAULT '',
            duration    INTEGER NOT NULL DEFAULT 0,
            added_time  INTEGER NOT NULL DEFAULT 0
        );

        CREATE INDEX IF NOT EXISTS idx_songs_title   ON songs(title);
        CREATE INDEX IF NOT EXISTS idx_songs_artist  ON songs(artist);
        CREATE INDEX IF NOT EXISTS idx_songs_album   ON songs(album);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "建表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    // 迁移：添加 sort_order 列（忽略"列已存在"错误）
    sqlite3_exec(db_,
        "ALTER TABLE songs ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0",
        nullptr, nullptr, nullptr);

    // 歌单表
    sqlite3_exec(db_, R"(
        CREATE TABLE IF NOT EXISTS playlists (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL DEFAULT '新建歌单',
            created_at  INTEGER NOT NULL DEFAULT 0,
            updated_at  INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS playlist_songs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            playlist_id INTEGER NOT NULL,
            song_id     INTEGER NOT NULL,
            sort_order  INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE,
            UNIQUE(playlist_id, song_id)
        );
    )", nullptr, nullptr, nullptr);
}

bool Database::executeSql(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL 执行失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// ============ Song CRUD ============

bool Database::insertSong(const SongInfo& song) {
    const char* sql = R"(
        INSERT OR IGNORE INTO songs (file_path, title, artist, album, duration, added_time)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, song.filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, song.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, song.artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, song.album.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  5, song.duration);
    sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(song.addedTime));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

static SongInfo songFromRow(sqlite3_stmt* stmt) {
    SongInfo s;
    s.id        = sqlite3_column_int(stmt, 0);
    s.filePath  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    s.title     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    s.artist    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    s.album     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    s.duration  = sqlite3_column_int(stmt, 5);
    s.addedTime = sqlite3_column_int64(stmt, 6);
    return s;
}

SongInfo Database::getSongByPath(const std::string& filePath) {
    const char* sql = "SELECT * FROM songs WHERE file_path = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);

    SongInfo song;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        song = songFromRow(stmt);
    }
    sqlite3_finalize(stmt);
    return song;  // id == 0 表示未找到
}

SongInfo Database::getSongById(int id) {
    const char* sql = "SELECT * FROM songs WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    SongInfo song;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        song = songFromRow(stmt);
    }
    sqlite3_finalize(stmt);
    return song;
}

std::vector<SongInfo> Database::getSongsByPage(int page, int size, int& total) {
    // 先查总数
    {
        const char* sql = "SELECT COUNT(*) FROM songs";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // 分页查询
    std::vector<SongInfo> results;
    const char* sql = "SELECT * FROM songs ORDER BY sort_order, id DESC LIMIT ? OFFSET ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, size);
    sqlite3_bind_int(stmt, 2, (page - 1) * size);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(songFromRow(stmt));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<SongInfo> Database::searchSongs(const std::string& keyword, int page, int size, int& total) {
    std::string like = "%" + keyword + "%";

    // 先查匹配总数
    {
        const char* sql = R"(
            SELECT COUNT(*) FROM songs
            WHERE title LIKE ? OR artist LIKE ? OR album LIKE ?
        )";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, like.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, like.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // 分页搜索
    std::vector<SongInfo> results;
    const char* sql = R"(
        SELECT * FROM songs
        WHERE title LIKE ? OR artist LIKE ? OR album LIKE ?
        ORDER BY sort_order, id DESC LIMIT ? OFFSET ?
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, like.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, like.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, size);
    sqlite3_bind_int(stmt, 5, (page - 1) * size);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(songFromRow(stmt));
    }
    sqlite3_finalize(stmt);
    return results;
}

bool Database::deleteSong(int id) {
    const char* sql = "DELETE FROM songs WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
}

bool Database::updateSongMeta(int id, const std::string& title, const std::string& artist) {
    const char* sql = "UPDATE songs SET title = ?, artist = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
}

// ============ 排序 ============

bool Database::reorderSongs(int from, int to) {
    // 1. 获取当前排序下的所有歌曲 ID
    std::vector<int> ids;
    const char* query = "SELECT id FROM songs ORDER BY sort_order, id DESC";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    int n = static_cast<int>(ids.size());
    if (from < 0 || from >= n || to < 0 || to >= n) {
        return false;
    }

    // 2. 移动元素
    int moved = ids[from];
    ids.erase(ids.begin() + from);
    ids.insert(ids.begin() + to, moved);

    // 3. 批量更新 sort_order
    executeSql("BEGIN TRANSACTION");
    const char* update = "UPDATE songs SET sort_order = ? WHERE id = ?";
    for (int i = 0; i < n; ++i) {
        sqlite3_prepare_v2(db_, update, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, i);
        sqlite3_bind_int(stmt, 2, ids[i]);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    executeSql("COMMIT");

    return true;
}

// ============ 统计 ============

int Database::getTotalSongs() {
    const char* sql = "SELECT COUNT(*) FROM songs";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalDuration() {
    const char* sql = "SELECT COALESCE(SUM(duration), 0) FROM songs";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return total;
}

int Database::getTotalArtists() {
    const char* sql = "SELECT COUNT(DISTINCT artist) FROM songs WHERE artist != ''";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalAlbums() {
    const char* sql = "SELECT COUNT(DISTINCT album) FROM songs WHERE album != ''";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

// ============ 歌单 ============

int Database::createPlaylist(const std::string& name) {
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    const char* sql = "INSERT INTO playlists (name, created_at, updated_at) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::deletePlaylist(int id) {
    const char* sql = "DELETE FROM playlists WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
}

bool Database::renamePlaylist(int id, const std::string& name) {
    const char* sql = "UPDATE playlists SET name = ?, updated_at = ? WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(std::time(nullptr)));
    sqlite3_bind_int(stmt, 3, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
}

std::vector<PlaylistInfo> Database::getAllPlaylists() {
    std::vector<PlaylistInfo> result;
    const char* sql = R"(
        SELECT p.id, p.name, COUNT(ps.id) as song_count, p.created_at, p.updated_at
        FROM playlists p
        LEFT JOIN playlist_songs ps ON p.id = ps.playlist_id
        GROUP BY p.id
        ORDER BY p.updated_at DESC
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PlaylistInfo info;
        info.id        = sqlite3_column_int(stmt, 0);
        info.name      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.songCount = sqlite3_column_int(stmt, 2);
        info.createdAt = sqlite3_column_int64(stmt, 3);
        info.updatedAt = sqlite3_column_int64(stmt, 4);
        result.push_back(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

PlaylistInfo Database::getPlaylistById(int id) {
    PlaylistInfo info;
    const char* sql = R"(
        SELECT p.id, p.name, COUNT(ps.id) as song_count, p.created_at, p.updated_at
        FROM playlists p
        LEFT JOIN playlist_songs ps ON p.id = ps.playlist_id
        WHERE p.id = ?
        GROUP BY p.id
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.id        = sqlite3_column_int(stmt, 0);
        info.name      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.songCount = sqlite3_column_int(stmt, 2);
        info.createdAt = sqlite3_column_int64(stmt, 3);
        info.updatedAt = sqlite3_column_int64(stmt, 4);
    }
    sqlite3_finalize(stmt);
    return info;
}

std::vector<SongInfo> Database::getPlaylistSongs(int playlistId) {
    std::vector<SongInfo> result;
    const char* sql = R"(
        SELECT s.id, s.file_path, s.title, s.artist, s.album, s.duration, s.added_time
        FROM playlist_songs ps
        JOIN songs s ON ps.song_id = s.id
        WHERE ps.playlist_id = ?
        ORDER BY ps.sort_order, ps.id
    )";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, playlistId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(songFromRow(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

int Database::addSongsToPlaylist(int playlistId, const std::vector<int>& songIds) {
    int added = 0;
    const char* sql = "INSERT OR IGNORE INTO playlist_songs (playlist_id, song_id) VALUES (?, ?)";
    for (int songId : songIds) {
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, playlistId);
        sqlite3_bind_int(stmt, 2, songId);
        if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0) {
            added++;
        }
        sqlite3_finalize(stmt);
    }
    // 更新时间戳
    if (added > 0) {
        const char* upd = "UPDATE playlists SET updated_at = ? WHERE id = ?";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db_, upd, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(std::time(nullptr)));
        sqlite3_bind_int(stmt, 2, playlistId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return added;
}

bool Database::removeSongFromPlaylist(int playlistId, int songId) {
    const char* sql = "DELETE FROM playlist_songs WHERE playlist_id = ? AND song_id = ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, playlistId);
    sqlite3_bind_int(stmt, 2, songId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
}

bool Database::reorderPlaylistSongs(int playlistId, int from, int to) {
    // 获取当前排序下的歌曲 ID
    std::vector<int> ids;
    const char* query = "SELECT song_id FROM playlist_songs WHERE playlist_id = ? ORDER BY sort_order, id";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, playlistId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    int n = static_cast<int>(ids.size());
    if (from < 0 || from >= n || to < 0 || to >= n) return false;

    int moved = ids[from];
    ids.erase(ids.begin() + from);
    ids.insert(ids.begin() + to, moved);

    executeSql("BEGIN TRANSACTION");
    const char* update = "UPDATE playlist_songs SET sort_order = ? WHERE playlist_id = ? AND song_id = ?";
    for (int i = 0; i < n; ++i) {
        sqlite3_prepare_v2(db_, update, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, i);
        sqlite3_bind_int(stmt, 2, playlistId);
        sqlite3_bind_int(stmt, 3, ids[i]);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    executeSql("COMMIT");
    return true;
}

std::vector<SongInfo> Database::getRandomSongs(int count) {
    std::vector<SongInfo> result;
    const char* sql = "SELECT * FROM songs ORDER BY RANDOM() LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, count);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.push_back(songFromRow(stmt));
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace Music
