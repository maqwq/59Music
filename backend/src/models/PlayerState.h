#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include "SongInfo.h"
#include "PlayMode.h"

#include <cstdint>
#include <optional>

using Music::SongInfo;

/**
 * PlayerState — 播放器完整状态快照
 *
 * 用于：
 *   1. GET /player/state 接口的响应
 *   2. WebSocket player_state 推送
 *   3. 前端 Pinia store 的同步
 */
struct PlayerState {
    bool                    isPlaying       = false;     ///< 是否正在播放
    std::optional<SongInfo> currentSong;                 ///< 当前歌曲，无歌时为 nullopt
    int                     currentPosition = 0;         ///< 当前播放进度（秒）
    int                     duration        = 0;         ///< 当前歌曲总时长（秒）
    int                     volume          = 80;        ///< 音量 0-100
    bool                    muted           = false;     ///< 是否静音
    PlayMode                mode            = PlayMode::Sequential;  ///< 播放模式
};

#endif // PLAYER_STATE_H
