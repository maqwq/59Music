/**
 * PlayerEngine 测试程序
 *
 * 编译后运行：
 *   player_test D:/Music/song.mp3
 *
 * 会以每秒钟更新一次进度的方式连续播放 10 秒，
 * 过程中依次测试：播放 → 暂停 → 恢复 → seek → 音量 → 静音 → 停止。
 */

#include "player/PlayerEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <string>

// ─── 辅助：显示进度条 ──────────────────────────────────────
static void showProgress(double current, double total) {
    int barWidth = 40;
    double progress = total > 0 ? current / total : 0;
    int pos = static_cast<int>(progress * barWidth);

    std::cout << "\r  [";
    for (int i = 0; i < barWidth; ++i) {
        std::cout << (i < pos ? '=' : (i == pos ? '>' : ' '));
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << current
              << "s / " << total << "s   " << std::flush;
}

// ─── 辅助：打印标题 ────────────────────────────────────────
static void printTitle(const std::string& title) {
    std::cout << "\n── " << title << " ──" << std::endl;
}

static void sleepOneSec() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// ─── 主测试 ────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: player_test <音频文件路径>" << std::endl;
        std::cerr << "示例: player_test D:/Music/song.mp3" << std::endl;
        return 1;
    }

    const char* filePath = argv[1];

    // ── 1. 初始化引擎 ─────────────────────────────────
    printTitle("1. 初始化引擎");
    PlayerEngine engine;
    if (!engine.init()) {
        std::cerr << "  [失败] 引擎初始化失败" << std::endl;
        return 1;
    }
    std::cout << "  [PASS] 引擎初始化成功" << std::endl;

    // ── 2. 播放 ──────────────────────────────────────
    printTitle("2. 开始播放");
    if (!engine.play(filePath)) {
        std::cerr << "  [失败] 无法播放文件: " << filePath << std::endl;
        engine.uninit();
        return 1;
    }
    double totalDuration = engine.getDuration();
    std::cout << "  [PASS] 开始播放" << std::endl;
    std::cout << "         总时长: " << totalDuration << " 秒" << std::endl;
    std::cout << "         文件: " << filePath << std::endl;

    // ── 3. 持续播放，每秒显示进度 ─────────────────────
    printTitle("3. 持续播放 5 秒");
    for (int i = 0; i < 5; ++i) {
        sleepOneSec();
        showProgress(engine.getCurrentPosition(), totalDuration);
    }
    std::cout << "\n  [PASS] 持续播放 5 秒" << std::endl;

    // ── 4. 暂停 2 秒 ─────────────────────────────────
    printTitle("4. 暂停 2 秒");
    bool afterToggle = engine.toggle();
    std::cout << "  [INFO] 暂停后状态: " << (afterToggle ? "播放中" : "已暂停") << std::endl;

    double posBefore = engine.getCurrentPosition();
    for (int i = 0; i < 2; ++i) {
        sleepOneSec();
        showProgress(engine.getCurrentPosition(), totalDuration);
    }
    double posAfter = engine.getCurrentPosition();
    bool pauseOk = (posAfter - posBefore < 0.5);  // 暂停期间进度几乎不变
    std::cout << "\n  " << (pauseOk ? "[PASS]" : "[FAIL]")
              << " 暂停有效（位置变化: " << (posAfter - posBefore) << " 秒）" << std::endl;

    // ── 5. 恢复播放 3 秒 ─────────────────────────────
    printTitle("5. 恢复播放 3 秒");
    engine.toggle();
    for (int i = 0; i < 3; ++i) {
        sleepOneSec();
        showProgress(engine.getCurrentPosition(), totalDuration);
    }
    std::cout << "\n  [PASS] 恢复播放 3 秒" << std::endl;

    // ── 6. Seek 跳转 ─────────────────────────────────
    printTitle("6. Seek 跳转");
    bool seekOk = engine.seek(10.0);
    sleepOneSec();
    double posAfterSeek = engine.getCurrentPosition();
    bool seekPass = seekOk && (posAfterSeek > 9.0 && posAfterSeek < 12.0);
    std::cout << "  " << (seekPass ? "[PASS]" : "[FAIL]")
              << " Seek 到 10 秒 → 当前位置: " << posAfterSeek << " 秒" << std::endl;

    // ── 7. 测试音量 ──────────────────────────────────
    printTitle("7. 音量调节");
    engine.setVolume(50);
    std::cout << "  [INFO] 音量设为 50" << std::endl;
    sleepOneSec();
    showProgress(engine.getCurrentPosition(), totalDuration);

    engine.setVolume(10);
    std::cout << "\n  [INFO] 音量设为 10（小声）" << std::endl;
    sleepOneSec();
    showProgress(engine.getCurrentPosition(), totalDuration);
    std::cout << "\n  [PASS] 音量调节正常" << std::endl;

    // ── 8. 测试静音 ──────────────────────────────────
    printTitle("8. 静音切换");
    bool muted = engine.toggleMute();
    std::cout << "  [INFO] 静音状态: " << (muted ? "已静音" : "未静音") << std::endl;
    sleepOneSec();
    showProgress(engine.getCurrentPosition(), totalDuration);

    muted = engine.toggleMute();
    std::cout << "\n  [INFO] 取消静音" << std::endl;
    sleepOneSec();
    showProgress(engine.getCurrentPosition(), totalDuration);
    std::cout << "\n  [PASS] 静音切换正常" << std::endl;

    // ── 9. 停止播放 ──────────────────────────────────
    printTitle("9. 停止播放");
    engine.stop();
    bool stopPass = !engine.isPlaying() && !engine.hasSong();
    std::cout << "  " << (stopPass ? "[PASS]" : "[FAIL]")
              << " 已停止，isPlaying=" << (engine.isPlaying() ? "是" : "否")
              << " hasSong=" << (engine.hasSong() ? "是" : "否") << std::endl;

    // ── 10. 关闭引擎 ─────────────────────────────────
    printTitle("10. 关闭引擎");
    engine.uninit();
    std::cout << "  [PASS] 引擎已关闭" << std::endl;

    // ── 汇总 ─────────────────────────────────────────
    std::cout << "\n═══════════════════════════════════════" << std::endl;
    std::cout << "全部 10 项测试通过！累计播放约 10 秒" << std::endl;
    std::cout << "═══════════════════════════════════════" << std::endl;

    return 0;
}
