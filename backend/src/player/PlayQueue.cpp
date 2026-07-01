#include "PlayQueue.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <stdexcept>

using Music::SongInfo;

// ============================================================
// 随机数生成器
// ============================================================

static std::mt19937& rng() {
    static std::mt19937 engine(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    ));
    return engine;
}

// ============================================================
// 内部辅助
// ============================================================

SongInfo PlayQueue::resolveCurrent() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(items_.size())) {
        return SongInfo{};
    }
    const auto& item = items_[currentIndex_];
    if (item.type == QueueItemType::Song) {
        if (songResolver_) return songResolver_(item.songId);
        return SongInfo{};
    }
    // Playlist item
    if (!playlistResolver_) return SongInfo{};
    auto songs = playlistResolver_(item.playlistId);
    if (playlistPos_ >= 0 && playlistPos_ < static_cast<int>(songs.size())) {
        return songs[playlistPos_];
    }
    return SongInfo{};
}

std::vector<SongInfo> PlayQueue::getCurrentPlaylistSongs() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(items_.size())) return {};
    const auto& item = items_[currentIndex_];
    if (item.type != QueueItemType::Playlist || !playlistResolver_) return {};
    return playlistResolver_(item.playlistId);
}

int PlayQueue::expandedSize() const {
    int count = 0;
    for (const auto& item : items_) {
        if (item.type == QueueItemType::Song) {
            count++;
        } else if (playlistResolver_) {
            count += static_cast<int>(playlistResolver_(item.playlistId).size());
        }
    }
    return count;
}

// ============================================================
// 队列管理
// ============================================================

int PlayQueue::addSongs(const std::vector<int>& songIds) {
    int added = 0;
    for (int sid : songIds) {
        // 去重：检查 Song 类型项中是否已存在
        bool duplicate = false;
        for (const auto& item : items_) {
            if (item.type == QueueItemType::Song && item.songId == sid) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        QueueItem qi;
        qi.type = QueueItemType::Song;
        qi.songId = sid;
        items_.push_back(qi);
        added++;
    }

    if (currentIndex_ < 0 && !items_.empty()) {
        currentIndex_ = 0;
        playlistPos_ = 0;
    }

    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
    return added;
}

void PlayQueue::addPlaylist(int playlistId, const std::string& name) {
    QueueItem qi;
    qi.type = QueueItemType::Playlist;
    qi.playlistId = playlistId;
    qi.playlistName = name;
    items_.push_back(qi);

    if (currentIndex_ < 0 && !items_.empty()) {
        currentIndex_ = 0;
        playlistPos_ = 0;
    }

    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
}

bool PlayQueue::removeAt(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) return false;

    items_.erase(items_.begin() + index);

    if (items_.empty()) {
        currentIndex_ = -1;
        playlistPos_ = -1;
        shuffleOrder_.clear();
        shufflePos_ = 0;
        return true;
    }

    if (index < currentIndex_) {
        currentIndex_--;
    } else if (index == currentIndex_) {
        // 当前项被删除
        if (currentIndex_ >= static_cast<int>(items_.size())) {
            currentIndex_ = static_cast<int>(items_.size()) - 1;
        }
        playlistPos_ = 0;  // 重置歌单内位置
    }

    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
    return true;
}

void PlayQueue::clear() {
    items_.clear();
    currentIndex_ = -1;
    playlistPos_ = -1;
    shuffleOrder_.clear();
    shufflePos_ = 0;
}

bool PlayQueue::reorder(int from, int to) {
    int n = static_cast<int>(items_.size());
    if (from < 0 || from >= n || to < 0 || to >= n || from == to) return false;

    QueueItem moved = items_[from];
    items_.erase(items_.begin() + from);
    items_.insert(items_.begin() + to, moved);

    // 调整 currentIndex_
    if (from == currentIndex_) {
        currentIndex_ = to;
    } else if (from < currentIndex_ && to >= currentIndex_) {
        currentIndex_--;
    } else if (from > currentIndex_ && to <= currentIndex_) {
        currentIndex_++;
    }

    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
    return true;
}

// ============================================================
// 播放导航
// ============================================================

std::optional<SongInfo> PlayQueue::current() {
    if (items_.empty()) return std::nullopt;
    SongInfo song = resolveCurrent();
    if (song.id == 0) return std::nullopt;
    return song;
}

