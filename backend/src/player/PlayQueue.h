#ifndef PLAY_QUEUE_H
#define PLAY_QUEUE_H

#include "models/SongInfo.h"
#include "models/PlayMode.h"

#include <vector>
#include <optional>
#include <cstdint>

/**
 * PlayQueue — 播放队列
 *
 * ═══════════════════════════════════════════════════════════════
 * 职责
 * ═══════════════════════════════════════════════════════════════
 * 1. 维护歌曲列表（增删改）
 *    提供 addSongs / removeAt / clear / reorder 操作
 *
 * 2. 维护当前播放索引
 *    currentIndex_ 指向 queue_ 中的哪首歌正在播放
 *    -1 表示队列为空，没有当前歌曲
 *
 * 3. 根据 PlayMode 决定切歌逻辑
 *    next() / previous() 内部根据 mode_ 走不同的分支
 *
 * ═══════════════════════════════════════════════════════════════
 * 与 PlayerEngine 的关系
 * ═══════════════════════════════════════════════════════════════
 * PlayQueue 只负责"决定下一首是谁"，不负责播放。
 * 外部调用方（如 P3 HttpServer）的流程：
 *   1. 调 PlayQueue::next()   → 得到下一首歌
 *   2. 调 PlayerEngine::play(下一首的 filePath) → 出声音
 *   3. 当前首歌播完时，回到步骤 1
 *
 * ═══════════════════════════════════════════════════════════════
 * 使用流程
 * ═══════════════════════════════════════════════════════════════
 *   PlayQueue queue;
 *   queue.setPlayMode(PlayMode::ListLoop);
 *   queue.addSongs(songs);
 *   queue.setCurrentIndex(0);
 *   auto song = queue.next();  // 根据 mode 自动决定下一首
 */
class PlayQueue {
public:
    // 用编译器自动生成的默认构造函数（所有成员用各自的默认值初始化）
    PlayQueue() = default;

    // ──────────────────────────────────────────────────────────
    // 队列管理
    // ──────────────────────────────────────────────────────────

    /**
     * 批量添加歌曲到队列末尾
     *
     * 若添加前队列为空，会自动将 currentIndex_ 指向第一首（index=0）。
     * 若当前处于 Shuffle 模式，会自动更新 shuffleOrder_。
     *
     * @param songs  要添加的歌曲列表（传入空 vector 则什么都不做）
     */
    void addSongs(const std::vector<SongInfo>& songs);

    /**
     * 根据索引移除歌曲（从 0 开始）
     *
     * 移除后会同步调整 currentIndex_：
     *   - 移除位置在当前歌之前 → currentIndex_ 减 1
     *   - 移除的正是当前歌     → currentIndex_ 移到下一位（或 0，或 -1）
     *   - 移除位置在当前歌之后 → currentIndex_ 不变
     *
     * @param  index  要移除的歌曲在队列中的位置（0-based）
     * @return true  移除成功
     *         false index 超出范围
     */
    bool removeAt(int index);

    /**
     * 清空队列，重置所有状态
     *
     * 会清空：
     *   - queue_（歌曲列表）
     *   - currentIndex_（设为 -1）
     *   - shuffleOrder_ 和 shufflePos_（Shuffle 历史）
     */
    void clear();

    /**
     * 调整队列中歌曲的位置（拖拽排序用）
     *
     * 将 from 位置的歌曲移动到 to 位置，其余元素依次平移。
     * 调整后同步更新 currentIndex_。
     *
     * 示例：
     *   队列 [A, B, C, D, E]，reorder(4→1)：
     *   → [A, E, B, C, D]（E 从第5位移到第2位）
     *
     * @param  from  原位置索引
     * @param  to    目标位置索引
     * @return true  排序成功
     *         false 参数无效（越界 / from==to）
     */
    bool reorder(int from, int to);

    // ──────────────────────────────────────────────────────────
    // 播放导航
    // ──────────────────────────────────────────────────────────

    /**
     * 获取当前歌曲
     *
     * 只是读取，不改变 currentIndex_。
     *
     * @return 当前歌曲，队列为空时返回 std::nullopt
     */
    std::optional<SongInfo> current() const;

    /**
     * 根据 PlayMode 获取下一首
     *
     * 各模式行为：
     *
     *   Sequential 顺序播放
     *     currentIndex_ + 1
     *     超出队列末尾 → 返回 nullopt（播完即停）
     *
     *   SingleLoop 单曲循环
     *     currentIndex_ 不变，永远返回同一首
     *     如果当前歌刚好被删了 → 跳到第 0 首
     *
     *   ListLoop 列表循环
     *     (currentIndex_ + 1) % size
     *     播完最后一首 → 回到第一首继续
     *
     *   Shuffle 随机播放
     *     从 shuffleOrder_（打乱后的索引列表）中取下一项
     *     所有歌都播过一遍 → 重新洗牌，开始下一轮
     *
     * @return 下一首，无可播歌曲时返回 nullopt
     */
    std::optional<SongInfo> next();

