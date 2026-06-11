#include <cstdio>
#include <cmath>
#include <cstdint>

int main() {
    FILE* f = fopen("test_sine.wav", "wb");
    if (!f) return 1;

    int sampleRate = 44100;
    int channels = 1;
    int bitsPerSample = 16;
    int duration = 15; // 15秒，足够测试播放控制
    int dataSize = sampleRate * duration * channels * (bitsPerSample / 8);

    // WAV header
    uint32_t chunkSize = 36 + dataSize;
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = channels;
    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    uint16_t blockAlign = channels * (bitsPerSample / 8);

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunkSize, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    fwrite(&fmtSize, 4, 1, f);
    fwrite(&audioFormat, 2, 1, f);
    fwrite(&numChannels, 2, 1, f);
    fwrite(&sampleRate, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&blockAlign, 2, 1, f);
    fwrite(&bitsPerSample, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&dataSize, 4, 1, f);

    // 生成 440Hz 正弦波
    for (int i = 0; i < sampleRate * duration; ++i) {
        double t = (double)i / sampleRate;
        int16_t sample = (int16_t)(sin(2.0 * M_PI * 440.0 * t) * 30000);
        fwrite(&sample, 2, 1, f);
    }

    fclose(f);
    printf("Created test_sine.wav (%d bytes, %d sec %dHz sine)\n", dataSize + 44, duration, 440);
    return 0;
}
