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

    <!-- 队列表格 -->
    <el-table
      ref="queueTable"
      :data="playerStore.queue"
      row-key="id"
      stripe
      style="width: 100%"
      :row-class-name="rowClassName"
      @row-dblclick="handlePlay"
    >
      <el-table-column label="#" width="60">
        <template #default="{ $index }">
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
        <template #default="{ row, $index }">
          <el-button link type="primary" @click="handlePlay(row)">播放</el-button>
          <el-button link type="danger" @click="handleRemove($index)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 空状态 -->
    <el-empty v-if="playerStore.queue.length === 0" description="队列是空的，去音乐库添加歌曲吧" />
  </div>
</template>

<script setup>
import { onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { VideoPlay } from '@element-plus/icons-vue'
import { usePlayerStore } from '../stores/player'
import { removeFromQueue, clearQueue, reorderQueue } from '../api/queue'

const playerStore = usePlayerStore()

onMounted(() => {
  playerStore.refreshQueue()
})

// ===== 事件处理 =====

function handlePlay(song) {
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
    ElMessage.success('队列已清空')
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('清空失败')
    }
  }
}

// ===== 表格高亮当前播放行 =====

function rowClassName({ row, rowIndex }) {
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
</style>
