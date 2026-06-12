<template>
  <div class="library-view">
    <!-- 统计信息 -->
    <div class="stats-row">
      <el-statistic title="总歌曲" :value="libraryStore.stats.totalSongs" />
      <el-statistic title="总时长" :value="formatDuration(libraryStore.stats.totalDuration)" />
      <el-statistic title="歌手" :value="libraryStore.stats.totalArtists" />
      <el-statistic title="专辑" :value="libraryStore.stats.totalAlbums" />
    </div>

    <!-- 操作栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-input
          v-model="scanPath"
          placeholder="输入要扫描的文件夹路径（mock 阶段）"
          clearable
          style="width: 320px"
          @keyup.enter="handleScan"
        />
        <el-button type="primary" :loading="libraryStore.scanning" @click="handleScan">
          扫描文件夹
        </el-button>
      </div>
      <div class="toolbar-right">
        <el-input
          v-model="searchKeyword"
          placeholder="搜索歌名 / 歌手 / 专辑"
          clearable
          style="width: 240px"
          @keyup.enter="handleSearch"
        />
        <el-button @click="handleSearch">搜索</el-button>
        <el-button @click="handleReset">重置</el-button>
      </div>
    </div>

    <!-- 批量操作栏 -->
    <div v-if="selectedSongs.length > 0" class="batch-toolbar">
      <span class="batch-info">已选择 {{ selectedSongs.length }} 首歌曲</span>
      <el-button type="primary" @click="handleAddSelectedToQueue">
        加入队列
      </el-button>
      <el-button type="danger" @click="handleDeleteSelected">
        删除
      </el-button>
    </div>

    <!-- 歌曲表格 -->
    <el-table
      v-loading="libraryStore.loading"
      :data="libraryStore.songs"
      stripe
      style="width: 100%"
      @row-dblclick="handlePlay"
      @selection-change="handleSelectionChange"
    >
      <el-table-column type="selection" width="55" />
      <el-table-column prop="title" label="歌名" min-width="180" />
      <el-table-column prop="artist" label="歌手" min-width="140" />
      <el-table-column prop="album" label="专辑" min-width="160" />
      <el-table-column label="时长" width="100">
        <template #default="{ row }">
          {{ formatTime(row.duration) }}
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200" fixed="right">
        <template #default="{ row }">
          <el-button link type="primary" @click="handlePlay(row)">播放</el-button>
          <el-button link type="primary" @click="handleAddToQueue(row)">加入队列</el-button>
          <el-button link type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 分页 -->
    <div class="pagination">
      <el-pagination
        v-model:current-page="libraryStore.page"
        v-model:page-size="libraryStore.pageSize"
        :total="libraryStore.total"
        :page-sizes="[20, 50, 100]"
        layout="total, sizes, prev, pager, next"
        @change="handlePageChange"
      />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useLibraryStore } from '../stores/library'
import { usePlayerStore } from '../stores/player'
import { addToQueue } from '../api/queue'

const libraryStore = useLibraryStore()
const playerStore = usePlayerStore()

const scanPath = ref('')
const searchKeyword = ref('')
const selectedSongs = ref([])

onMounted(() => {
  libraryStore.loadSongs()
  libraryStore.loadStats()
})

// ===== 选择变化 =====

function handleSelectionChange(selection) {
  selectedSongs.value = selection
}

// ===== 事件处理 =====

function handleSearch() {
  libraryStore.setKeyword(searchKeyword.value)
}

function handleReset() {
  searchKeyword.value = ''
  libraryStore.resetSearch()
}

async function handleScan() {
  const folder = scanPath.value.trim()
  if (!folder) {
    ElMessage.warning('请输入文件夹路径')
    return
  }
  try {
    const result = await libraryStore.scanFolder(folder)
    ElMessage.success(`扫描完成，新增 ${result.addedCount} 首歌曲`)
  } catch {
    ElMessage.error('扫描失败')
  }
}

function handlePlay(song) {
  playerStore.playSong(song)
  ElMessage.success(`开始播放：${song.title}`)
}

async function handleAddToQueue(song) {
  try {
    await addToQueue([song.id])
    await playerStore.refreshQueue()
    ElMessage.success(`已加入队列：${song.title}`)
  } catch {
    ElMessage.error('加入队列失败')
  }
}

async function handleDelete(song) {
  try {
    await ElMessageBox.confirm(
      `确定从音乐库删除《${song.title}》吗？原文件不会被删除。`,
      '删除确认',
      { confirmButtonText: '删除', cancelButtonText: '取消', type: 'warning' }
    )
    await libraryStore.deleteSong(song.id)
    ElMessage.success('删除成功')
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

async function handleAddSelectedToQueue() {
  const ids = selectedSongs.value.map((song) => song.id)
  if (ids.length === 0) return
  try {
    await addToQueue(ids)
    await playerStore.refreshQueue()
    ElMessage.success(`已将 ${ids.length} 首歌曲加入队列`)
  } catch {
    ElMessage.error('加入队列失败')
  }
}

async function handleDeleteSelected() {
  const count = selectedSongs.value.length
  if (count === 0) return
  try {
    await ElMessageBox.confirm(
      `确定从音乐库删除选中的 ${count} 首歌曲吗？原文件不会被删除。`,
      '批量删除确认',
      { confirmButtonText: '删除', cancelButtonText: '取消', type: 'warning' }
    )
    await Promise.all(
      selectedSongs.value.map((song) => libraryStore.deleteSong(song.id))
    )
    selectedSongs.value = []
    ElMessage.success(`已删除 ${count} 首歌曲`)
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

function handlePageChange() {
  libraryStore.loadSongs()
}

// ===== 格式化 =====

function formatTime(seconds) {
  if (!seconds || seconds < 0) return '00:00'
  const m = Math.floor(seconds / 60)
  const s = Math.floor(seconds % 60)
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`
}

function formatDuration(seconds) {
  if (!seconds || seconds < 0) return '00:00'
  const h = Math.floor(seconds / 3600)
  const m = Math.floor((seconds % 3600) / 60)
  if (h > 0) {
    return `${h}小时${m}分钟`
  }
  return `${m}分钟`
}
</script>

<style scoped>
.library-view {
  padding: 20px;
}

.stats-row {
  display: flex;
  gap: 40px;
  margin-bottom: 20px;
  padding: 16px 20px;
  background-color: #f5f7fa;
  border-radius: 8px;
}

.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.toolbar-left,
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 12px;
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

.pagination {
  display: flex;
  justify-content: flex-end;
  margin-top: 16px;
}
</style>
