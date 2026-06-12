// miniaudio 函数实现已提取到 miniaudio_impl.cpp，此处只需声明
#include "miniaudio.h"

#include "AudioDecoder.h"

// ─── PImpl 内部实现结构 ───────────────────────────────────────
struct AudioDecoder::Impl {
    ma_decoder decoder;          // miniaudio 解码器
    ma_format format;            // 解码格式 (我们指定为 f32)
    bool opened = false;

    // 缓存当前播放位置（秒），miniaudio 的 decoder 不直接提供此信息
    double currentPositionSec = 0.0;

    ~Impl() {
        if (opened) {
            ma_decoder_uninit(&decoder);
            opened = false;
        }
    }
};

// ─── 公有接口实现 ─────────────────────────────────────────────

AudioDecoder::AudioDecoder()
    : impl_(new Impl())
{
}

AudioDecoder::~AudioDecoder() {
    delete impl_;
}

// 移动构造
AudioDecoder::AudioDecoder(AudioDecoder&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

AudioDecoder& AudioDecoder::operator=(AudioDecoder&& other) noexcept {
    if (this != &other) {
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

bool AudioDecoder::open(const std::string& filePath) {
    // 先关掉之前打开的文件
    close();

    // 配置解码器输出格式：float32，立体声，44100Hz
    ma_decoder_config config = ma_decoder_config_init(
        ma_format_f32,      // 输出 PCM 格式：32位浮点
        2,                  // 声道数：立体声
        44100               // 采样率
    );

    ma_result result = ma_decoder_init_file(
        filePath.c_str(),
        &config,
        &impl_->decoder
    );

    if (result != MA_SUCCESS) {
        // 打开失败，可能文件不存在或不支持的格式
        impl_->opened = false;
        return false;
    }

    impl_->opened = true;
    impl_->currentPositionSec = 0.0;

    // 获取解码器输出格式信息（注意：outputFormat 可能和 config 不同
    // 因为源文件本身可能有不同的格式，miniaudio 做了转换）
    impl_->format = impl_->decoder.outputFormat;

    return true;
}

void AudioDecoder::close() {
    if (impl_ && impl_->opened) {
        ma_decoder_uninit(&impl_->decoder);
        impl_->opened = false;
        impl_->currentPositionSec = 0.0;
    }
}

std::vector<float> AudioDecoder::readSamples(uint64_t numSamples) {
    if (!impl_->opened || impl_->decoder.outputChannels == 0) {
        return {};
    }

    // 分配缓冲区
    std::vector<float> buffer(numSamples, 0.0f);

    // 读取 PCM 帧（一帧 = 所有声道的一个采样点）
    // 请求的帧数 = 总采样点数 / 声道数
    ma_uint64 framesToRead = numSamples / impl_->decoder.outputChannels;
    if (framesToRead == 0) return {};

    ma_uint64 framesRead;
    ma_result result = ma_decoder_read_pcm_frames(
        &impl_->decoder,
        buffer.data(),
        framesToRead,
        &framesRead
    );

    if (result != MA_SUCCESS) {
        return {};
    }

    // 更新当前播放位置（帧数 / 采样率）
    ma_uint64 totalFramesRead;
    ma_decoder_get_cursor_in_pcm_frames(&impl_->decoder, &totalFramesRead);
    impl_->currentPositionSec = static_cast<double>(totalFramesRead)
                              / impl_->decoder.outputSampleRate;

    // 实际读取到的采样点数
    uint64_t actualSamples = framesRead * impl_->decoder.outputChannels;
    buffer.resize(actualSamples);

    return buffer;
}

bool AudioDecoder::seek(double seconds) {
    if (!impl_->opened) {
        return false;
    }

    // 将秒数转为 PCM 帧位置
    ma_uint64 targetFrame = static_cast<ma_uint64>(
        seconds * impl_->decoder.outputSampleRate
    );

    ma_result result = ma_decoder_seek_to_pcm_frame(
        &impl_->decoder,
        targetFrame
    );

    if (result != MA_SUCCESS) {
        return false;
    }

    impl_->currentPositionSec = seconds;
    return true;
}

bool AudioDecoder::isOpen() const {
    return impl_ && impl_->opened;
}

double AudioDecoder::getDuration() const {
    if (!impl_->opened) {
        return -1.0;
    }

    // 获取总 PCM 帧数
    ma_uint64 totalFrames;
    ma_result result = ma_decoder_get_length_in_pcm_frames(
        &impl_->decoder,
        &totalFrames
    );

    if (result != MA_SUCCESS) {
        return -1.0;
    }

    return static_cast<double>(totalFrames) / impl_->decoder.outputSampleRate;
}

int AudioDecoder::getSampleRate() const {
    if (!impl_->opened) {
        return 0;
    }
    return static_cast<int>(impl_->decoder.outputSampleRate);
}

int AudioDecoder::getChannels() const {
    if (!impl_->opened) {
        return 0;
    }
    return static_cast<int>(impl_->decoder.outputChannels);
}

double AudioDecoder::getCurrentPosition() const {
    return impl_->currentPositionSec;
}
