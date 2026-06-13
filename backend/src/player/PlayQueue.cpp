#include "PlayQueue.h"
#include <algorithm>
#include <random>
#include <chrono>

using Music::SongInfo;

// ═══════════════════════════════════════════════════════════════
// 随机数生成器（函数级静态单例）
// ═══════════════════════════════════════════════════════════════
//
// 用当前时间戳播种，保证每次程序启动时 shuffle 顺序不同。
// 声明为 static 函数，作用域仅限于本文件。
//
static std::mt19937& rng() {
    // steady_clock 保证时间单调递增，不会因系统时间调整而跳变
    static std::mt19937 engine(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()
    ));
    return engine;
}

// ═══════════════════════════════════════════════════════════════
// 队列管理
// ═══════════════════════════════════════════════════════════════

void PlayQueue::addSongs(const std::vector<SongInfo>& songs) {
    // 传入空列表时什么都不做，避免误操作
    if (songs.empty()) return;

    // 用法说明：
    // queue_.insert(目标位置迭代器, 起始迭代器, 结束迭代器)
    // 将 songs 所有元素追加到 queue_ 末尾
    queue_.insert(queue_.end(), songs.begin(), songs.end());

    // 如果之前队列是空的（currentIndex_ == -1），
    // 添加完歌曲后自动指向第一首（index = 0），
    // 这样外部调用 current() 不会返回 nullopt
    if (currentIndex_ < 0 && !queue_.empty()) {
        currentIndex_ = 0;
    }

    // Shuffle 模式下，队列增加了新歌，
    // 重新生成 shuffle 顺序，把新歌也纳入随机池
    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
}

bool PlayQueue::removeAt(int index) {
    // 参数校验：index 必须 >= 0 且 < 队列长度
    // 注意 queue_.size() 返回 size_t（无符号），直接比较会有符号警告，
    // 所以转为 int 再比较
    if (index < 0 || index >= static_cast<int>(queue_.size())) {
        return false;
    }

    // erase(iterator) 会删除指定位置的元素，
    // 后面的元素自动前移，vector 长度减 1
    queue_.erase(queue_.begin() + index);

    // ─── 同步更新 currentIndex_ ────────────────────────────
    //
    // 删除元素后，后面的歌会向前补位，所以 currentIndex_ 需要调整。
    // 分三种情况：
    //
    // 情况 A：删除位置 < 当前播放位置
    //   示例：queue=[A, B, C, D]，currentIndex=3（D），删除 index=1（B）
    //   删除后：queue=[A, C, D]，D 从 index 3 挪到了 index 2
    //   → currentIndex_ 减 1（3→2）
    //
    // 情况 B：删除位置 == 当前播放位置
    //   示例：queue=[A, B, C]，currentIndex=1（B），删除 index=1（B）
    //   删除后：queue=[A, C]
    //   如果当前歌后面还有歌 → 指向后面的歌（index 不变，因为它前移了）
    //   如果后面没歌了 → 指向前面的歌（size-1）
    //   如果全删光了 → 设为 -1
    //
    // 情况 C：删除位置 > 当前播放位置
    //   当前歌位置不变，不影响
    //
    // ─────────────────────────────────────────────────────────
    if (index < currentIndex_) {
        // 情况 A：删除位置在当前歌之前
        currentIndex_--;
    } else if (index == currentIndex_) {
        // 情况 B：删的正是当前歌
        if (currentIndex_ >= static_cast<int>(queue_.size())) {
            // 当前歌之后没歌了，指向新末尾的前一位
            currentIndex_ = queue_.empty() ? -1 : static_cast<int>(queue_.size()) - 1;
        }
        // 否则 currentIndex_ 不变（后面的歌前移填补了空缺）
    }
    // 情况 C：不做任何操作

    // Shuffle 模式需要重新洗牌
    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
    return true;
}

void PlayQueue::clear() {
    // 清空所有数据，重置到刚创建时的状态
    queue_.clear();          // 歌曲列表 → 空
    currentIndex_ = -1;      // 无当前歌曲
    shuffleOrder_.clear();   // 洗牌顺序 → 空
    shufflePos_ = 0;         // 洗牌位置 → 开头
}

