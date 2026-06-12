<template>
  <div class="queue-view">
    <!-- 标题栏 -->
    <div class="queue-header">
      <div class="title-section">
        <h2>播放队列</h2>
        <span class="subtitle">共 {{ playerStore.queue.length }} 首</span>
      </div>
      <el-button type="danger" :disabled="playerStore.queue.length === 0" @click="handleClear">
        清空队列
      </el-button>
    </div>

    <!-- 批量操作栏 -->
    <div v-if="selection.selectedCount > 0" class="batch-toolbar">
      <span class="batch-info">已选择 {{ selection.selectedCount }} 首歌曲</span>
      <el-button type="danger" @click="handleRemoveSelected">
        删除
      </el-button>
    </div>

    <!-- 队列表格 -->
    <el-table
      ref="queueTable"
      :data="queueWithIndex"
      row-key="index"
      stripe
      style="width: 100%"
      :row-class-name="rowClassName"
      @row-dblclick="handlePlay"
    >
      <!-- 自定义选择列 -->
      <el-table-column width="55">
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

      <el-table-column label="#" width="60">
        <template #default="{ row, $index }">
          <span v-if="$index === playerStore.currentIndex" class="playing-indicator">
            <el-icon><Video-Play /></el-icon>
          </span>
          <span v-else>{{ $index + 1 }}</span>
        </template>
      </el-table-column>

      <el-table-column prop="title" label="歌名" min-width="180" />

      <el-table-column prop="artist" label="歌手" min-width="140" />

      <el-table-column prop="album" label="专辑" min-width="160" />

      <el-table-column label="时长" width="100">
        <template #default="{ row }">
          {{ formatTime(row.duration) }}
        </template>
      </el-table-column>

      <el-table-column label="操作" width="160" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" @click="handlePlay(row)">播放</el-button>
          <el-button link type="danger" @click="handleRemove(row.index)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 空状态 -->
    <el-empty v-if="playerStore.queue.length === 0" description="队列是空的，去音乐库添加歌曲吧" />
  </div>
</template>

<script setup>
import { onMounted, computed } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { VideoPlay } from '@element-plus/icons-vue'
import { usePlayerStore } from '../stores/player'
import { removeFromQueue, clearQueue, reorderQueue } from '../api/queue'
import { useSelection } from '../composables/useSelection'
import { useSortableRows } from '../composables/useSortableRows'

const playerStore = usePlayerStore()

const queueTable = ref(null)

/**
 * 队列表格数据，补充 index 字段
 * 队列中同一首歌可能出现多次，所以用 index 作为唯一 key
 */
const queueWithIndex = computed(() =>
  playerStore.queue.map((song, index) => ({ ...song, index }))
)

const selection = useSelection(queueWithIndex, { key: 'index' })

useSortableRows(queueTable, {
  data: computed(() => playerStore.queue),
  onSort: async (oldIndex, newIndex) => {
    try {
      await reorderQueue(oldIndex, newIndex)
      await playerStore.refreshQueue()
      ElMessage.success('队列顺序已更新')
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

function handlePlay(row) {
  const song = { ...row }
  delete song.index
  playerStore.playSong(song)
  ElMessage.success(`开始播放：${song.title}`)
}

async function handleRemove(index) {
  try {
    await removeFromQueue(index)
    await playerStore.refreshQueue()
    ElMessage.success('已从队列移除')
  } catch {
    ElMessage.error('移除失败')
  }
}

async function handleRemoveSelected() {
  const count = selection.selectedCount
  if (count === 0) return

  try {
    await ElMessageBox.confirm(
      `确定从队列中移除选中的 ${count} 首歌曲吗？`,
      '批量移除确认',
      { confirmButtonText: '移除', cancelButtonText: '取消', type: 'warning' }
    )

    // 按索引从大到小删除，避免删除过程中索引变化导致删错
    const indices = selection.selectedItems
      .map((item) => item.index)
      .sort((a, b) => b - a)

    for (const index of indices) {
      await removeFromQueue(index)
    }

    selection.clear()
    await playerStore.refreshQueue()
    ElMessage.success(`已移除 ${count} 首歌曲`)
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('移除失败')
    }
  }
}

async function handleClear() {
  try {
    await ElMessageBox.confirm('确定清空当前播放队列吗？', '清空确认', {
      confirmButtonText: '清空',
      cancelButtonText: '取消',
      type: 'warning',
    })
    await clearQueue()
    await playerStore.refreshQueue()
    playerStore.currentSong = null
    playerStore.currentPosition = 0
    playerStore.duration = 0
    playerStore.isPlaying = false
    playerStore.currentIndex = -1
    selection.clear()
    ElMessage.success('队列已清空')
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('清空失败')
    }
  }
}

// ===== 表格高亮当前播放行 =====

function rowClassName({ rowIndex }) {
  if (rowIndex === playerStore.currentIndex) {
    return 'current-playing-row'
  }
  return ''
}

// ===== 格式化 =====

function formatTime(seconds) {
  if (!seconds || seconds < 0) return '00:00'
  const m = Math.floor(seconds / 60)
  const s = Math.floor(seconds % 60)
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`
}
</script>

<style scoped>
.queue-view {
  padding: 20px;
}

.queue-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.title-section {
  display: flex;
  align-items: baseline;
  gap: 12px;
}

.title-section h2 {
  margin: 0;
  font-size: 20px;
}

.subtitle {
  font-size: 13px;
  color: #909399;
}

.batch-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
  padding: 8px 12px;
  background-color: #ecf5ff;
  border: 1px solid #d9ecff;
  border-radius: 4px;
}

.batch-info {
  font-size: 13px;
  color: #606266;
  margin-right: 8px;
}

.playing-indicator {
  color: #409eff;
  display: flex;
  align-items: center;
}

:deep(.current-playing-row) {
  background-color: #ecf5ff !important;
}

:deep(.current-playing-row td) {
  background-color: #ecf5ff !important;
}

:deep(.sortable-ghost) {
  opacity: 0.5;
  background-color: #f5f7fa !important;
}

:deep(.sortable-chosen) {
  background-color: #ecf5ff !important;
}
</style>
