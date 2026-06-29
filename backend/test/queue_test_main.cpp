/**
 * PlayQueue 单元测试
 *
 * 测试队列管理 + 4种播放模式的切歌逻辑。
 * 纯逻辑测试，不需要音频文件。
 */

#include "player/PlayQueue.h"
#include <iostream>
#include <cassert>
#include <string>
#include <map>

using Music::SongInfo;

// ─── 辅助函数 ──────────────────────────────────────────────

static std::map<int, SongInfo> songDb;

static SongInfo makeSong(int id, const std::string& title) {
    SongInfo s;
    s.id = id;
    s.title = title;
    s.filePath = "D:/Music/" + title + ".mp3";
    songDb[id] = s;
    return s;
}

static SongInfo resolveSong(int songId) {
    auto it = songDb.find(songId);
    if (it != songDb.end()) return it->second;
    return SongInfo{};
}

static void printResult(const std::string& name, bool ok) {
    std::cout << "  " << (ok ? "[PASS]" : "[FAIL]") << " " << name << std::endl;
}

static int failed = 0;
#define TEST(name, expr) do { \
    bool ok = (expr); \
    printResult(name, ok); \
    if (!ok) failed++; \
} while(0)

// ─── 主测试 ────────────────────────────────────────────────

int main() {
    std::cout << "=== PlayQueue 测试 ===\n" << std::endl;

    // 准备测试用歌曲
    for (int i = 1; i <= 5; ++i) {
        makeSong(i, "Song" + std::to_string(i));
    }
    std::vector<int> songIds = {1, 2, 3, 4, 5};

    // ─── 1. 基础队列操作 ───────────────────────────────
    std::cout << "--- 1. 基础队列操作 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        TEST("新建队列为空", q.isEmpty());
        TEST("size=0", q.size() == 0);
        TEST("current 为 null", !q.current().has_value());
        TEST("currentIndex=-1", q.getCurrentIndex() == -1);

        q.addSongs(songIds);
        TEST("添加5首歌后不为空", !q.isEmpty());
        TEST("size=5", q.size() == 5);
        TEST("自动指向第一首", q.getCurrentIndex() == 0);
        TEST("当前歌是 Song1", q.current()->title == "Song1");

        q.removeAt(0);
        TEST("移除第1首后 size=4", q.size() == 4);
        TEST("指向新第一首 Song2", q.current()->title == "Song2");

        q.clear();
        TEST("清空后为空", q.isEmpty());
        TEST("size=0", q.size() == 0);
    }

    // ─── 2. reorder ───────────────────────────────────
    std::cout << "\n--- 2. 队列排序 (reorder) ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);

        // [Song1, Song2, Song3, Song4, Song5] -> 移 Song5 到 Song1 前
        q.reorder(4, 0);
        const auto& items = q.getItems();
        TEST("Song5 移到第1位", items[0].songId == 5);
        TEST("Song1 移到第2位", items[1].songId == 1);
        TEST("Song4 移到第5位", items[4].songId == 4);
    }

    // ─── 3. Sequential 模式 ──────────────────────────
    std::cout << "\n--- 3. Sequential 模式 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);
        q.setPlayMode(PlayMode::Sequential);

        // 从第1首开始
        auto s1 = q.next();
        TEST("next 到 Song2", s1->title == "Song2");

        auto s2 = q.next();
        TEST("next 到 Song3", s2->title == "Song3");

        // 跳到末尾
        q.setCurrentIndex(4);
        auto s3 = q.next();
        TEST("播完最后一首后返回 null", !s3.has_value());

        // previous
        q.setCurrentIndex(0);
        auto s4 = q.previous();
        TEST("第一首往前返回 null", !s4.has_value());
    }

    // ─── 4. SingleLoop 模式 ─────────────────────────
    std::cout << "\n--- 4. SingleLoop 模式 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);
        q.setCurrentIndex(2);
        q.setPlayMode(PlayMode::SingleLoop);

        auto s1 = q.next();
        TEST("SingleLoop next 仍是 Song3", s1->title == "Song3");

        auto s2 = q.next();
        TEST("SingleLoop next 还是 Song3", s2->title == "Song3");

        auto s3 = q.previous();
        TEST("SingleLoop prev 还是 Song3", s3->title == "Song3");
    }

    // ─── 5. ListLoop 模式 ────────────────────────────
    std::cout << "\n--- 5. ListLoop 模式 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);
        q.setPlayMode(PlayMode::ListLoop);

        // 从 Song1 开始，跳转到最后一首
        q.setCurrentIndex(4);
        auto s1 = q.next();
        TEST("ListLoop 最后一首 next 回到 Song1", s1->title == "Song1");

        auto s2 = q.previous();
        TEST("ListLoop 第一首 prev 回到 Song5", s2->title == "Song5");
    }

    // ─── 6. Shuffle 模式 ─────────────────────────────
    std::cout << "\n--- 6. Shuffle 模式 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);
        q.setCurrentIndex(0);
        q.setPlayMode(PlayMode::Shuffle);

        // Shuffle 模式下 next 5 次，所有歌都应该出现一次且不重复
        bool allVisited[5] = {false};
        allVisited[0] = true; // 当前是 Song1

        for (int i = 1; i < 5; ++i) {
            auto s = q.next();
            int idx = s->id - 1;
            if (idx >= 0 && idx < 5) {
                allVisited[idx] = true;
            }
        }

        bool allCovered = true;
        for (bool v : allVisited) {
            if (!v) { allCovered = false; break; }
        }
        TEST("Shuffle 5次播完所有歌", allCovered);
    }

    // ─── 7. 切换播放模式 ─────────────────────────────
    std::cout << "\n--- 7. 模式切换 ---" << std::endl;
    {
        PlayQueue q;
        q.setSongResolver(resolveSong);
        q.addSongs(songIds);
        q.setCurrentIndex(0);

        q.setPlayMode(PlayMode::ListLoop);
        TEST("切换到 ListLoop", q.getPlayMode() == PlayMode::ListLoop);

        auto s = q.next();
        TEST("ListLoop 下 next 到 Song2", s->title == "Song2");

        q.setPlayMode(PlayMode::Sequential);
        TEST("切换到 Sequential", q.getPlayMode() == PlayMode::Sequential);
        // 模式切换不丢当前位置
        TEST("切换后当前歌不变", q.current()->title == "Song2");
    }

    // ─── 总结 ────────────────────────────────────────
    std::cout << "\n=== 测试完成 ===" << std::endl;
    if (failed == 0) {
        std::cout << "全部通过!" << std::endl;
    } else {
        std::cout << "失败: " << failed << " 项" << std::endl;
    }
    return failed;
}