bool PlayQueue::reorder(int from, int to) {
    int size = static_cast<int>(queue_.size());

    // 参数校验：
    // - from 和 to 必须在有效范围内 [0, size)
    // - from == to 不需要移动，直接返回成功
    if (from < 0 || from >= size || to < 0 || to >= size || from == to) {
        return false;
    }

    // ─── 移动元素 ────────────────────────────────────────
    //
    // 操作逻辑：先删除 from 位置的元素，再插入到 to 位置。
    //
    // 示例：queue=[A, B, C, D, E], from=4(E), to=1
    //   step 1: erase(4) → [A, B, C, D]
    //   step 2: insert(to=1, E) → [A, E, B, C, D]
    //
    SongInfo song = queue_[from];
    queue_.erase(queue_.begin() + from);
    queue_.insert(queue_.begin() + to, song);

    // ─── 同步更新 currentIndex_ ────────────────────────────
    //
    // 移动元素后，currentIndex_ 可能指向的元素变了位置。
    // 分三种情况：
    //
    // 情况 A：移动的正是当前歌 → currentIndex_ = to
    // 情况 B：移动位置在当前歌之前，但目标在当前歌之后
    //         → 当前歌被往前挤了一位，currentIndex_ 减 1
    // 情况 C：移动位置在当前歌之后，但目标在当前歌之前
    //         → 当前歌被往后挤了一位，currentIndex_ 加 1
    //
    if (from == currentIndex_) {
        // 情况 A：当前歌被移动了
        currentIndex_ = to;
    } else if (from < currentIndex_ && to >= currentIndex_) {
        // 情况 B：从当前歌前面移到当前歌后面
        currentIndex_--;
    } else if (from > currentIndex_ && to <= currentIndex_) {
        // 情况 C：从当前歌后面移到当前歌前面
        currentIndex_++;
    }
    // 其他情况：移动位置不影响当前歌的位置，不需要调整

    if (mode_ == PlayMode::Shuffle) {
        regenerateShuffleOrder();
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
// 播放导航
// ═══════════════════════════════════════════════════════════════

std::optional<SongInfo> PlayQueue::current() const {
    // 检查 currentIndex_ 是否有效
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(queue_.size())) {
        return std::nullopt;  // 没有当前歌曲
    }
    return queue_[currentIndex_];
}

std::optional<SongInfo> PlayQueue::next() {
    // 队列为空 → 没歌可播
    if (queue_.empty()) return std::nullopt;

    // 根据当前播放模式决定"下一首"的策略
    switch (mode_) {

        // ── Sequential 顺序播放 ──────────────────────────
        //
        // 逻辑：索引 + 1
        // 边界：超出队列末尾 → 播放结束，返回 nullopt
        //
        case PlayMode::Sequential: {
            int nextIdx = currentIndex_ + 1;
            if (nextIdx >= static_cast<int>(queue_.size())) {
                return std::nullopt;  // 播完即停，不循环
            }
            currentIndex_ = nextIdx;
            return queue_[currentIndex_];
        }

        // ── SingleLoop 单曲循环 ──────────────────────────
        //
        // 逻辑：索引不变，永远重复同一首
        // 边界：如果当前歌被删了，自动切到第 0 首
        //
        case PlayMode::SingleLoop: {
            if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(queue_.size())) {
                return queue_[currentIndex_];  // 还是同一首
            }
            // 当前歌曲不在有效范围内（可能被删了），跳转到第一首
            currentIndex_ = 0;
            return queue_[0];
        }

        // ── ListLoop 列表循环 ────────────────────────────
        //
        // 逻辑：(currentIndex_ + 1) % size
        // 效果：播完最后一首回到第一首，无限循环
        //
        case PlayMode::ListLoop: {
            int nextIdx = (currentIndex_ + 1) % static_cast<int>(queue_.size());
            currentIndex_ = nextIdx;
            return queue_[currentIndex_];
        }

        // ── Shuffle 随机播放 ─────────────────────────────
        //
        // 逻辑：
        //   1. 从 shuffleOrder_ 取下一项
        //   2. 如果所有歌都播过一遍 → 重新洗牌，开始新的一轮
        //   3. 根据 shuffleOrder_[shufflePos_] 找到 queue_ 中实际的歌
        //
        case PlayMode::Shuffle: {
            ensureShuffleOrder();      // 确保顺序有效
            shufflePos_++;             // 前进到下一首
            if (shufflePos_ >= static_cast<int>(shuffleOrder_.size())) {
                // 这一轮全部播完，重新洗牌
                regenerateShuffleOrder();
                shufflePos_ = 0;
            }
            currentIndex_ = shuffleOrder_[shufflePos_];
            return queue_[currentIndex_];
        }
    }
    return std::nullopt;
}

