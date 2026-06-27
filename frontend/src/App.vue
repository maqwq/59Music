<template>
  <el-container class="app-layout">
    <!-- 深色侧边栏 -->
    <el-aside width="220px" class="sidebar">
      <div class="logo">
        <span class="logo-icon">🎵</span>
        <span class="logo-text">59Music</span>
      </div>

      <!-- 固定导航区 -->
      <el-menu
        :default-active="sidebarActive"
        router
        class="nav-menu"
        background-color="#1e1e2d"
        text-color="#a0a0b8"
        active-text-color="#ffffff"
      >
        <el-menu-item index="/">
          <el-icon><Headset /></el-icon>
          <span>音乐库</span>
        </el-menu-item>
        <el-menu-item v-if="playlistStore.favoritePlaylistId" :index="`/playlists/${playlistStore.favoritePlaylistId}`">
          <el-icon color="#f56c6c"><Star /></el-icon>
          <span>我喜欢</span>
        </el-menu-item>
      </el-menu>

      <!-- 歌单分区 -->
      <div class="playlist-section">
        <div class="playlist-header" @click="$router.push('/playlists')">
          <el-icon><Menu /></el-icon>
          <span>歌单</span>
          <el-icon
            class="expand-arrow"
            :class="{ expanded: playlistExpanded }"
            @click.stop="playlistExpanded = !playlistExpanded"
          >
            <ArrowDown v-if="playlistExpanded" />
            <ArrowRight v-else />
          </el-icon>
        </div>

        <transition name="expand">
          <div v-show="playlistExpanded" class="playlist-list">
            <div
              v-for="pl in playlistsExceptFavorite"
              :key="pl.id"
              class="playlist-item"
              :class="{ active: $route.path === `/playlists/${pl.id}` }"
              @click="$router.push(`/playlists/${pl.id}`)"
              @contextmenu.prevent="(e) => showPlaylistMenu(e, pl)"
            >
              <span class="playlist-item-icon">🎵</span>
              <span class="playlist-item-name">{{ pl.name }}</span>
              <span class="playlist-item-count">{{ pl.songCount }}</span>
            </div>

            <div v-if="playlistsExceptFavorite.length === 0" class="playlist-empty">
              暂无歌单
            </div>

            <div class="playlist-add" @click="handleCreatePlaylist">
              <el-icon><Plus /></el-icon>
              <span>新建歌单</span>
            </div>
          </div>
        </transition>
      </div>

      <!-- 歌单右键菜单 -->
      <div
        v-if="contextMenu.visible"
        class="context-menu"
        :style="{ top: contextMenu.y + 'px', left: contextMenu.x + 'px' }"
        @click.stop
      >
        <div class="context-menu-item" @click="handleRenamePlaylist">
          <el-icon><Edit /></el-icon> 重命名
        </div>
        <div
          v-if="contextMenu.playlist?.name !== '我喜欢'"
          class="context-menu-item danger"
          @click="handleDeletePlaylist"
        >
          <el-icon><Delete /></el-icon> 删除
        </div>
      </div>

      <!-- 底部快捷键提示 -->
      <div class="sidebar-footer">
        <div class="shortcut-hint">
          <div class="hint-row"><kbd>Space</kbd> 播放/暂停</div>
          <div class="hint-row"><kbd>← →</kbd> 切歌</div>
          <div class="hint-row"><kbd>↑ ↓</kbd> 音量</div>
        </div>
      </div>
    </el-aside>

    <!-- 主内容区 -->
    <el-container class="main-area">
      <el-main class="content">
        <router-view />
      </el-main>

      <!-- 底部播放栏（玻璃态） -->
      <el-footer height="96px" class="player-bar">
        <!-- 左侧：歌曲信息 + 专辑封面占位 -->
        <div class="song-info">
          <div class="cover-art" :class="{ active: playerStore.hasCurrentSong }">
            <span v-if="playerStore.hasCurrentSong" class="cover-note">🎶</span>
            <span v-else class="cover-note muted">♪</span>
          </div>
          <div class="song-meta">
            <div class="title">{{ playerStore.hasCurrentSong ? playerStore.currentSong.title : '暂无歌曲' }}</div>
            <div class="artist">
              {{ playerStore.hasCurrentSong ? playerStore.currentSong.artist : '选择一首歌曲开始播放' }}
            </div>
          </div>
          <el-icon
            v-if="playerStore.hasCurrentSong"
            class="bar-fav-heart"
            :class="{ active: playlistStore.isFavorite(playerStore.currentSong.id) }"
            @click="handleBarToggleFavorite"
          >
            <StarFilled v-if="playlistStore.isFavorite(playerStore.currentSong.id)" />
            <Star v-else />
          </el-icon>
        </div>

        <!-- 中间：控制按钮 + 进度 -->
        <div class="player-center">
          <div class="controls">
            <el-button
              circle
              class="ctrl-btn"
              :disabled="!playerStore.hasCurrentSong"
              @click="playerStore.previous"
              title="上一首 (←)"
            >
              <el-icon size="18"><ArrowLeft /></el-icon>
            </el-button>
            <el-button
              circle
              class="ctrl-btn play-btn"
              :disabled="!playerStore.hasCurrentSong"
              @click="playerStore.togglePlay"
              title="播放/暂停 (Space)"
            >
              <el-icon size="24">
                <VideoPlay v-if="!playerStore.isPlaying" />
                <VideoPause v-else />
              </el-icon>
            </el-button>
            <el-button
              circle
              class="ctrl-btn"
              :disabled="!playerStore.hasCurrentSong"
              @click="playerStore.next"
              title="下一首 (→)"
            >
              <el-icon size="18"><ArrowRight /></el-icon>
            </el-button>
          </div>

          <div class="progress-area">
            <span class="time">{{ playerStore.formattedPosition }}</span>
            <el-slider
              v-model="progressValue"
              :max="100"
              :disabled="!playerStore.hasCurrentSong"
              size="small"
              class="progress-slider"
              @change="onSeek"
            />
            <span class="time">{{ playerStore.formattedDuration }}</span>
          </div>
        </div>

        <!-- 右侧：音量 + 模式 -->
        <div class="player-extra">
          <el-tooltip :content="modeText" placement="top">
            <el-button
              circle
              class="ctrl-btn"
              size="small"
              :disabled="!playerStore.hasCurrentSong"
              @click="cycleMode"
            >
              <el-icon size="16">
                <RefreshRight v-if="playerStore.mode === 'single_loop'" />
                <Refresh v-else-if="playerStore.mode === 'list_loop'" />
                <Sort v-else-if="playerStore.mode === 'shuffle'" />
                <ArrowLeftBold v-else-if="playerStore.mode === 'reverse'" />
                <ArrowRight v-else />
              </el-icon>
            </el-button>
          </el-tooltip>

          <el-button
            circle
            class="ctrl-btn"
            size="small"
            @click="playerStore.toggleMuted"
            title="静音"
          >
            <el-icon size="16">
              <Mute v-if="playerStore.muted || playerStore.volume === 0" />
              <Microphone v-else />
            </el-icon>
          </el-button>

          <el-slider
            v-model="volumeValue"
            :max="100"
            size="small"
            class="volume-slider"
            @change="playerStore.setVolume"
          />

          <el-tooltip content="播放列表" placement="top">
            <el-button
              circle
              class="ctrl-btn"
              size="small"
              @click="queueDrawerVisible = true"
            >
              <el-icon size="16"><List /></el-icon>
            </el-button>
          </el-tooltip>
        </div>
      </el-footer>

      <!-- 播放列表抽屉 -->
      <el-drawer
        v-model="queueDrawerVisible"
        title="播放列表"
        direction="rtl"
        size="420px"
        :modal="true"
        :append-to-body="true"
        class="queue-drawer"
      >
        <template #header>
          <div class="drawer-header">
            <span class="drawer-title">播放列表</span>
            <span class="drawer-subtitle">{{ expandedCount }} 首歌 · {{ playerStore.queue.length }} 项</span>
          </div>
        </template>
        <template #header-extra>
          <el-button
            type="danger"
            plain
            size="small"
            @click="handleClearQueue"
            :disabled="playerStore.queue.length === 0"
          >
            清空
          </el-button>
        </template>

        <div ref="drawerListRef" class="drawer-queue-list" v-if="playerStore.queue.length > 0">
          <div
            v-for="(item, index) in playerStore.queue"
            :key="item.type === 'song' ? `song-${item.songId}` : `playlist-${item.playlistId}`"
            class="drawer-queue-item"
            :class="{ active: index === playerStore.currentIndex }"
            @dblclick="item.type === 'song' && item.song && playerStore.playSong(item.song)"
          >
            <!-- 歌单类型 -->
            <template v-if="item.type === 'playlist'">
              <span class="drawer-item-icon">🎵</span>
              <div class="drawer-item-info">
                <span class="drawer-item-title">{{ item.playlistName }}</span>
                <span class="drawer-item-artist">{{ item.songs?.length || 0 }} 首</span>
              </div>
              <span class="drawer-item-duration">{{ formatTime(totalDuration(item.songs)) }}</span>
            </template>
            <!-- 单曲类型 -->
            <template v-else>
              <span class="drawer-item-index">
                <el-icon v-if="index === playerStore.currentIndex" color="#409eff" size="14"><VideoPlay /></el-icon>
                <span v-else>{{ index + 1 }}</span>
              </span>
              <div class="drawer-item-info">
                <span class="drawer-item-title">{{ item.song?.title || '(已删除)' }}</span>
                <span class="drawer-item-artist">{{ item.song?.artist || '' }}</span>
              </div>
              <el-icon
                v-if="item.song"
                class="drawer-item-fav"
                :class="{ active: playlistStore.isFavorite(item.song.id) }"
                @click.stop="handleDrawerToggleFavorite(item.song)"
              >
                <StarFilled v-if="playlistStore.isFavorite(item.song.id)" />
                <Star v-else />
              </el-icon>
              <span class="drawer-item-duration">{{ formatTime(item.song?.duration) }}</span>
              <el-icon class="drawer-item-remove" @click.stop="handleDrawerRemove(index)"><Delete /></el-icon>
            </template>
          </div>
        </div>
        <el-empty v-else description="队列是空的" :image-size="60" />
      </el-drawer>
    </el-container>
  </el-container>
