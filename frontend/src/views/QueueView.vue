<template>
  <div class="queue-view">
    <!-- 头部 -->
    <div class="queue-header">
      <div class="title-section">
        <h2>播放队列</h2>
        <span class="subtitle">{{ expandedCount }} 首歌 · {{ playerStore.queue.length }} 项</span>
      </div>
      <el-button
        type="danger"
        plain
        size="large"
        @click="handleClear"
        :disabled="playerStore.queue.length === 0"
      >
        清空队列
      </el-button>
    </div>

    <!-- 批量操作栏 -->
    <div v-if="selection.selectedCount > 0" class="batch-toolbar">
      <span class="batch-info">
        <el-icon><Check /></el-icon>
        已选择 {{ selection.selectedCount }} 项
      </span>
      <el-button size="small" type="danger" @click="handleRemoveSelected">删除</el-button>
    </div>

    <!-- 队列表格 -->
    <div class="table-wrapper" v-if="playerStore.queue.length > 0">
      <el-table
        ref="queueTable"
        :data="queueWithIndex"
        row-key="key"
        :row-class-name="getRowClass"
        class="queue-table"
        @row-dblclick="handleRowDblClick"
      >
        <!-- 复选框列 -->
        <el-table-column width="50">
          <template #header>
            <el-checkbox
              :model-value="selection.isAllSelected"
              :indeterminate="selection.isIndeterminate"
              @change="selection.handleSelectAllChange"
            />
          </template>
          <template #default="{ row }">
            <el-checkbox
              :model-value="selection.isSelected(row)"
              @mousedown.prevent="(e) => selection.startDrag(e, row)"
              @change="selection.toggle(row)"
              @mouseenter="selection.onItemEnter(row)"
            />
          </template>
        </el-table-column>

        <!-- # 列 -->
        <el-table-column width="60" align="center">
          <template #default="{ $index }">
            <div v-if="$index === playerStore.currentIndex" class="playing-badge">
              <el-icon color="#409eff" size="16"><VideoPlay /></el-icon>
            </div>
            <span v-else class="track-num">{{ $index + 1 }}</span>
          </template>
        </el-table-column>

        <!-- 信息列 -->
        <el-table-column label="歌曲 / 歌单" min-width="300">
          <template #default="{ row }">
            <!-- 歌单类型 -->
            <template v-if="row.type === 'playlist'">
              <div class="playlist-row">
                <span class="playlist-icon">🎵</span>
                <span class="playlist-name">{{ row.playlistName }}</span>
                <el-tag size="small" type="info" effect="plain">{{ row.songs?.length || 0 }}首</el-tag>
              </div>
            </template>
            <!-- 单曲类型 -->
            <template v-else>
              <span class="song-title">{{ row.song?.title || '(已删除)' }}</span>
              <span class="song-artist">{{ row.song?.artist || '' }}</span>
            </template>
          </template>
        </el-table-column>

        <!-- 时长 -->
        <el-table-column width="100" align="center">
          <template #default="{ row }">
            <span class="duration-cell" v-if="row.type === 'song'">
              {{ formatTime(row.song?.duration) }}
            </span>
            <span class="duration-cell" v-else>
              {{ formatTime(totalDuration(row.songs)) }}
            </span>
          </template>
        </el-table-column>

        <!-- 操作 -->
        <el-table-column width="200" fixed="right" align="center">
          <template #default="{ row }">
            <el-button
              v-if="row.type === 'song'"
              class="action-btn play-btn"
              type="primary"
              size="small"
              @click="handlePlaySong(row.song)"
            >
              <el-icon><VideoPlay /></el-icon> 播放
            </el-button>
            <el-button class="action-btn delete-btn" type="danger" size="small" @click="handleRemove(row._index)">
              <el-icon><Delete /></el-icon> 删除
            </el-button>
          </template>
        </el-table-column>

        <!-- 歌单展开 -->
        <el-table-column type="expand">
          <template #default="{ row }">
            <div v-if="row.type === 'playlist'" class="playlist-songs">
              <div
                v-for="(song, si) in row.songs"
                :key="si"
                class="playlist-song-item"
                @dblclick="handlePlaySong(song)"
              >
                <span class="song-idx">{{ si + 1 }}.</span>
                <span class="ps-title">{{ song.title }}</span>
                <span class="ps-artist">{{ song.artist }}</span>
                <span class="ps-duration">{{ formatTime(song.duration) }}</span>
              </div>
              <el-empty v-if="!row.songs || row.songs.length === 0" description="歌单为空" :image-size="40" />
            </div>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <el-empty v-else description="队列是空的，去音乐库添加歌曲吧" :image-size="80" />
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { usePlayerStore } from '../stores/player'
import { removeFromQueue, clearQueue, reorderQueue } from '../api/queue'
import { useSelection } from '../composables/useSelection'
import { useSortableRows } from '../composables/useSortableRows'
import { formatTime } from '../utils/format'
import { VideoPlay, Check, Delete } from '@element-plus/icons-vue'

const playerStore = usePlayerStore()
const queueTable = ref(null)

// 为每项生成稳定 key + 保留原始索引（不使用 Date.now()，避免不必要的 DOM 重建）
const queueWithIndex = computed(() =>
  playerStore.queue.map((item, i) => ({ ...item, key: `${item.type}-${i}`, _index: i }))
)

const selection = useSelection(computed(() => queueWithIndex.value), { key: 'key' })