    /**
     * 根据 PlayMode 获取上一首
     *
     * 各模式行为：
     *
     *   Sequential
     *     currentIndex_ - 1
     *     小于 0 → 返回 nullopt
     *
     *   SingleLoop
     *     返回当前歌曲
     *
     *   ListLoop
     *     (currentIndex_ - 1 + size) % size
     *     到头 → 回到最后一首
     *
     *   Shuffle
     *     在 shuffleOrder_ 中回退一步
     *     已在开头 → 回到上一轮最后一首
     *
     * @return 上一首，无可播歌曲时返回 nullopt
     */
    std::optional<SongInfo> previous();

    /**
     * 直接跳转到队列中的指定位置
     *
     * 用于：用户点击队列中的某首歌直接播放。
     * 在 Shuffle 模式下，会同步定位 shufflePos_。
     *
     * @param  index  目标位置（0-based）
     * @return true  跳转成功
     *         false index 超出范围
     */
    bool setCurrentIndex(int index);

    // ──────────────────────────────────────────────────────────
    // 播放模式
    // ──────────────────────────────────────────────────────────

    /**
     * 切换播放模式
     *
     * 切换到 Shuffle 时，自动生成随机播放顺序
     * （当前歌始终排在 shuffle 的第一位）。
     * 从 Shuffle 切回其他模式时，清除 shuffle 状态。
     *
     * @param mode  目标播放模式
     */
    void setPlayMode(PlayMode mode);

    /** 获取当前播放模式 */
    PlayMode getPlayMode() const;

    // ──────────────────────────────────────────────────────────
    // 状态查询
    // ──────────────────────────────────────────────────────────

    /**
     * 获取整个队列的 const 引用
     *
     * 外部可以遍历但不允许修改。
     * 用于：HTTP GET /queue 接口返回队列数据。
     */
    const std::vector<SongInfo>& getQueue() const;

    /**
     * 获取当前播放索引
     *
     * @return 0-based 索引，-1 表示队列为空没有当前歌曲
     */
    int getCurrentIndex() const;

    /** 队列中的歌曲总数 */
    int size() const;

    /** 队列是否为空（size() == 0） */
    bool isEmpty() const;

private:
    // ──────────────────────────────────────────────────────────
    // 核心数据
    // ──────────────────────────────────────────────────────────

    /**
     * 歌曲列表
     * 按添加顺序排列，这是"原始顺序"。
     * 所有模式（包括 Shuffle）最终播的都是这里面的歌。
     */
    std::vector<SongInfo> queue_;

    /**
     * 当前播放索引
     * 指向 queue_ 中正在播放的歌曲的位置。
     * -1  = 队列为空，还没有任何歌曲被选中播放
     * 0   = 第 1 首（永远从 0 开始计数）
     */
    int currentIndex_ = -1;

    /**
     * 当前播放模式
     * 默认 Sequential（顺序播放，播完即停）。
     * next() / previous() 内部根据此值走不同分支。
     */
    PlayMode mode_ = PlayMode::Sequential;

    // ──────────────────────────────────────────────────────────
    // Shuffle 模式专用数据
    // ──────────────────────────────────────────────────────────

    /**
     * 打乱后的索引顺序
     *
     * 举例：
     *   queue_ = [稻香, 晴天, 七里香]  (index 0, 1, 2)
     *   shuffleOrder_ = [2, 0, 1]
     *   意味着实际播放顺序是：七里香 → 稻香 → 晴天
     *
     * 只在 mode_ == Shuffle 时有效，其他模式为空。
     * 队列内容变化时（增/删/排序）会自动重新生成。
     */
    std::vector<int> shuffleOrder_;

    /**
     * 当前在 shuffleOrder_ 中的位置
     *
     * shuffleOrder_ 相当于一个"播放列表"，
     * shufflePos_ 表示这个列表走到哪了。
     *
     * 当 shufflePos_ 到达末尾时（所有歌播完一轮），
     * 重新洗牌生成新的 shuffleOrder_，shufflePos_ 回到 0。
     */
    int shufflePos_ = 0;

    // ──────────────────────────────────────────────────────────
    // 内部辅助方法
    // ──────────────────────────────────────────────────────────

    /**
     * 重新生成随机播放顺序
     *
     * 步骤：
     *   1. 生成 [0, 1, 2, ..., size-1] 的有序数组
     *   2. 用 std::shuffle 打乱
     *   3. 确保当前歌曲排在第一位
     *   4. shufflePos_ 重置为 0
     *
     * 触发时机：
     *   - 切换到 Shuffle 模式
     *   - 队列增删改后
     *   - Shuffle 模式下一轮播完
     */
    void regenerateShuffleOrder();

    /**
     * 确保 shuffle 顺序有效
     *
     * 如果当前是 Shuffle 模式但 shuffleOrder_ 为空，
     * 则调用 regenerateShuffleOrder() 初始化。
     *
     * 用于 addSongs / next / previous / setCurrentIndex 时，
     * 确保 shuffle 状态和数据一致。
     */
    void ensureShuffleOrder();
};

#endif // PLAY_QUEUE_H