std::optional<SongInfo> PlayQueue::previous() {
    if (queue_.empty()) return std::nullopt;

    switch (mode_) {

        // ── Sequential 顺序播放 ──────────────────────────
        //
        // 逻辑：索引 - 1
        // 边界：小于 0 → 已经到头了，返回 nullopt
        //
        case PlayMode::Sequential: {
            int prevIdx = currentIndex_ - 1;
            if (prevIdx < 0) {
                return std::nullopt;  // 已经是第一首，没有上一首
            }
            currentIndex_ = prevIdx;
            return queue_[currentIndex_];
        }

        // ── SingleLoop 单曲循环 ──────────────────────────
        //
        // 逻辑：返回同一首
        //
        case PlayMode::SingleLoop: {
            if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(queue_.size())) {
                return queue_[currentIndex_];
            }
            currentIndex_ = 0;
            return queue_[0];
        }

        // ── ListLoop 列表循环 ────────────────────────────
        //
        // 逻辑：(currentIndex_ - 1 + size) % size
        // 效果：到头回到最后一首，无限循环
        //
        // 为什么 + size 再 % size？
        //   因为 C++ 的 % 运算符对于负数会得到负值，
        //   加 size 确保结果为正。例如：
        //   (0 - 1 + 5) % 5 = 4，正确回到最后一首
        //
        case PlayMode::ListLoop: {
            int size = static_cast<int>(queue_.size());
            int prevIdx = (currentIndex_ - 1 + size) % size;
            currentIndex_ = prevIdx;
            return queue_[currentIndex_];
        }

        // ── Shuffle 随机播放 ─────────────────────────────
        //
        // 逻辑：在 shuffle 顺序中回退一位
        // 边界：已在开头 → 回到上一轮末尾
        //
        case PlayMode::Shuffle: {
            ensureShuffleOrder();
            if (shufflePos_ > 0) {
                shufflePos_--;  // 正常回退一步
            } else {
                // 已经是最开头了，回到这一轮最后一首
                shufflePos_ = static_cast<int>(shuffleOrder_.size()) - 1;
            }
            currentIndex_ = shuffleOrder_[shufflePos_];
            return queue_[currentIndex_];
        }
    }
    return std::nullopt;
}

bool PlayQueue::setCurrentIndex(int index) {
    // 检查 index 是否在有效范围内
    if (index < 0 || index >= static_cast<int>(queue_.size())) {
        return false;
    }

    currentIndex_ = index;

    // Shuffle 模式下，需要在 shuffleOrder_ 中找到这个 index 的位置
    if (mode_ == PlayMode::Shuffle) {
        ensureShuffleOrder();

        // 在 shuffleOrder_ 中查找当前 index 出现的位置
        auto it = std::find(shuffleOrder_.begin(), shuffleOrder_.end(), index);
        if (it != shuffleOrder_.end()) {
            // 找到了，同步更新 shufflePos_
            shufflePos_ = static_cast<int>(it - shuffleOrder_.begin());
        } else {
            // 没找到（理论上不会发生，但以防万一），重新洗牌
            regenerateShuffleOrder();
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
// 播放模式
// ═══════════════════════════════════════════════════════════════

void PlayQueue::setPlayMode(PlayMode mode) {
    // 如果模式没变，不做任何操作
    if (mode_ == mode) return;

    mode_ = mode;

    if (mode == PlayMode::Shuffle) {
        // 进入 Shuffle 模式时，生成随机播放顺序
        // regenerateShuffleOrder 内部会把当前歌放在第一位
        regenerateShuffleOrder();
    } else {
        // 从 Shuffle 切出来时，清除 shuffle 相关数据
        shuffleOrder_.clear();
        shufflePos_ = 0;
    }
}

PlayMode PlayQueue::getPlayMode() const {
    return mode_;
}

// ═══════════════════════════════════════════════════════════════
// 状态查询
// ═══════════════════════════════════════════════════════════════

const std::vector<SongInfo>& PlayQueue::getQueue() const {
    return queue_;
}

int PlayQueue::getCurrentIndex() const {
    return currentIndex_;
}

int PlayQueue::size() const {
    return static_cast<int>(queue_.size());
}

bool PlayQueue::isEmpty() const {
    return queue_.empty();
}

// ═══════════════════════════════════════════════════════════════
// Shuffle 内部方法
// ═══════════════════════════════════════════════════════════════

void PlayQueue::regenerateShuffleOrder() {
    int size = static_cast<int>(queue_.size());

    // ─── 步骤 1：生成有序数组 [0, 1, 2, ..., size-1] ───────
    shuffleOrder_.resize(size);
    for (int i = 0; i < size; ++i) {
        shuffleOrder_[i] = i;
    }

    // ─── 步骤 2：用 Fisher-Yates 算法打乱 ──────────────────
    std::shuffle(shuffleOrder_.begin(), shuffleOrder_.end(), rng());

    // ─── 步骤 3：确保当前歌排在第一位 ──────────────────────
    //
    // 用户切换到 Shuffle 模式时，正在播的歌不应该被切掉。
    // 所以把当前歌的 index 换到 shuffleOrder_ 的开头。
    //
    shufflePos_ = 0;
    if (currentIndex_ >= 0 && currentIndex_ < size) {
        auto it = std::find(shuffleOrder_.begin(), shuffleOrder_.end(), currentIndex_);
        if (it != shuffleOrder_.end()) {
            // 找到当前歌在 shuffle 顺序中的位置，和第一个元素交换
            std::swap(shuffleOrder_[0], *it);
        }
    }
    // 现在 shuffleOrder_ = [当前歌, 随机1, 随机2, ...]
}

void PlayQueue::ensureShuffleOrder() {
    // 如果当前是 Shuffle 模式但 shuffleOrder_ 还没生成（或已被清空），
    // 调用 regenerateShuffleOrder() 初始化
    if (mode_ == PlayMode::Shuffle && shuffleOrder_.empty()) {
        regenerateShuffleOrder();
    }
}
