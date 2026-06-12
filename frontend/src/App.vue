<template>
  <el-container class="app-layout">
    <!-- 侧边栏 -->
    <el-aside width="200px" class="sidebar">
      <div class="logo">59Music</div>
      <el-menu
        :default-active="$route.path"
        router
        class="nav-menu"
      >
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
      <el-footer height="80px" class="player-bar">
        <div class="song-info">
          <div class="title">暂无歌曲</div>
          <div class="artist">-</div>
        </div>
        <div class="controls">
          <el-button circle>
            <el-icon><Arrow-Left /></el-icon>
          </el-button>
          <el-button circle size="large" @click="playerStore.togglePlay">
            <el-icon>
              <Video-Play v-if="!playerStore.isPlaying" />
              <Video-Pause v-else />
            </el-icon>
          </el-button>
          <el-button circle>
            <el-icon><Arrow-Right /></el-icon>
          </el-button>
        </div>
        <div class="extra">
          <span>音量 / 进度占位</span>
        </div>
      </el-footer>
    </el-container>
  </el-container>
</template>

<script setup>
import { usePlayerStore } from './stores/player'
import {
  Headset,
  List,
  ArrowLeft,
  ArrowRight,
  VideoPlay,
  VideoPause,
} from '@element-plus/icons-vue'

const playerStore = usePlayerStore()
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
  padding: 0 20px;
  background-color: #ffffff;
  border-top: 1px solid #e4e7ed;
}

.song-info {
  width: 200px;
}

.song-info .title {
  font-weight: bold;
}

.song-info .artist {
  font-size: 12px;
  color: #909399;
}

.controls {
  display: flex;
  align-items: center;
  gap: 12px;
}

.extra {
  width: 200px;
  text-align: right;
  font-size: 12px;
  color: #909399;
}
</style>
