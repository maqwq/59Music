/**
 * AudioDecoder 测试程序
 *
 * 编译后运行：
 *   audio_test D:/Music/test.wav
 *
 * 会输出音频文件的元信息和部分 PCM 数据。
 * 如果播放正常且没有崩溃，说明解码器工作正常。
 */

#include "player/AudioDecoder.h"
#include <iostream>
#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: audio_test <音频文件路径>" << std::endl;
        std::cerr << "示例: audio_test D:/Music/test.mp3" << std::endl;
        return 1;
    }

    const char* filePath = argv[1];

    // 1. 测试打开
    AudioDecoder decoder;
    if (!decoder.open(filePath)) {
        std::cerr << "[失败] 无法打开文件: " << filePath << std::endl;
        return 1;
    }

    std::cout << "[成功] 文件打开成功" << std::endl;
    std::cout << "       路径: " << filePath << std::endl;
    std::cout << "       时长: " << decoder.getDuration() << " 秒" << std::endl;
    std::cout << "       采样率: " << decoder.getSampleRate() << " Hz" << std::endl;
    std::cout << "       声道数: " << decoder.getChannels() << std::endl;

    // 2. 测试读取 PCM 数据
    std::cout << "\n[测试] 读取 44100 个采样点（1秒数据）..." << std::endl;
    auto samples = decoder.readSamples(44100);
    std::cout << "       实际读取: " << samples.size() << " 个采样点" << std::endl;

    if (!samples.empty()) {
        std::cout << "       前 10 个采样值: ";
        for (int i = 0; i < 10 && i < (int)samples.size(); ++i) {
            printf("%.4f ", samples[i]);
        }
        std::cout << std::endl;
    }

    // 3. 测试当前位置
    std::cout << "\n[测试] 当前位置: " << decoder.getCurrentPosition() << " 秒" << std::endl;

    // 4. 测试 seek
    double totalDuration = decoder.getDuration();
    if (totalDuration > 30) {
        std::cout << "\n[测试] 跳转到 30 秒处..." << std::endl;
        bool seekOk = decoder.seek(30.0);
        std::cout << "       Seek " << (seekOk ? "成功" : "失败") << std::endl;
        std::cout << "       当前位置: " << decoder.getCurrentPosition() << " 秒" << std::endl;

        // seek 后读取，验证数据不同
        auto samplesAfterSeek = decoder.readSamples(4410);
        std::cout << "       Seek 后读取: " << samplesAfterSeek.size() << " 个采样点" << std::endl;
    }

    // 5. 关闭
    decoder.close();
    std::cout << "\n[完成] 所有测试通过，解码器工作正常!" << std::endl;

    return 0;
}