</template>

<script setup>
import { computed, onMounted, ref, watch, nextTick } from 'vue'
import Sortable from 'sortablejs'
import { useRoute, useRouter } from 'vue-router'
import { usePlayerStore } from './stores/player'
import { usePlaylistStore } from './stores/playlist'
import { useWebSocket } from './composables/useWebSocket'
import { useKeyboard } from './composables/useKeyboard'
import { removeFromQueue, clearQueue, reorderQueue } from './api/queue'
import { formatTime } from './utils/format'
import {
  Headset,
  List,
  Menu,
  Star,
  StarFilled,
  ArrowLeft,
  ArrowRight,
  ArrowDown,
  ArrowLeftBold,
  VideoPlay,
  VideoPause,
  Mute,
  Microphone,
  RefreshRight,
  Refresh,
  Sort,
  Plus,
  Edit,
  Delete,
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

const route = useRoute()
const router = useRouter()
const playerStore = usePlayerStore()
const playlistStore = usePlaylistStore()

// ===== 播放列表抽屉 =====
const queueDrawerVisible = ref(false)
const drawerListRef = ref(null)
let drawerSortableInstance = null

function initDrawerSortable() {
  const el = drawerListRef.value
  if (!el) return
  drawerSortableInstance?.destroy()
  drawerSortableInstance = Sortable.create(el, {
    animation: 150,
    ghostClass: 'sortable-ghost',
    chosenClass: 'sortable-chosen',
    onEnd: async (event) => {
      const { oldIndex, newIndex } = event
      if (oldIndex === newIndex || newIndex == null) return
      try {
        await reorderQueue(oldIndex, newIndex)
        await playerStore.refreshQueue()
      } catch {
        ElMessage.error('排序失败')
        await playerStore.refreshQueue()
      }
    },
  })
}

watch(queueDrawerVisible, (visible) => {
  if (visible) {
    nextTick(initDrawerSortable)
  } else {
    drawerSortableInstance?.destroy()
    drawerSortableInstance = null
  }
})

const expandedCount = computed(() => {
  let count = 0
  for (const item of playerStore.queue) {
    if (item.type === 'song') count++
    else if (item.type === 'playlist') count += item.songs?.length || 0
  }
  return count
})

function totalDuration(songs) {
  if (!songs) return 0
  return songs.reduce((sum, s) => sum + (s.duration || 0), 0)
}

async function handleDrawerRemove(index) {
  try {
    await removeFromQueue(index)
    await playerStore.refreshQueue()
  } catch {
    ElMessage.error('移除失败')
  }
}

async function handleDrawerToggleFavorite(song) {
  try {
    await playlistStore.toggleFavorite(song.id)
    const isFav = playlistStore.isFavorite(song.id)
    ElMessage.success(isFav ? `已收藏「${song.title}」` : `已取消收藏「${song.title}」`)
  } catch {
    ElMessage.error('操作失败')
  }
}

async function handleClearQueue() {
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

// ===== 歌单侧边栏 =====
const playlistExpanded = ref(true)
const contextMenu = ref({ visible: false, x: 0, y: 0, playlist: null })

const playlistsExceptFavorite = computed(() =>
  playlistStore.playlists.filter(p => p.id !== playlistStore.favoritePlaylistId)
)

// 侧边栏高亮：我喜欢和歌单详情页需要匹配具体路径
const sidebarActive = computed(() => {
  const path = route.path
  if (path.startsWith('/playlists/') && playlistStore.favoritePlaylistId) {
    return path // 详情页时精确匹配
  }
  return path
})

function showPlaylistMenu(e, pl) {
  contextMenu.value = { visible: true, x: e.clientX, y: e.clientY, playlist: pl }
}

function closeContextMenu() {
  contextMenu.value = { visible: false, x: 0, y: 0, playlist: null }
}

async function handleRenamePlaylist() {
  const pl = contextMenu.value.playlist
  closeContextMenu()
  if (!pl) return
  try {
    const { value } = await ElMessageBox.prompt('输入新名称', '重命名歌单', {
      inputValue: pl.name,
      confirmButtonText: '确定',
      cancelButtonText: '取消',
    })
    const name = value?.trim()
    if (name && name !== pl.name) {
      await playlistStore.rename(pl.id, name)
      ElMessage.success('重命名成功')
    }
  } catch {}
}

async function handleDeletePlaylist() {
  const pl = contextMenu.value.playlist
  closeContextMenu()
  if (!pl || pl.name === '我喜欢') return
  try {
    await ElMessageBox.confirm(`确定删除歌单「${pl.name}」？`, '删除确认', {
      confirmButtonText: '删除',
      cancelButtonText: '取消',
      type: 'warning',
    })
    await playlistStore.remove(pl.id)
    ElMessage.success('已删除')
  } catch {}
}

async function handleCreatePlaylist() {
  try {
    const { value } = await ElMessageBox.prompt('输入歌单名称', '新建歌单', {
      inputValue: '新建歌单',
      confirmButtonText: '创建',
      cancelButtonText: '取消',
    })
    const name = value?.trim()
    if (name) {
      const result = await playlistStore.create(name)
      ElMessage.success(`歌单「${name}」已创建`)
      if (result?.id) {
        router.push(`/playlists/${result.id}`)
      }
    }
  } catch {}
}

// 全局点击关闭右键菜单
document.addEventListener('click', closeContextMenu)

// ===== 键盘快捷键 =====
useKeyboard()

// ===== 进度条绑定 =====
const progressValue = computed({
  get: () => playerStore.progressPercent,
  set: (value) => {
    playerStore.currentPosition = Math.round((value / 100) * playerStore.duration)
  },
})

function onSeek(value) {
  const position = Math.round((value / 100) * playerStore.duration)
  playerStore.seek(position)
}

// ===== 音量条绑定 =====
const volumeValue = computed({
  get: () => playerStore.volume,
  set: (value) => {
    playerStore.volume = value
  },
})

// ===== 播放模式切换 =====
const modeOrder = ['sequential', 'reverse', 'list_loop', 'single_loop', 'shuffle']

const modeTextMap = {
  sequential: '顺序播放',
  reverse: '倒序播放',
  list_loop: '列表循环',
  single_loop: '单曲循环',
  shuffle: '随机播放',
}

const modeText = computed(() => modeTextMap[playerStore.mode] || '顺序播放')

function cycleMode() {
  const currentIndex = modeOrder.indexOf(playerStore.mode)
  const nextIndex = (currentIndex + 1) % modeOrder.length
  playerStore.setMode(modeOrder[nextIndex])
}

// ===== 播放栏收藏 =====
async function handleBarToggleFavorite() {
  if (!playerStore.hasCurrentSong) return
  try {
    await playlistStore.toggleFavorite(playerStore.currentSong.id)
    const isFav = playlistStore.isFavorite(playerStore.currentSong.id)
    ElMessage.success(isFav ? `已收藏「${playerStore.currentSong.title}」` : `已取消收藏`)
  } catch {
    ElMessage.error('操作失败')
  }
}

// ===== WebSocket 连接（进度推送 + 状态同步）=====
const { connected } = useWebSocket()

onMounted(async () => {
  await playerStore.loadState()
  await playerStore.refreshQueue()
  await playlistStore.ensureFavoritePlaylist()

  // 兜底：如果 WebSocket 连不上，用本地 tick 模拟进度
  let tickTimer = null
  const unwatch = watch(connected, (isConnected) => {
    if (!isConnected && !tickTimer) {
      console.warn('[App] WebSocket 未连接，启用本地进度模拟')
      tickTimer = setInterval(() => playerStore.tick(), 1000)
    }
  }, { immediate: true })
})
</script>

<style>
/* ===== 全局样式 ===== */
html, body {
  margin: 0;
  padding: 0;
  height: 100%;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'PingFang SC', 'Microsoft YaHei', sans-serif;
  -webkit-font-smoothing: antialiased;
}

#app {
  height: 100%;
}

/* 自定义滚动条 */
::-webkit-scrollbar {
  width: 6px;
}
::-webkit-scrollbar-track {
  background: transparent;
}
::-webkit-scrollbar-thumb {
  background: #c0c4cc;
  border-radius: 3px;
}
::-webkit-scrollbar-thumb:hover {
  background: #909399;
}

/* Element Plus 表格行 hover 优化 */
.el-table__body tr:hover > td {
  background-color: #f0f2ff !important;
}
</style>

<style scoped>
.app-layout {
  height: 100vh;
  overflow: hidden;
}

/* ===== 深色侧边栏 ===== */
.sidebar {
  background-color: #1e1e2d;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.logo {
  height: 64px;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.06);
}

