<template>
  <el-container class="app-layout">
    <!-- 侧边栏 -->
    <el-aside width="200px" class="sidebar">
      <div class="logo">59Music</div>
      <el-menu :default-active="$route.path" router class="nav-menu">
        <el-menu-item index="/">
          <el-icon><Headset /></el-icon>
          <span>音乐库</span>
        </el-menu-item>
        <el-menu-item index="/queue">
          <el-icon><List /></el-icon>
          <span>播放队列</span>
        </el-menu-item>
      </el-menu>
    </el-aside>

    <!-- 主内容区 -->
    <el-container class="main-area">
      <el-main class="content">
        <router-view />
      </el-main>

      <!-- 底部播放栏 -->
      <el-footer height="90px" class="player-bar">
        <!-- 左侧：歌曲信息 -->
        <div class="song-info">
          <div class="title">{{ playerStore.hasCurrentSong ? playerStore.currentSong.title : '暂无歌曲' }}</div>
          <div class="artist">
            {{ playerStore.hasCurrentSong ? playerStore.currentSong.artist : '-' }}
          </div>
        </div>

        <!-- 中间：控制按钮 + 进度 -->
        <div class="player-center">
          <div class="controls">
            <el-button circle size="small" :disabled="!playerStore.hasCurrentSong" @click="playerStore.previous">
              <el-icon><Arrow-Left /></el-icon>
            </el-button>
            <el-button circle size="large" :disabled="!playerStore.hasCurrentSong" @click="playerStore.togglePlay">
              <el-icon>
                <Video-Play v-if="!playerStore.isPlaying" />
                <Video-Pause v-else />
              </el-icon>
            </el-button>
            <el-button circle size="small" :disabled="!playerStore.hasCurrentSong" @click="playerStore.next">
              <el-icon><Arrow-Right /></el-icon>
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
            <el-button circle size="small" :disabled="!playerStore.hasCurrentSong" @click="cycleMode">
              <el-icon>
                <Refresh-Right v-if="playerStore.mode === 'single_loop'" />
                <Refresh v-else-if="playerStore.mode === 'list_loop'" />
                <Sort v-else-if="playerStore.mode === 'shuffle'" />
                <Arrow-Right v-else />
              </el-icon>
            </el-button>
          </el-tooltip>

          <el-button circle size="small" @click="playerStore.toggleMuted">
            <el-icon>
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
        </div>
      </el-footer>
    </el-container>
  </el-container>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { usePlayerStore } from './stores/player'
import {
  Headset,
  List,
  ArrowLeft,
  ArrowRight,
  VideoPlay,
  VideoPause,
  Mute,
  Microphone,
  RefreshRight,
  Refresh,
  Sort,
} from '@element-plus/icons-vue'

const playerStore = usePlayerStore()

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
const modeOrder = ['sequential', 'list_loop', 'single_loop', 'shuffle']

const modeTextMap = {
  sequential: '顺序播放',
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

// ===== mock 阶段：模拟播放进度推进 =====
let progressTimer = null

onMounted(() => {
  playerStore.loadState()
  progressTimer = setInterval(() => {
    playerStore.tick()
  }, 1000)
})

onUnmounted(() => {
  if (progressTimer) {
    clearInterval(progressTimer)
  }
})
</script>

<style scoped>
.app-layout {
  height: 100vh;
}

.sidebar {
  background-color: #f5f7fa;
  border-right: 1px solid #e4e7ed;
}

.logo {
  height: 60px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  font-weight: bold;
  color: #409eff;
  border-bottom: 1px solid #e4e7ed;
}

.nav-menu {
  border-right: none;
}

.main-area {
  display: flex;
  flex-direction: column;
}

.content {
  flex: 1;
  overflow: auto;
  background-color: #ffffff;
}

.player-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  background-color: #ffffff;
  border-top: 1px solid #e4e7ed;
}

/* 左侧歌曲信息 */
.song-info {
  width: 200px;
  overflow: hidden;
}

.song-info .title {
  font-weight: bold;
  font-size: 14px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.song-info .artist {
  font-size: 12px;
  color: #909399;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* 中间控制区 */
.player-center {
  flex: 1;
  max-width: 600px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}

.controls {
  display: flex;
  align-items: center;
  gap: 16px;
}

.progress-area {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 12px;
}

.progress-area .time {
  font-size: 12px;
  color: #909399;
  min-width: 36px;
  text-align: center;
}

.progress-slider {
  flex: 1;
}

/* 右侧功能区 */
.player-extra {
  width: 200px;
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 12px;
}

.volume-slider {
  width: 100px;
}
</style>