const expandedCount = computed(() => {
  let count = 0
  for (const item of playerStore.queue) {
    if (item.type === 'song') count++
    else if (item.type === 'playlist') count += item.songs?.length || 0
  }
  return count
})

// 拖拽排序
useSortableRows(queueTable, {
  data: computed(() => queueWithIndex.value),
  onSort: async (oldIndex, newIndex) => {
    try {
      await reorderQueue(oldIndex, newIndex)
      await playerStore.refreshQueue()
    } catch {
      ElMessage.error('排序失败')
      await playerStore.refreshQueue()
    }
  },
})

onMounted(() => {
  playerStore.refreshQueue()
})

// ===== 事件处理 =====

function getRowClass({ row }) {
  return row.type === 'playlist' ? 'playlist-item-row' : ''
}

function handleRowDblClick(row) {
  if (row.type === 'song' && row.song) {
    playerStore.playSong(row.song)
  }
}

function handlePlaySong(song) {
  if (song) playerStore.playSong(song)
}

async function handleRemove(index) {
  try {
    await removeFromQueue(index)
    await playerStore.refreshQueue()
  } catch {
    ElMessage.error('移除失败')
  }
}

async function handleRemoveSelected() {
  const indices = selection.selectedItems
    .map(item => queueWithIndex.value.findIndex(q => q.key === item.key))
    .filter(i => i >= 0)
    .sort((a, b) => b - a)

  for (const idx of indices) {
    await removeFromQueue(idx)
  }
  selection.clear()
  await playerStore.refreshQueue()
  ElMessage.success(`已删除 ${indices.length} 项`)
}

async function handleClear() {
  try {
    await ElMessageBox.confirm('确定清空整个播放队列吗？', '确认', { type: 'warning' })
    await clearQueue()
    playerStore.queue = []
    playerStore.currentSong = null
    playerStore.currentPosition = 0
    playerStore.duration = 0
    playerStore.isPlaying = false
    playerStore.currentIndex = -1
  } catch (error) {
    if (error !== 'cancel') ElMessage.error('清空失败')
  }
}

function totalDuration(songs) {
  if (!songs) return 0
  return songs.reduce((sum, s) => sum + (s.duration || 0), 0)
}
</script>

<style scoped>
.queue-view {
  padding: 24px;
  max-width: 1200px;
}

/* ===== 头部 ===== */
.queue-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.title-section {
  display: flex;
  align-items: baseline;
  gap: 12px;
}

.title-section h2 {
  margin: 0;
  font-size: 22px;
  font-weight: 700;
  color: #303133;
}

.subtitle {
  font-size: 13px;
  color: #909399;
}

/* ===== 批量操作栏 ===== */
.batch-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
  padding: 10px 16px;
  background: linear-gradient(135deg, #fef0f0, #fde2e2);
  border: 1px solid #fde2e2;
  border-radius: 8px;
}

.batch-info {
  font-size: 13px;
  color: #f56c6c;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 6px;
}

/* ===== 表格 ===== */
.table-wrapper {
  background: #ffffff;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
}

.queue-table :deep(.el-table__header th) {
  background: #fafbfc;
  color: #606266;
  font-weight: 600;
  font-size: 13px;
  border-bottom: 2px solid #ebeef5;
}

.queue-table :deep(.el-table__body tr) {
  transition: background-color 0.15s;
}

.track-num {
  color: #909399;
  font-size: 13px;
}

.playing-badge {
  display: flex;
  align-items: center;
  justify-content: center;
}

.duration-cell {
  color: #909399;
  font-size: 13px;
  font-variant-numeric: tabular-nums;
}

/* ===== 歌单行 ===== */
.playlist-row {
  display: flex;
  align-items: center;
  gap: 8px;
  user-select: none;
}

.playlist-icon {
  font-size: 18px;
}

.playlist-name {
  font-weight: 500;
}

.song-title {
  margin-right: 12px;
  font-weight: 500;
}

.song-artist {
  color: #909399;
  font-size: 13px;
}

/* ===== 歌单展开 ===== */
.playlist-songs {
  padding: 8px 0;
}

.playlist-song-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 24px;
  cursor: pointer;
  border-radius: 4px;
  transition: background-color 0.15s;
}

.playlist-song-item:hover {
  background: #f0f2ff;
}

.song-idx {
  color: #909399;
  width: 28px;
  text-align: right;
  font-size: 13px;
}

.ps-title {
  flex: 1;
  font-size: 14px;
}

.ps-artist {
  color: #909399;
  font-size: 13px;
  width: 120px;
}

.ps-duration {
  color: #909399;
  font-size: 13px;
  width: 48px;
  text-align: right;
}

/* ===== 操作按钮 ===== */
.action-btn {
  border-radius: 6px;
  font-weight: 500;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  border: none;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.action-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

.action-btn:active {
  transform: translateY(0);
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.15);
}

.play-btn {
  background: linear-gradient(135deg, #667eea, #764ba2);
}

.play-btn:hover {
  background: linear-gradient(135deg, #764ba2, #667eea);
}

.delete-btn {
  background: linear-gradient(135deg, #f093fb, #f5576c);
}

.delete-btn:hover {
  background: linear-gradient(135deg, #f5576c, #f093fb);
}

/* ===== 行样式 ===== */
:deep(.playlist-item-row) {
  background-color: #f9fafb;
}

:deep(.sortable-ghost) {
  opacity: 0.4;
  background-color: #f5f7fa !important;
}

:deep(.sortable-chosen) {
  background-color: #ecf5ff !important;
}
</style>
