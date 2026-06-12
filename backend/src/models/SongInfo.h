#ifndef SONG_INFO_H
#define SONG_INFO_H

#include <string>
#include <cstdint>

/**
 * SongInfo — 歌曲信息结构体
 *
 * 与 API 文档中的 SongInfo 保持一致。
 * id = 0 表示尚未入库（临时数据）。
 */
struct SongInfo {
    int         id        = 0;      ///< 数据库主键，自增
    std::string title;              ///< 歌名
    std::string artist;             ///< 歌手
    std::string album;              ///< 专辑
    int         duration  = 0;      ///< 时长（秒）
    std::string filePath;           ///< 音频文件绝对路径
    int64_t     addedTime = 0;      ///< 加入时间戳（Unix 秒）
};

#endif // SONG_INFO_H
