#ifndef PLAY_MODE_H
#define PLAY_MODE_H

#include <string>

/**
 * PlayMode — 播放模式枚举
 *
 * 决定播放队列中切歌时的行为。
 */
enum class PlayMode {
    Sequential,   ///< 顺序播放：队列播完即停
    Reverse,      ///< 倒序播放：从尾到头反向播放
    SingleLoop,   ///< 单曲循环：当前曲播完从头开始
    ListLoop,     ///< 列表循环：队列播完回到第一首继续
    Shuffle       ///< 随机播放：打乱队列顺序
};

/** 将 PlayMode 转为字符串（用于 API 响应） */
inline std::string playModeToString(PlayMode mode) {
    switch (mode) {
        case PlayMode::Sequential: return "sequential";
        case PlayMode::Reverse:    return "reverse";
        case PlayMode::SingleLoop: return "single_loop";
        case PlayMode::ListLoop:   return "list_loop";
        case PlayMode::Shuffle:    return "shuffle";
        default:                   return "sequential";
    }
}

/** 将字符串转为 PlayMode，不匹配时返回 Sequential */
inline PlayMode stringToPlayMode(const std::string& s) {
    if (s == "reverse")      return PlayMode::Reverse;
    if (s == "single_loop")  return PlayMode::SingleLoop;
    if (s == "list_loop")    return PlayMode::ListLoop;
    if (s == "shuffle")      return PlayMode::Shuffle;
    return PlayMode::Sequential;
}

#endif // PLAY_MODE_H
