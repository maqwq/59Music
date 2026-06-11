#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <string>
#include <cstdint>
#include <vector>

/**
 * AudioDecoder — 音频文件解码器
 *
 * 职责：打开音频文件，读取 PCM 数据，支持进度跳转。
 * 播放控制（play/pause）由上层 PlayerEngine 负责。
 *
 * 使用流程：
 *   AudioDecoder decoder;
 *   decoder.open("song.mp3");
 *   int duration = decoder.getDuration();            // 总时长(秒)
 *   std::vector<float> pcm = decoder.readSamples(1024); // 读 1024 个采样点
 *   decoder.seek(60);                                // 跳转到 60 秒处
 *   decoder.close();
 */
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // 禁止拷贝
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    // 允许移动
    AudioDecoder(AudioDecoder&& other) noexcept;
    AudioDecoder& operator=(AudioDecoder&& other) noexcept;

    /**
     * 打开一个音频文件
     * @param filePath 文件绝对路径，例如 "D:/Music/song.mp3"
     * @return true 打开成功，false 失败（文件不存在/格式不支持）
     */
    bool open(const std::string& filePath);

    /**
     * 关闭当前打开的音频文件，释放资源
     */
    void close();

    /**
     * 读取指定数量的 PCM 采样点（float 格式，-1.0 ~ 1.0）
     * @param numSamples 想要读取的采样点数
     * @return 实际读取到的采样点数组，可能少于请求数量（文件结尾）
     */
    std::vector<float> readSamples(uint64_t numSamples);

    /**
     * 跳转到指定播放位置
     * @param seconds 目标位置（秒），超出时长则停留在末尾
     * @return true 跳转成功，false 失败
     */
    bool seek(double seconds);

    // ─── 元信息查询 ──────────────────────────────────────────────

    /** 当前是否已打开一个有效文件 */
    bool isOpen() const;

    /** 歌曲总时长（秒），-1 表示未知 */
    double getDuration() const;

    /** 采样率，例如 44100 */
    int getSampleRate() const;

    /** 声道数，例如 2（立体声） */
    int getChannels() const;

    /** 当前播放位置（秒） */
    double getCurrentPosition() const;

private:
    // 使用 PImpl 技法隐藏 miniaudio 的具体类型，
    // 避免外部依赖 miniaudio.h
    struct Impl;
    Impl* impl_;
};

#endif // AUDIO_DECODER_H
