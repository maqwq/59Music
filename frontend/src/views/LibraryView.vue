<template>
  <div class="library-view">
    <!-- 统计卡片 -->
    <div class="stats-row">
      <div class="stat-card">
        <div class="stat-icon" style="background: linear-gradient(135deg, #667eea, #764ba2)">
          <el-icon size="20"><Headset /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ libraryStore.stats.totalSongs }}</div>
          <div class="stat-label">总歌曲</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: linear-gradient(135deg, #f093fb, #f5576c)">
          <el-icon size="20"><Timer /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ formatDuration(libraryStore.stats.totalDuration) }}</div>
          <div class="stat-label">总时长</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: linear-gradient(135deg, #4facfe, #00f2fe)">
          <el-icon size="20"><User /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ libraryStore.stats.totalArtists }}</div>
          <div class="stat-label">歌手</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon" style="background: linear-gradient(135deg, #43e97b, #38f9d7)">
          <el-icon size="20"><Folder /></el-icon>
        </div>
        <div class="stat-info">
          <div class="stat-value">{{ libraryStore.stats.totalAlbums }}</div>
          <div class="stat-label">专辑</div>
        </div>
      </div>
    </div>

    <!-- 操作栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-input
          v-model="scanPath"
          placeholder="输入文件夹路径，按回车扫描"
          clearable
          class="scan-input"
          @keyup.enter="handleScan"
        >
          <template #prefix>
            <el-icon><FolderOpened /></el-icon>
          </template>
        </el-input>
        <el-button type="primary" :loading="libraryStore.scanning" @click="handleScan">
          <el-icon><Search /></el-icon>
          扫描文件夹
        </el-button>
      </div>
      <div class="toolbar-right">
        <el-input
          v-model="searchKeyword"
          placeholder="搜索歌名 / 歌手 / 专辑"
          clearable
          class="search-input"
          @keyup.enter="handleSearch"
        >
          <template #prefix>
            <el-icon><Search /></el-icon>
          </template>
        </el-input>
        <el-button @click="handleSearch">搜索</el-button>
        <el-button @click="handleReset">重置</el-button>
      </div>
    </div>

    <!-- 批量操作栏 -->
    <div v-if="selection.selectedCount > 0" class="batch-toolbar">
      <span class="batch-info">
        <el-icon><Check /></el-icon>
        已选择 {{ selection.selectedCount }} 首歌曲
      </span>
      <div class="batch-actions">
        <el-button size="small" type="primary" @click="handleAddSelectedToQueue">
          加入队列
        </el-button>
        <el-button size="small" type="success" @click="showCreatePlaylistDialog = true">
          创建歌单
        </el-button>
        <el-button size="small" @click="showAddToPlaylistDialog = true">
          添加到歌单
        </el-button>
        <el-button size="small" type="danger" plain @click="handleDeleteSelected">
          删除
        </el-button>
      </div>
    </div>

    <!-- 歌曲表格 -->
    <div class="table-wrapper">
      <el-table
        ref="libraryTable"
        v-loading="libraryStore.loading"
        :data="libraryStore.songs"
        row-key="id"
        class="song-table"
        @row-dblclick="handlePlay"
      >
        <!-- 自定义选择列 -->
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

        <el-table-column label="歌名" min-width="200" show-overflow-tooltip>
          <template #default="{ row }">
            <span v-if="editingCell !== `title-${row.id}`" class="editable-cell" @click="startEdit(row, 'title')">
              {{ row.title }}
            </span>
            <el-input
              v-else
              :ref="(el) => setEditRef(`title-${row.id}`, el)"
              v-model="editValue"
              size="small"
              @blur="saveEdit(row)"
              @keyup.enter="saveEdit(row)"
            />
          </template>
        </el-table-column>
        <el-table-column label="歌手" min-width="140" show-overflow-tooltip>
          <template #default="{ row }">
            <span v-if="editingCell !== `artist-${row.id}`" class="editable-cell" @click="startEdit(row, 'artist')">
              {{ row.artist }}
            </span>
            <el-input
              v-else
              :ref="(el) => setEditRef(`artist-${row.id}`, el)"
              v-model="editValue"
              size="small"
              @blur="saveEdit(row)"
              @keyup.enter="saveEdit(row)"
            />
          </template>
        </el-table-column>
        <el-table-column label="专辑" min-width="140" show-overflow-tooltip>
          <template #default="{ row }">
            <span class="album-cell">{{ row.album }}</span>
          </template>
        </el-table-column>
        <el-table-column label="时长" width="90" align="center">
          <template #default="{ row }">
            <span class="duration-cell">{{ formatTime(row.duration) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="260" fixed="right">
          <template #default="{ row }">
            <el-button class="action-btn play-btn" type="primary" size="small" @click="handlePlay(row)">
              <el-icon><VideoPlay /></el-icon> 播放
            </el-button>
            <el-button class="action-btn queue-btn" type="success" size="small" @click="handleAddToQueue(row)">
              <el-icon><Plus /></el-icon> 加入队列
            </el-button>
            <el-button class="action-btn delete-btn" type="danger" size="small" @click="handleDelete(row)">
              <el-icon><Delete /></el-icon> 删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <!-- 分页 -->
    <div class="pagination">
      <el-pagination
        v-model:current-page="libraryStore.page"
        v-model:page-size="libraryStore.pageSize"
        :total="libraryStore.total"
        :page-sizes="[20, 50, 100]"
        layout="total, sizes, prev, pager, next"
        background
        @change="handlePageChange"
      />
    </div>

    <!-- 创建歌单弹框 -->
    <el-dialog v-model="showCreatePlaylistDialog" title="创建歌单" width="420px" :close-on-click-modal="false">
      <el-input v-model="newPlaylistName" placeholder="输入歌单名称" size="large" />
      <template #footer>
        <el-button @click="showCreatePlaylistDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreatePlaylist">创建</el-button>
      </template>
    </el-dialog>

    <!-- 添加到已有歌单弹框 -->
    <el-dialog v-model="showAddToPlaylistDialog" title="添加到歌单" width="420px" :close-on-click-modal="false">
      <el-select v-model="targetPlaylistId" placeholder="选择歌单" style="width: 100%" size="large">
        <el-option
          v-for="pl in playlistStore.playlists"
          :key="pl.id"
          :label="`${pl.name} (${pl.songCount}首)`"
          :value="pl.id"
        />
      </el-select>
      <template #footer>
        <el-button @click="showAddToPlaylistDialog = false">取消</el-button>
        <el-button type="primary" @click="handleAddToPlaylist" :disabled="!targetPlaylistId">添加</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, computed, nextTick } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useLibraryStore } from '../stores/library'
import { usePlayerStore } from '../stores/player'
import { usePlaylistStore } from '../stores/playlist'
import { addToQueue } from '../api/queue'
import { scanFolder, getSongs, deleteSong, getStats, reorderSongs, updateSongMeta } from '../api/library'
import { useSelection } from '../composables/useSelection'
import { useSortableRows } from '../composables/useSortableRows'
import { formatTime, formatDuration } from '../utils/format'
import {
  Headset,
  Timer,
  User,
  Folder,
  FolderOpened,
  Search,
  Check,
  VideoPlay,
  Plus,
  Delete,
} from '@element-plus/icons-vue'

const libraryStore = useLibraryStore()
const playerStore = usePlayerStore()
const playlistStore = usePlaylistStore()

const libraryTable = ref(null)
const scanPath = ref('')
const searchKeyword = ref('')

const selection = useSelection(computed(() => libraryStore.songs), { key: 'id' })

useSortableRows(libraryTable, {
  data: computed(() => libraryStore.songs),
  onSort: async (oldIndex, newIndex) => {
    try {
      await reorderSongs(oldIndex, newIndex)
      await libraryStore.loadSongs()
      ElMessage.success('排序已更新')
    } catch {
      ElMessage.error('排序失败')
      await libraryStore.loadSongs()
    }
  },
})

onMounted(() => {
  libraryStore.loadSongs()
  libraryStore.loadStats()
  playlistStore.loadPlaylists()
})

// ===== 内联编辑 =====

const editingCell = ref(null)   // "title-{id}" | "artist-{id}" | null
const editValue = ref('')
const editRefs = {}

function setEditRef(key, el) {
  if (el) editRefs[key] = el
}

function startEdit(row, field) {
  editingCell.value = `${field}-${row.id}`
  editValue.value = row[field]
  nextTick(() => {
    const input = editRefs[`${field}-${row.id}`]
    if (input && typeof input.focus === 'function') input.focus()
  })
}

async function saveEdit(row) {
  const key = editingCell.value
  if (!key) return
  const field = key.startsWith('title-') ? 'title' : 'artist'
  const newValue = editValue.value.trim()
  editingCell.value = null

  if (newValue && newValue !== row[field]) {
    try {
      await updateSongMeta(row.id, field === 'title' ? newValue : row.title, field === 'artist' ? newValue : row.artist)
      row.title = field === 'title' ? newValue : row.title
      row.artist = field === 'artist' ? newValue : row.artist
      ElMessage.success('已更新')
    } catch {
      ElMessage.error('更新失败')
    }
  }
}

// ===== 歌单弹框 =====

const showCreatePlaylistDialog = ref(false)
const showAddToPlaylistDialog = ref(false)
const newPlaylistName = ref('')
const targetPlaylistId = ref(null)

async function handleCreatePlaylist() {
  const name = newPlaylistName.value.trim()
  if (!name) return
  const ids = selection.selectedIds
  if (ids.length === 0) {
    ElMessage.warning('请先选择歌曲')
    return
  }
  try {
    await playlistStore.create(name, ids)
    showCreatePlaylistDialog.value = false
    newPlaylistName.value = ''
    selection.clear()
    ElMessage.success(`歌单 "${name}" 已创建`)
  } catch {
    ElMessage.error('创建失败')
  }
}

async function handleAddToPlaylist() {
  if (!targetPlaylistId.value) return
  const ids = selection.selectedIds
  try {
    await playlistStore.addSongs(targetPlaylistId.value, ids)
    showAddToPlaylistDialog.value = false
    targetPlaylistId.value = null
    selection.clear()
    ElMessage.success(`已将 ${ids.length} 首歌加入歌单`)
  } catch {
    ElMessage.error('添加失败')
  }
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
  const ids = selection.selectedIds
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
  const count = selection.selectedCount
  if (count === 0) return
  try {
    await ElMessageBox.confirm(
      `确定从音乐库删除选中的 ${count} 首歌曲吗？原文件不会被删除。`,
      '批量删除确认',
      { confirmButtonText: '删除', cancelButtonText: '取消', type: 'warning' }
    )
    await Promise.all(
      selection.selectedItems.map((song) => libraryStore.deleteSong(song.id))
    )
    selection.clear()
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
</script>

<style scoped>
.library-view {
  padding: 24px;
  max-width: 1400px;
}

/* ===== 统计卡片 ===== */
.stats-row {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
  margin-bottom: 24px;
}

.stat-card {
  background: #ffffff;
  border-radius: 12px;
  padding: 20px 24px;
  display: flex;
  align-items: center;
  gap: 16px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
  transition: transform 0.2s, box-shadow 0.2s;
}

.stat-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.08);
}

.stat-icon {
  width: 44px;
  height: 44px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ffffff;
  flex-shrink: 0;
}

.stat-value {
  font-size: 22px;
  font-weight: 700;
  color: #303133;
  line-height: 1.2;
}

.stat-label {
  font-size: 13px;
  color: #909399;
  margin-top: 2px;
}

/* ===== 操作栏 ===== */
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  gap: 16px;
}

