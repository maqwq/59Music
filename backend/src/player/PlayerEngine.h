#ifndef PLAYER_ENGINE_H
#define PLAYER_ENGINE_H

#include <string>
#include <cstdint>

struct ma_sound;  // miniaudio 声音对象，用于回调参数类型

/**
 * PlayerEngine — 音频播放引擎
 *
 * 基于 miniaudio 的 ma_engine + ma_sound 实现，直接驱动扬声器发声。
 * 职责：播放/暂停/停止/跳转/音量/静音控制，不管理队列（由 PlayQueue 负责）。
 *
 * 自动检测歌曲播放完毕：注册了 end callback，
 * 歌曲播完时内部将状态设为"未播放"，外部轮询 isPlaying() 即可感知。
 *
 * 使用流程：
 *   PlayerEngine engine;
 *   engine.init();
 *   engine.play("song.mp3");
 *   // ... 自动出声 ...
 *   engine.toggle();         // 暂停
 *   engine.toggle();         // 恢复（返回 true 表示正在播放）
 *   engine.seek(60.0);       // 跳到 60 秒
 *   engine.setVolume(50);    // 音量 50%
 *   engine.setMuted(true);   // 静音
 *   engine.stop();
 *   engine.uninit();
 */
class PlayerEngine {
public:
    PlayerEngine();
    ~PlayerEngine();

    PlayerEngine(const PlayerEngine&) = delete;
    PlayerEngine& operator=(const PlayerEngine&) = delete;

    PlayerEngine(PlayerEngine&& other) noexcept;
    PlayerEngine& operator=(PlayerEngine&& other) noexcept;

    // ─── 引擎生命周期 ─────────────────────────────────────

    /** 初始化音频引擎，准备播放 */
    bool init();

    /** 关闭引擎，释放所有资源 */
    void uninit();

    /** 引擎是否已初始化 */
    bool isInitialized() const;

    // ─── 播放控制 ─────────────────────────────────────────

    /**
     * 加载并播放指定文件
     * @param filePath 音频文件路径（支持 mp3/flac/wav 等）
     * @return true 开始播放，false 加载失败
     */
    bool play(const std::string& filePath);

    /**
     * 切换播放/暂停
     * @return 切换后的播放状态：true=播放中，false=已暂停
     */
    bool toggle();

    /** 停止播放（自动 uninit sound，下次 play 重新加载） */
    void stop();

    /**
     * 跳转到指定位置（秒）
     * @param seconds 目标位置，超出时长则停在末尾
     */
    bool seek(double seconds);

    // ─── 音量控制 ─────────────────────────────────────────

    /**
     * 设置音量
     * @param volume 0-100
     */
    void setVolume(int volume);

    /**
     * 设置静音状态
     * @param muted true=静音，false=恢复原音量
     */
    void setMuted(bool muted);

    /**
     * 切换静音状态
     * @return 切换后的静音状态 true=已静音
     */
    bool toggleMute();

    // ─── 状态查询 ─────────────────────────────────────────

    /** 是否正在播放（歌曲自然播完时自动变为 false） */
    bool isPlaying() const;

    /** 当前播放位置（秒） */
    double getCurrentPosition() const;

    /** 当前歌曲总时长（秒） */
    double getDuration() const;

    /** 当前音量（0-100） */
    int getVolume() const;

    /** 是否静音 */
    bool isMuted() const;

    /** 当前是否有加载的歌曲 */
    bool hasSong() const;

private:
    // 歌曲播放完毕时由 miniaudio 音频线程回调的静态函数
    static void onSoundEnd(void* pUserData, ma_sound* pSound);

    struct Impl;
    Impl* impl_;
};

#endif // PLAYER_ENGINE_H
