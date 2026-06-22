// 注意：不定义 MINIAUDIO_IMPLEMENTATION！
// 该宏已在 miniaudio_impl.cpp 中定义一次。
#include "miniaudio.h"

#include "PlayerEngine.h"
#include <cstring>
#include <stdexcept>

// ─── 歌曲结束回调（在 miniaudio 音频线程调用） ──────────
static void onSoundEnd(void* pUserData, ma_sound*) {
    // 仅设置标志，不做其他操作（音频线程中调用）
    *static_cast<bool*>(pUserData) = false;
}

// ─── PImpl 内部实现结构 ───────────────────────────────────────
//
// 手动维护 playing_ 标志，避免直接调用 ma_sound_* 函数
// 导致的崩溃问题。playing_ 只由 play/stop/toggle 和
// ma_sound_set_end_callback 在音频线程中修改。
//
struct PlayerEngine::Impl {
    ma_engine* engine = nullptr;
    ma_sound*  sound  = nullptr;

    bool initialized = false;
    bool playing     = false;   // 手动维护的播放状态

    int  volume       = 80;
    int  savedVolume  = 80;
    bool muted        = false;

    double durationSec = 0.0;   // 缓存时长，避免跨线程查询 miniaudio
    double positionSec = 0.0;   // 缓存位置（每次 play/seek/toggle 时更新）

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
        ma_result result = ma_sound_init_from_file(
            engine, filePath.c_str(), 0, nullptr, nullptr, sound
        );
        if (result != MA_SUCCESS) {
            delete sound;
            sound = nullptr;
            return false;
        }

        currentFilePath = filePath;
        ma_sound_set_volume(sound, muted ? 0.0f : volume / 100.0f);

        // 缓存时长（避免 progressLoop 跨线程查询崩溃）
        ma_uint64 frames;
        if (ma_sound_get_length_in_pcm_frames(sound, &frames) == MA_SUCCESS) {
            durationSec = static_cast<double>(frames) / ma_engine_get_sample_rate(engine);
        } else {
            durationSec = 0.0;
        }

        // 注册结束回调（miniaudio 音频线程中设置手动标志）
        ma_sound_set_end_callback(sound, onSoundEnd, &playing);

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
    impl_->positionSec = 0.0;
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

    ma_uint64 frame = static_cast<ma_uint64>(
        seconds * ma_engine_get_sample_rate(impl_->engine)
    );
    ma_result result = ma_sound_seek_to_pcm_frame(impl_->sound, frame);
    if (result == MA_SUCCESS) {
        impl_->positionSec = seconds;
        return true;
    }
    return false;
}

void PlayerEngine::updatePosition() {
    if (impl_->sound && impl_->playing) {
        ma_uint64 cursor;
        if (ma_sound_get_cursor_in_pcm_frames(impl_->sound, &cursor) == MA_SUCCESS) {
            impl_->positionSec = static_cast<double>(cursor) / ma_engine_get_sample_rate(impl_->engine);
        }
    }
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
    // 使用手动标志，不调 miniaudio 函数，避免跨线程访问
    return impl_->playing;
}

double PlayerEngine::getCurrentPosition() const {
    // 从缓存读取，不调 miniaudio 函数（跨线程安全）
    return impl_->positionSec;
}

double PlayerEngine::getDuration() const {
    // 从缓存读取，不调 miniaudio 函数（跨线程安全）
    return impl_->durationSec;
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