.logo-icon {
  font-size: 24px;
}

.logo-text {
  font-size: 20px;
  font-weight: 700;
  color: #ffffff;
  letter-spacing: 1px;
}

.nav-menu {
  flex: 0 0 auto;
  border-right: none !important;
}

.nav-menu :deep(.el-menu-item) {
  height: 48px;
  line-height: 48px;
  margin: 2px 8px;
  border-radius: 8px;
  font-size: 14px;
  transition: all 0.2s;
}

.nav-menu :deep(.el-menu-item:hover) {
  background-color: rgba(255, 255, 255, 0.06) !important;
}

.nav-menu :deep(.el-menu-item.is-active) {
  background-color: rgba(64, 158, 255, 0.2) !important;
  color: #ffffff !important;
}

/* ===== 歌单分区 ===== */
.playlist-section {
  display: flex;
  flex-direction: column;
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  border-top: 1px solid rgba(255, 255, 255, 0.06);
}

.playlist-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 14px 20px;
  font-size: 13px;
  font-weight: 600;
  color: #a0a0b8;
  cursor: pointer;
  transition: all 0.2s;
  user-select: none;
}

.playlist-header:hover {
  color: #ffffff;
  background-color: rgba(255, 255, 255, 0.04);
}

.expand-arrow {
  margin-left: auto;
  font-size: 12px;
  transition: transform 0.2s;
}

