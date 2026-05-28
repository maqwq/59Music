#include "LibraryManager.h"
#include <iostream>

namespace Music {

LibraryManager::LibraryManager(std::shared_ptr<Database> db)
    : db_(db) {}

int LibraryManager::scanAndImport(const std::string& folderPath) {
    std::vector<std::string> audioFiles = scanner_.scanFolder(folderPath);
    int addedCount = 0;

    for (const auto& filePath : audioFiles) {
        // 去重：检查是否已存在
        SongInfo existing = db_->getSongByPath(filePath);
        if (existing.id > 0) {
            continue;
        }

        // 解析元数据
        SongInfo song = parser_.parseFile(filePath);

        // 入库
        if (db_->insertSong(song)) {
            addedCount++;
        }
    }

    return addedCount;
}

std::vector<SongInfo> LibraryManager::getSongsByPage(int page, int size, int& total) {
    return db_->getSongsByPage(page, size, total);
}

std::vector<SongInfo> LibraryManager::searchSongs(const std::string& keyword, int page, int size, int& total) {
    return db_->searchSongs(keyword, page, size, total);
}

bool LibraryManager::deleteSong(int id) {
    return db_->deleteSong(id);
}

SongInfo LibraryManager::getSongById(int id) {
    return db_->getSongById(id);
}

int LibraryManager::getTotalSongs() {
    return db_->getTotalSongs();
}

int LibraryManager::getTotalDuration() {
    return db_->getTotalDuration();
}

int LibraryManager::getTotalArtists() {
    return db_->getTotalArtists();
}

int LibraryManager::getTotalAlbums() {
    return db_->getTotalAlbums();
}

} // namespace Music