<template>
  <div class="playlist-detail-view">
    <!-- 头部 -->
    <div class="detail-header">
      <el-button text size="large" @click="$router.push('/playlists')" class="back-btn">
        <el-icon><ArrowLeft /></el-icon> 返回歌单列表
      </el-button>
    </div>

    <div class="detail-hero" v-if="store.current">
      <div class="hero-cover">
        <span class="hero-emoji">🎵</span>
      </div>
      <div class="hero-info">
        <div class="hero-label">歌单</div>
        <el-input
          v-if="editingName"
          ref="nameInput"
          v-model="editName"
          class="name-input"
          size="large"
          @blur="saveName"
          @keyup.enter="saveName"
        />
        <h2 v-else class="hero-name" @click="startEditName">{{ store.current.name }}</h2>
        <div class="hero-meta">
          <span>{{ store.current.songCount || 0 }} 首歌</span>
        </div>
        <div class="hero-actions">
          <el-button type="primary" size="large" @click="handleAddToQueue" :disabled="!store.current?.songCount">
            <el-icon><VideoPlay /></el-icon> 加入队列
          </el-button>
          <el-button v-if="store.current?.name !== '我喜欢'" type="danger" plain size="large" @click="handleDeletePlaylist">
            删除歌单
          </el-button>
        </div>
      </div>
    </div>

    <!-- 歌曲表格 -->
    <div class="table-wrapper" v-if="store.current">
      <el-table
        ref="tableRef"
        v-loading="store.loading"
        :data="store.current.songs"
        row-key="id"
        class="song-table"
        @row-dblclick="handlePlay"
      >
        <el-table-column label="#" width="60" align="center">
          <template #default="{ $index }">
            <span class="track-num">{{ $index + 1 }}</span>
          </template>
        </el-table-column>

        <!-- 收藏列 -->
        <el-table-column width="46" align="center">
          <template #default="{ row }">
            <el-icon
              class="fav-heart"
              :class="{ active: store.isFavorite(row.id) }"
              @click="handleToggleFavorite(row)"
            >
              <StarFilled v-if="store.isFavorite(row.id)" />
              <Star v-else />
            </el-icon>
          </template>
        </el-table-column>

        <el-table-column prop="title" label="歌名" min-width="200" show-overflow-tooltip />
        <el-table-column prop="artist" label="歌手" min-width="150" show-overflow-tooltip>
          <template #default="{ row }">
            <span class="artist-cell">{{ row.artist }}</span>
          </template>
        </el-table-column>
        <el-table-column label="时长" width="100" align="center">
          <template #default="{ row }">
            <span class="duration-cell">{{ formatTime(row.duration) }}</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="120" fixed="right" align="center">
          <template #default="{ row }">
            <el-button class="action-btn delete-btn" type="danger" size="small" @click="handleRemove(row)">
              <el-icon><Delete /></el-icon> 移除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <el-empty v-else-if="!store.loading" description="歌单不存在或已被删除" :image-size="80" />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch, nextTick } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { usePlaylistStore } from '../stores/playlist'
import { usePlayerStore } from '../stores/player'
import { addPlaylistToQueue } from '../api/queue'
import { ArrowLeft, VideoPlay, Delete, Star, StarFilled } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useSortableRows } from '../composables/useSortableRows'
import { formatTime } from '../utils/format'

const route = useRoute()
const router = useRouter()
const store = usePlaylistStore()
const playerStore = usePlayerStore()

const tableRef = ref(null)
const editingName = ref(false)
const editName = ref('')
const nameInput = ref(null)

const playlistId = computed(() => parseInt(route.params.id))

// 拖拽排序
useSortableRows(tableRef, {
  data: computed(() => store.current?.songs || []),
  onSort: async (oldIndex, newIndex) => {
    try {
      await store.reorder(playlistId.value, oldIndex, newIndex)
      await store.loadPlaylist(playlistId.value)
    } catch {
      ElMessage.error('排序失败')
      await store.loadPlaylist(playlistId.value)
    }
  },
})

