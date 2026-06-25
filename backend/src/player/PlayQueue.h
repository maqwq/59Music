#ifndef PLAY_QUEUE_H
#define PLAY_QUEUE_H

#include "models/SongInfo.h"
#include "models/PlayMode.h"

#include <vector>
#include <optional>
#include <functional>
#include <string>
#include <cstdint>

using Music::SongInfo;

// ============================================================
// 队列项类型
// ============================================================

enum class QueueItemType { Song, Playlist };

struct QueueItem {
    QueueItemType type = QueueItemType::Song;
    int songId = 0;              // type==Song 时有效
    int playlistId = 0;          // type==Playlist 时有效
    std::string playlistName;    // type==Playlist 时的显示名称
};

// ============================================================
// 回调类型
// ============================================================

/// 根据 songId 解析 SongInfo（由 HttpServer 注入，调用 db_->getSongById）
using SongResolver = std::function<SongInfo(int songId)>;

/// 根据 playlistId 获取歌单内所有歌曲（实时引用，调用 db_->getPlaylistSongs）
using PlaylistResolver = std::function<std::vector<SongInfo>(int playlistId)>;

// ============================================================
// PlayQueue
// ============================================================

class PlayQueue {
public:
    PlayQueue() = default;

    // ── 回调注入 ─────────────────────────────────────────

    void setSongResolver(SongResolver resolver) { songResolver_ = std::move(resolver); }
    void setPlaylistResolver(PlaylistResolver resolver) { playlistResolver_ = std::move(resolver); }

    // ── 队列管理 ─────────────────────────────────────────

    /// 添加单首歌曲（带去重：同 songId 不可重复出现在 Song 类型项中）
    /// 返回实际添加数量（跳过的重复不计入）
    int addSongs(const std::vector<int>& songIds);

    /// 添加歌单引用
    void addPlaylist(int playlistId, const std::string& name);

    /// 移除指定位置的项
    bool removeAt(int index);

    /// 清空队列
    void clear();

    /// 调整项的顺序
    bool reorder(int from, int to);

    // ── 播放导航 ─────────────────────────────────────────

    std::optional<SongInfo> current();
    std::optional<SongInfo> next();
    std::optional<SongInfo> previous();
    bool setCurrentIndex(int index);

    // ── 播放模式 ─────────────────────────────────────────

    void setPlayMode(PlayMode mode);
    PlayMode getPlayMode() const;

    // ── 状态查询 ─────────────────────────────────────────

    const std::vector<QueueItem>& getItems() const { return items_; }
    int getCurrentIndex() const { return currentIndex_; }
    int getPlaylistPos() const { return playlistPos_; }
    int size() const { return static_cast<int>(items_.size()); }
    bool isEmpty() const { return items_.empty(); }

    /// 获取展开后的队列歌曲总数（用于显示）
    int expandedSize() const;

private:
    // ── 核心数据 ─────────────────────────────────────────

    std::vector<QueueItem> items_;

    /// 当前播放项索引（-1 = 空队列）
    int currentIndex_ = -1;

    /// 歌单项内播放位置（仅在当前 item 为 Playlist 时有效，-1 = 还未开始）
    int playlistPos_ = -1;

    PlayMode mode_ = PlayMode::Sequential;

    // ── Shuffle 专用 ─────────────────────────────────────

    std::vector<int> shuffleOrder_;
    int shufflePos_ = 0;

    // ── 回调 ────────────────────────────────────────────

    SongResolver songResolver_;
    PlaylistResolver playlistResolver_;

    // ── 内部辅助 ─────────────────────────────────────────

    /// 解析当前 item 对应的 SongInfo（处理 Playlist 展开）
    SongInfo resolveCurrent() const;

    /// 获取当前 playlist item 内的歌曲列表
    std::vector<SongInfo> getCurrentPlaylistSongs() const;

    void regenerateShuffleOrder();
    void ensureShuffleOrder();
};

#endif // PLAY_QUEUE_H