.expand-arrow.expanded {
  transform: rotate(0deg);
}

.playlist-list {
  display: flex;
  flex-direction: column;
  gap: 1px;
  padding: 0 8px 8px;
}

.playlist-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 13px;
  color: #a0a0b8;
  cursor: pointer;
  transition: all 0.15s;
}

.playlist-item:hover {
  background-color: rgba(255, 255, 255, 0.06);
  color: #ffffff;
}

.playlist-item.active {
  background-color: rgba(64, 158, 255, 0.15);
  color: #ffffff;
}

.playlist-item-icon {
  font-size: 14px;
  flex-shrink: 0;
}

.playlist-item-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.playlist-item-count {
  font-size: 11px;
  color: #6a6a7a;
  flex-shrink: 0;
}

.playlist-empty {
  padding: 12px;
  text-align: center;
  font-size: 12px;
  color: #6a6a7a;
}

.playlist-add {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 13px;
  color: #6a6a7a;
  cursor: pointer;
  transition: all 0.15s;
  margin-top: 4px;
}

.playlist-add:hover {
  background-color: rgba(255, 255, 255, 0.06);
  color: #a0a0b8;
}

/* 展开/折叠动画 */
.expand-enter-active,
.expand-leave-active {
  transition: all 0.2s ease;
  overflow: hidden;
}