// 路由参数变化时重新加载（Vue 会复用组件，onMounted 不会再次触发）
watch(playlistId, (newId) => {
  if (newId) store.loadPlaylist(newId)
}, { immediate: true })

function startEditName() {
  editName.value = store.current?.name || ''
  editingName.value = true
  nextTick(() => nameInput.value?.focus())
}

async function saveName() {
  editingName.value = false
  const name = editName.value.trim()
  if (name && name !== store.current?.name) {
    await store.rename(playlistId.value, name)
  }
}

async function handleRemove(song) {
  await store.removeSong(playlistId.value, song.id)
  ElMessage.success('已移除')
}

async function handleToggleFavorite(song) {
  try {
    await store.toggleFavorite(song.id)
    const isFav = store.isFavorite(song.id)
    ElMessage.success(isFav ? `已收藏「${song.title}」` : `已取消收藏「${song.title}」`)
  } catch {
    ElMessage.error('操作失败')
  }
}

async function handleAddToQueue() {
  const result = await addPlaylistToQueue(playlistId.value)
  await playerStore.refreshQueue()
  ElMessage.success(`已将歌单 "${store.current?.name}" 加入队列`)
}

async function handleDeletePlaylist() {
  await ElMessageBox.confirm('确定删除此歌单？', '确认', { type: 'warning' })
  await store.remove(playlistId.value)
  ElMessage.success('歌单已删除')
  router.push('/playlists')
}

function handlePlay(song) {
  playerStore.playSong(song)
}
</script>

<style scoped>
.playlist-detail-view {
  padding: 24px;
}

/* ===== 返回按钮 ===== */
.detail-header {
  margin-bottom: 20px;
}

.back-btn {
  font-size: 14px;
  color: #606266;
}

/* ===== Hero 区域 ===== */
.detail-hero {
  display: flex;
  align-items: flex-start;
  gap: 28px;
  margin-bottom: 28px;
  padding: 24px;
  background: #ffffff;
  border-radius: 16px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.05);
}

.hero-cover {
  width: 180px;
  height: 180px;
  border-radius: 14px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.3);
}

.hero-emoji {
  font-size: 64px;
  filter: drop-shadow(0 2px 6px rgba(0,0,0,0.2));
}

.hero-info {
  flex: 1;
  min-width: 0;
  padding-top: 8px;
}

.hero-label {
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 1px;
  color: #909399;
  margin-bottom: 8px;
}

.hero-name {
  margin: 0 0 12px 0;
  font-size: 28px;
  font-weight: 700;
  color: #303133;
  cursor: pointer;
  transition: color 0.15s;
}

.hero-name:hover {
  color: #409eff;
}

.name-input {
  width: 320px;
  margin-bottom: 12px;
}

.hero-meta {
  font-size: 14px;
  color: #909399;
  margin-bottom: 20px;
}

.hero-actions {
  display: flex;
  gap: 10px;
}

/* ===== 表格 ===== */
.table-wrapper {
  background: #ffffff;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
}

.song-table :deep(.el-table__header th) {
  background: #fafbfc;
  color: #606266;
  font-weight: 600;
  font-size: 13px;
  border-bottom: 2px solid #ebeef5;
}

.track-num {
  color: #909399;
  font-size: 13px;
}

.artist-cell {
  color: #606266;
  font-size: 13px;
}

.duration-cell {
  color: #909399;
  font-size: 13px;
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

.delete-btn {
  background: linear-gradient(135deg, #f07070, #b71c1c);
}

.delete-btn:hover {
  background: linear-gradient(135deg, #b71c1c, #f07070);
}

/* ===== 收藏爱心 ===== */
.fav-heart {
  font-size: 18px;
  cursor: pointer;
  color: #c0c4cc;
  transition: all 0.2s;
}

.fav-heart:hover {
  color: #f56c6c;
  transform: scale(1.2);
}

.fav-heart.active {
  color: #f56c6c;
}

.fav-heart.active:hover {
  color: #f78989;
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
