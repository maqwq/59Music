// 注意：不定义 MINIAUDIO_IMPLEMENTATION！
// 该宏已在 miniaudio_impl.cpp 中定义一次。
#include "miniaudio.h"

#include "PlayerEngine.h"
#include <cstring>
#include <stdexcept>

// ─── 歌曲播放完毕回调（在 miniaudio 音频线程中调用） ─────
//
// 歌曲自然播完时 miniaudio 会回调此函数。
// 我们的任务：将 playing 标志设为 false，
// 外部通过轮询 isPlaying() 即可感知播完事件，触发自动切歌。
//
void PlayerEngine::onSoundEnd(void* pUserData, ma_sound* /*pSound*/) {
    auto* playing = static_cast<bool*>(pUserData);
    *playing = false;
}

// ─── PImpl 内部实现结构 ───────────────────────────────────────
struct PlayerEngine::Impl {
    ma_engine* engine = nullptr;
    ma_sound*  sound  = nullptr;

    bool initialized = false;
    bool playing     = false;

    int  volume       = 80;  // 0-100
    int  savedVolume  = 80;  // 静音前的音量
    bool muted        = false;

    std::string currentFilePath;

    ~Impl() {
        cleanupSound();
        cleanupEngine();
    }

    bool initEngine() {
        if (engine) return true;

        engine = new ma_engine();
        ma_result result = ma_engine_init(nullptr, engine);
        if (result != MA_SUCCESS) {
            delete engine;
            engine = nullptr;
            return false;
        }

        initialized = true;
        ma_engine_start(engine);
        return true;
    }

    void cleanupEngine() {
        if (engine) {
            ma_engine_uninit(engine);
            delete engine;
            engine = nullptr;
        }
        initialized = false;
    }

    bool loadSound(const std::string& filePath) {
        cleanupSound();

        sound = new ma_sound();
        // flags = 0 表示不会自动播放，由外部调用 ma_sound_start
        ma_result result = ma_sound_init_from_file(
            engine, filePath.c_str(), 0, nullptr, nullptr, sound
        );

        if (result != MA_SUCCESS) {
            delete sound;
            sound = nullptr;
            return false;
        }

        currentFilePath = filePath;

        // 注册歌曲播完回调，pUserData 指向 playing 标志
        ma_sound_set_end_callback(sound, onSoundEnd, &playing);

        // 应用当前音量（如果是静音状态则设为 0）
        ma_sound_set_volume(sound, muted ? 0.0f : volume / 100.0f);

        return true;
    }

    void cleanupSound() {
        if (sound) {
            ma_sound_stop(sound);
            ma_sound_uninit(sound);
            delete sound;
            sound = nullptr;
        }
        playing = false;
        currentFilePath.clear();
    }
};

// ─── 公有接口实现 ─────────────────────────────────────────────

PlayerEngine::PlayerEngine()
    : impl_(new Impl())
{
}

PlayerEngine::~PlayerEngine() {
    delete impl_;
}

PlayerEngine::PlayerEngine(PlayerEngine&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

PlayerEngine& PlayerEngine::operator=(PlayerEngine&& other) noexcept {
    if (this != &other) {
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

bool PlayerEngine::init() {
    return impl_->initEngine();
}

void PlayerEngine::uninit() {
    impl_->cleanupSound();
    impl_->cleanupEngine();
}

bool PlayerEngine::isInitialized() const {
    return impl_->initialized;
}

bool PlayerEngine::play(const std::string& filePath) {
    if (!impl_->engine) return false;

    if (!impl_->loadSound(filePath)) {
        return false;
    }

    ma_result result = ma_sound_start(impl_->sound);
    if (result != MA_SUCCESS) {
        impl_->cleanupSound();
        return false;
    }

    impl_->playing = true;
    return true;
}

bool PlayerEngine::toggle() {
    if (!impl_->sound) return false;

    if (impl_->playing) {
        ma_sound_stop(impl_->sound);
        impl_->playing = false;
    } else {
        ma_sound_start(impl_->sound);
        impl_->playing = true;
    }
    return impl_->playing;
}

void PlayerEngine::stop() {
    impl_->cleanupSound();
}

bool PlayerEngine::seek(double seconds) {
    if (!impl_->sound) return false;

    // 使用引擎采样率进行帧计算。
    // miniaudio 引擎会自动重采样所有音频到引擎采样率，
    // 所以用引擎采样率计算帧位置是准确的。
    ma_uint64 frame = static_cast<ma_uint64>(
        seconds * ma_engine_get_sample_rate(impl_->engine)
    );
    ma_result result = ma_sound_seek_to_pcm_frame(impl_->sound, frame);
    return result == MA_SUCCESS;
}

void PlayerEngine::setVolume(int volume) {
    impl_->volume = (volume < 0) ? 0 : (volume > 100) ? 100 : volume;
    impl_->savedVolume = impl_->volume;
    if (impl_->sound && !impl_->muted) {
        ma_sound_set_volume(impl_->sound, impl_->volume / 100.0f);
    }
}

void PlayerEngine::setMuted(bool muted) {
    if (impl_->muted == muted) return;
    impl_->muted = muted;

    if (impl_->sound) {
        if (muted) {
            impl_->savedVolume = impl_->volume;
            ma_sound_set_volume(impl_->sound, 0.0f);
        } else {
            ma_sound_set_volume(impl_->sound, impl_->savedVolume / 100.0f);
        }
    }
}

bool PlayerEngine::toggleMute() {
    setMuted(!impl_->muted);
    return impl_->muted;
}

bool PlayerEngine::isPlaying() const {
    return impl_->playing;
}

double PlayerEngine::getCurrentPosition() const {
    if (!impl_->sound) return 0.0;

    ma_uint64 cursor;
    ma_result result = ma_sound_get_cursor_in_pcm_frames(impl_->sound, &cursor);
    if (result != MA_SUCCESS) return 0.0;

    return static_cast<double>(cursor) / ma_engine_get_sample_rate(impl_->engine);
}

double PlayerEngine::getDuration() const {
    if (!impl_->sound) return 0.0;

    ma_uint64 length;
    ma_result result = ma_sound_get_length_in_pcm_frames(impl_->sound, &length);
    if (result != MA_SUCCESS) return 0.0;

    return static_cast<double>(length) / ma_engine_get_sample_rate(impl_->engine);
}

int PlayerEngine::getVolume() const {
    return impl_->volume;
}

bool PlayerEngine::isMuted() const {
    return impl_->muted;
}

bool PlayerEngine::hasSong() const {
    return impl_->sound != nullptr;
}