.toolbar-left,
.toolbar-right {
  display: flex;
  align-items: center;
  gap: 10px;
}

.scan-input {
  width: 300px;
}

.search-input {
  width: 240px;
}

/* ===== 批量操作栏 ===== */
.batch-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
  padding: 10px 16px;
  background: linear-gradient(135deg, #ecf5ff, #e8f4fd);
  border: 1px solid #d9ecff;
  border-radius: 8px;
}

.batch-info {
  font-size: 13px;
  color: #409eff;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 6px;
}

.batch-actions {
  display: flex;
  gap: 8px;
}

/* ===== 表格 ===== */
.table-wrapper {
  background: #ffffff;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
}

.song-table {
  width: 100%;
}

.song-table :deep(.el-table__header th) {
  background: #fafbfc;
  color: #606266;
  font-weight: 600;
  font-size: 13px;
  border-bottom: 2px solid #ebeef5;
}

.song-table :deep(.el-table__body tr) {
  transition: background-color 0.15s;
}

.album-cell {
  color: #909399;
  font-size: 13px;
}

.duration-cell {
  color: #909399;
  font-size: 13px;
  font-variant-numeric: tabular-nums;
}

/* ===== 分页 ===== */
.pagination {
  display: flex;
  justify-content: flex-end;
  margin-top: 16px;
}

/* ===== 内联编辑 ===== */
.editable-cell {
  cursor: pointer;
  display: inline-block;
  width: 100%;
  padding: 3px 6px;
  border-radius: 4px;
  transition: background-color 0.15s;
}

.editable-cell:hover {
  background-color: #ecf5ff;
  color: #409eff;
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

.queue-btn {
  background: linear-gradient(135deg, #43e97b, #38f9d7);
  color: #fff;
}

.queue-btn:hover {
  background: linear-gradient(135deg, #38f9d7, #43e97b);
}

.delete-btn {
  background: linear-gradient(135deg, #f093fb, #f5576c);
}

.delete-btn:hover {
  background: linear-gradient(135deg, #f5576c, #f093fb);
}

/* ===== 拖拽排序 ===== */
:deep(.sortable-ghost) {
  opacity: 0.4;
  background-color: #f5f7fa !important;
}

:deep(.sortable-chosen) {
  background-color: #ecf5ff !important;
}
</style>
