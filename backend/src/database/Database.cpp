#include "database/Database.h"
#include <sqlite3.h>
#include <iostream>

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
    const char* sql = "SELECT * FROM songs ORDER BY id DESC LIMIT ? OFFSET ?";
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
        ORDER BY id DESC LIMIT ? OFFSET ?
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

} // namespace Music