.expand-enter-from,
.expand-leave-to {
  opacity: 0;
  max-height: 0;
}

.expand-enter-to,
.expand-leave-from {
  opacity: 1;
  max-height: 500px;
}

/* ===== 右键菜单 ===== */
.context-menu {
  position: fixed;
  z-index: 9999;
  background: #2a2a3d;
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  padding: 4px 0;
  min-width: 140px;
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.4);
}

.context-menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  font-size: 13px;
  color: #a0a0b8;
  cursor: pointer;
  transition: all 0.15s;
}

.context-menu-item:hover {
  background-color: rgba(255, 255, 255, 0.08);
  color: #ffffff;
}

.context-menu-item.danger:hover {
  background-color: rgba(245, 108, 108, 0.15);
  color: #f56c6c;
}

.sidebar-footer {
  padding: 12px 16px;
  border-top: 1px solid rgba(255, 255, 255, 0.06);
}

.shortcut-hint {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.hint-row {
  font-size: 11px;
  color: #6a6a7a;
  display: flex;
  align-items: center;
  gap: 6px;
}

.hint-row kbd {
  display: inline-block;
  padding: 1px 6px;
  font-size: 10px;
  font-family: inherit;
  color: #8a8a9a;
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 3px;
  min-width: 24px;
  text-align: center;
}

/* ===== 主内容区 ===== */
.main-area {
  display: flex;
  flex-direction: column;
  background-color: #f8f9fc;
}

.content {
  flex: 1;
  overflow: auto;
  background-color: #f8f9fc;
}

/* ===== 底部播放栏（玻璃态） ===== */
.player-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 28px;
  background: linear-gradient(180deg, #ffffff 0%, #fafbfc 100%);
  border-top: 1px solid #ebeef5;
  box-shadow: 0 -2px 12px rgba(0, 0, 0, 0.04);
  backdrop-filter: blur(8px);
}

/* 左侧歌曲信息 */
.song-info {
  width: 220px;
  display: flex;
  align-items: center;
  gap: 12px;
  overflow: hidden;
}

.cover-art {
  width: 52px;
  height: 52px;
  border-radius: 8px;
  background: linear-gradient(135deg, #e8eaed 0%, #dde0e4 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  transition: all 0.3s;
}

.cover-art.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
}

.cover-note {
  font-size: 20px;
}

.cover-note.muted {
  opacity: 0.3;
}

.song-meta {
  overflow: hidden;
  min-width: 0;
}

.song-meta .title {
  font-weight: 600;
  font-size: 14px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  color: #303133;
}

.song-meta .artist {
  font-size: 12px;
  color: #909399;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  margin-top: 2px;
}

.bar-fav-heart {
  font-size: 20px;
  cursor: pointer;
  color: #c0c4cc;
  transition: all 0.2s;
  flex-shrink: 0;
  margin-left: 8px;
}

.bar-fav-heart:hover {
  color: #f56c6c;
  transform: scale(1.15);
}

.bar-fav-heart.active {
  color: #f56c6c;
}

.bar-fav-heart.active:hover {
  color: #f78989;
}

/* 中间控制区 */
.player-center {
  flex: 1;
  max-width: 640px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 6px;
}

.controls {
  display: flex;
  align-items: center;
  gap: 20px;
}

.ctrl-btn {
  border: none !important;
  background: transparent !important;
  color: #606266;
  transition: all 0.2s;
}

.ctrl-btn:hover:not(:disabled) {
  color: #409eff;
  background: rgba(64, 158, 255, 0.08) !important;
}

.ctrl-btn.play-btn {
  width: 44px !important;
  height: 44px !important;
  background: linear-gradient(135deg, #409eff 0%, #337ecc 100%) !important;
  color: #ffffff !important;
  box-shadow: 0 2px 8px rgba(64, 158, 255, 0.35);
}

.ctrl-btn.play-btn:hover:not(:disabled) {
  box-shadow: 0 4px 16px rgba(64, 158, 255, 0.5);
  transform: scale(1.06);
}

.progress-area {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 12px;
}

.progress-area .time {
  font-size: 11px;
  color: #909399;
  min-width: 36px;
  text-align: center;
  font-variant-numeric: tabular-nums;
}

.progress-slider {
  flex: 1;
}

/* 右侧功能区 */
.player-extra {
  width: 220px;
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
}

.volume-slider {
  width: 100px;
}

/* ===== 播放列表抽屉 ===== */
:deep(.queue-drawer .el-drawer__header) {
  margin-bottom: 0;
  padding: 16px 20px;
  border-bottom: 1px solid #ebeef5;
}

.drawer-header {
  display: flex;
  align-items: baseline;
  gap: 10px;
}

.drawer-title {
  font-size: 16px;
  font-weight: 700;
  color: #303133;
}

.drawer-subtitle {
  font-size: 12px;
  color: #909399;
}

:deep(.queue-drawer .el-drawer__body) {
  padding: 0;
  overflow: hidden;
}

.drawer-queue-list {
  height: 100%;
  overflow-y: auto;
  padding: 8px 0;
}

.drawer-queue-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 20px;
  cursor: pointer;
  transition: background-color 0.15s;
  user-select: none;
}

.drawer-queue-item:hover {
  background-color: #f0f2ff;
}

.drawer-queue-item.active {
  background-color: #ecf5ff;
}

.drawer-queue-item.active .drawer-item-title {
  color: #409eff;
  font-weight: 600;
}

.drawer-item-index {
  width: 24px;
  text-align: center;
  font-size: 12px;
  color: #909399;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
}

.drawer-item-icon {
  font-size: 16px;
  flex-shrink: 0;
}

.drawer-item-info {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.drawer-item-title {
  font-size: 13px;
  color: #303133;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.drawer-item-artist {
  font-size: 11px;
  color: #909399;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.drawer-item-duration {
  font-size: 12px;
  color: #909399;
  flex-shrink: 0;
  font-variant-numeric: tabular-nums;
}

.drawer-item-remove {
  font-size: 14px;
  color: #c0c4cc;
  cursor: pointer;
  flex-shrink: 0;
  opacity: 0;
  transition: all 0.15s;
}

.drawer-queue-item:hover .drawer-item-remove {
  opacity: 1;
}

.drawer-item-remove:hover {
  color: #f56c6c;
}

.drawer-item-fav {
  font-size: 14px;
  cursor: pointer;
  color: #c0c4cc;
  flex-shrink: 0;
  transition: all 0.2s;
}

.drawer-item-fav:hover {
  color: #f56c6c;
  transform: scale(1.2);
}

.drawer-item-fav.active {
  color: #f56c6c;
}

.drawer-item-fav.active:hover {
  color: #f78989;
}

/* 拖拽占位样式 */
:deep(.sortable-ghost) {
  opacity: 0.4;
  background-color: #f5f7fa !important;
}

:deep(.sortable-chosen) {
  background-color: #ecf5ff !important;
}
</style>