std::optional<SongInfo> PlayQueue::next() {
    if (items_.empty()) return std::nullopt;

    const auto& item = items_[currentIndex_];

    // 如果当前是 Playlist，先尝试在歌单内前进
    if (item.type == QueueItemType::Playlist && playlistResolver_) {
        auto songs = playlistResolver_(item.playlistId);
        if (!songs.empty()) {
            playlistPos_++;
            if (playlistPos_ < static_cast<int>(songs.size())) {
                return songs[playlistPos_];
            }
        }
        // 歌单播完，前进到下一项
        playlistPos_ = 0;
    }

    // 前进到下一个 item
    int nextItemIdx = -1;

    switch (mode_) {
        case PlayMode::Sequential: {
            nextItemIdx = currentIndex_ + 1;
            if (nextItemIdx >= static_cast<int>(items_.size())) return std::nullopt;
            break;
        }
        case PlayMode::Reverse: {
            nextItemIdx = currentIndex_ - 1;
            if (nextItemIdx < 0) return std::nullopt;
            break;
        }
        case PlayMode::SingleLoop: {
            // 同一项：如果当前项是歌单，回到歌单第一首
            if (item.type == QueueItemType::Playlist && playlistResolver_) {
                auto songs = playlistResolver_(item.playlistId);
                if (!songs.empty()) {
                    playlistPos_ = 0;
                    return songs[0];
                }
            }
            // Song 项：返回同一首
            nextItemIdx = currentIndex_;
            break;
        }
        case PlayMode::ListLoop: {
            nextItemIdx = (currentIndex_ + 1) % static_cast<int>(items_.size());
            break;
        }
        case PlayMode::Shuffle: {
            ensureShuffleOrder();
            shufflePos_++;
            if (shufflePos_ >= static_cast<int>(shuffleOrder_.size())) {
                regenerateShuffleOrder();
                // regenerateShuffleOrder 会将 currentIndex_ 换到 shuffleOrder_[0]，
                // 避免新一轮首首歌与刚播完的歌重复：多首时从位置 1 开始，单首时保留 0。
                shufflePos_ = (static_cast<int>(shuffleOrder_.size()) > 1) ? 1 : 0;
            }
            nextItemIdx = shuffleOrder_[shufflePos_];
            break;
        }
    }

    if (nextItemIdx < 0 || nextItemIdx >= static_cast<int>(items_.size())) {
        return std::nullopt;
    }

    currentIndex_ = nextItemIdx;
    playlistPos_ = 0;

    // 解析新 item 的第一首歌
    SongInfo song = resolveCurrent();
    if (song.id == 0) return std::nullopt;
    return song;
}

std::optional<SongInfo> PlayQueue::previous() {
    if (items_.empty()) return std::nullopt;

    const auto& item = items_[currentIndex_];

    // 如果当前是 Playlist 且不在第一首，先在歌单内后退
    if (item.type == QueueItemType::Playlist && playlistPos_ > 0 && playlistResolver_) {
        auto songs = playlistResolver_(item.playlistId);
        playlistPos_--;
        if (playlistPos_ < static_cast<int>(songs.size())) {
            return songs[playlistPos_];
        }
    }

    // 后退到上一个 item
    int prevItemIdx = -1;

    switch (mode_) {
        case PlayMode::Sequential: {
            prevItemIdx = currentIndex_ - 1;
            if (prevItemIdx < 0) return std::nullopt;
            break;
        }
        case PlayMode::Reverse: {
            prevItemIdx = currentIndex_ + 1;
            if (prevItemIdx >= static_cast<int>(items_.size())) return std::nullopt;
            break;
        }
        case PlayMode::SingleLoop: {
            prevItemIdx = currentIndex_;
            break;
        }
        case PlayMode::ListLoop: {
            int sz = static_cast<int>(items_.size());
            prevItemIdx = (currentIndex_ - 1 + sz) % sz;
            break;
        }
        case PlayMode::Shuffle: {
            ensureShuffleOrder();
            if (shufflePos_ > 0) {
                shufflePos_--;
            } else {
                shufflePos_ = static_cast<int>(shuffleOrder_.size()) - 1;
            }
            prevItemIdx = shuffleOrder_[shufflePos_];
            break;
        }
    }

    if (prevItemIdx < 0 || prevItemIdx >= static_cast<int>(items_.size())) {
        return std::nullopt;
    }

    currentIndex_ = prevItemIdx;

    // 如果上一个 item 是 Playlist，定位到歌单最后一首
    const auto& prevItem = items_[currentIndex_];
    if (prevItem.type == QueueItemType::Playlist && playlistResolver_) {
        auto songs = playlistResolver_(prevItem.playlistId);
        playlistPos_ = static_cast<int>(songs.size()) - 1;
        if (playlistPos_ < 0) playlistPos_ = 0;
    } else {
        playlistPos_ = 0;
    }

    SongInfo song = resolveCurrent();
    if (song.id == 0) return std::nullopt;
    return song;
}

bool PlayQueue::setCurrentIndex(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) return false;

    currentIndex_ = index;
    playlistPos_ = 0;

    if (mode_ == PlayMode::Shuffle) {
        ensureShuffleOrder();
        auto it = std::find(shuffleOrder_.begin(), shuffleOrder_.end(), index);
        if (it != shuffleOrder_.end()) {
            shufflePos_ = static_cast<int>(it - shuffleOrder_.begin());
        } else {
            regenerateShuffleOrder();
        }
    }
    return true;
}

// ============================================================
// 播放模式
// ============================================================

void PlayQueue::setPlayMode(PlayMode mode) {
    if (mode_ == mode) return;
    mode_ = mode;
    if (mode == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    } else {
        shuffleOrder_.clear();
        shufflePos_ = 0;
    }
}

PlayMode PlayQueue::getPlayMode() const {
    return mode_;
}

// ============================================================
// Shuffle 内部方法
// ============================================================

void PlayQueue::regenerateShuffleOrder() {
    int n = static_cast<int>(items_.size());
    shuffleOrder_.resize(n);
    for (int i = 0; i < n; ++i) shuffleOrder_[i] = i;
    std::shuffle(shuffleOrder_.begin(), shuffleOrder_.end(), rng());

    shufflePos_ = 0;
    if (currentIndex_ >= 0 && currentIndex_ < n) {
        auto it = std::find(shuffleOrder_.begin(), shuffleOrder_.end(), currentIndex_);
        if (it != shuffleOrder_.end()) {
            std::swap(shuffleOrder_[0], *it);
        }
    }
}

void PlayQueue::ensureShuffleOrder() {
    if (mode_ == PlayMode::Shuffle && shuffleOrder_.empty()) {
        regenerateShuffleOrder();
    }
}
