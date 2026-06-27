<template>
  <div class="playlist-list-view">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>我的歌单</h2>
      <span class="header-sub" v-if="store.playlists.length">{{ store.playlists.length }} 个歌单</span>
    </div>

    <!-- 操作栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-input
          v-model="newName"
          placeholder="输入歌单名称"
          class="create-input"
          size="large"
          @keyup.enter="handleCreate"
        >
          <template #prefix>
            <el-icon><FolderAdd /></el-icon>
          </template>
        </el-input>
        <el-button type="primary" size="large" @click="handleCreate" :disabled="!newName.trim()">
          创建歌单
        </el-button>
      </div>
      <div class="toolbar-right">
        <el-button size="large" @click="showRandomDialog = true">
          <el-icon><MagicStick /></el-icon>
          随机生成
        </el-button>
      </div>
    </div>

    <!-- 歌单卡片网格 -->
    <div v-loading="store.loading" class="playlist-grid">
      <el-empty v-if="!store.loading && store.playlists.length === 0" description="还没有歌单，去音乐库选歌创建吧" :image-size="80" />

      <div
        v-for="(pl, idx) in store.playlists"
        :key="pl.id"
        class="playlist-card"
        :style="{ animationDelay: `${idx * 50}ms` }"
        @click="$router.push(`/playlists/${pl.id}`)"
      >
        <div class="card-cover" :style="{ background: cardColors[idx % cardColors.length] }">
          <span class="cover-emoji">🎵</span>
          <div class="cover-overlay">
            <el-icon size="32"><VideoPlay /></el-icon>
          </div>
        </div>
        <div class="card-body">
          <div class="card-name">{{ pl.name }}</div>
          <div class="card-count">{{ pl.songCount }} 首歌</div>
        </div>
      </div>
    </div>

    <!-- 随机生成弹框 -->
    <el-dialog v-model="showRandomDialog" title="随机生成歌单" width="440px" :close-on-click-modal="false">
      <el-form label-width="80px" label-position="left">
        <el-form-item label="歌单名称">
          <el-input v-model="randomName" placeholder="随机歌单" size="large" />
        </el-form-item>
        <el-form-item label="歌曲数量">
          <el-input-number v-model="randomCount" :min="1" :max="totalSongs" size="large" style="width: 100%" />
        </el-form-item>
        <el-form-item v-if="totalSongs > 0" label=" ">
          <span class="form-hint">音乐库共 {{ totalSongs }} 首歌曲</span>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showRandomDialog = false">取消</el-button>
        <el-button type="primary" @click="handleGenerate">生成</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { usePlaylistStore } from '../stores/playlist'
import { useLibraryStore } from '../stores/library'
import { ElMessage } from 'element-plus'
import { FolderAdd, MagicStick, VideoPlay } from '@element-plus/icons-vue'

const store = usePlaylistStore()
const libraryStore = useLibraryStore()

const newName = ref('')
const showRandomDialog = ref(false)
const randomName = ref('')
const randomCount = ref(10)
const totalSongs = ref(0)

// 卡片渐变色（每个歌单一个颜色）
const cardColors = [
  'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
  'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)',
  'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)',
  'linear-gradient(135deg, #43e97b 0%, #38f9d7 100%)',
  'linear-gradient(135deg, #fa709a 0%, #fee140 100%)',
  'linear-gradient(135deg, #a18cd1 0%, #fbc2eb 100%)',
  'linear-gradient(135deg, #fccb90 0%, #d57eeb 100%)',
  'linear-gradient(135deg, #96fbc4 0%, #f9f586 100%)',
]

onMounted(async () => {
  await Promise.all([
    store.loadPlaylists(),
    libraryStore.loadStats(),
  ])
  totalSongs.value = libraryStore.stats.totalSongs
})

async function handleCreate() {
  const name = newName.value.trim()
  if (!name) return
  await store.create(name)
  newName.value = ''
  ElMessage.success('歌单已创建')
}

async function handleGenerate() {
  if (randomCount.value > totalSongs.value) {
    ElMessage.warning(`歌曲数量不能超过音乐库总数 ${totalSongs.value}`)
    return
  }
  await store.generateRandom(randomCount.value, randomName.value || '随机歌单')
  showRandomDialog.value = false
  ElMessage.success('随机歌单已生成')
}
</script>

<style scoped>
.playlist-list-view {
  padding: 24px;
}

/* ===== 页面标题 ===== */
.page-header {
  display: flex;
  align-items: baseline;
  gap: 12px;
  margin-bottom: 24px;
}

.page-header h2 {
  margin: 0;
  font-size: 22px;
  font-weight: 700;
  color: #303133;
}

.header-sub {
  font-size: 13px;
  color: #909399;
}

/* ===== 操作栏 ===== */
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
  gap: 16px;
}

.toolbar-left {
  display: flex;
  gap: 10px;
  align-items: center;
}

.create-input {
  width: 220px;
}

/* ===== 歌单卡片网格 ===== */
.playlist-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 20px;
}

.playlist-card {
  background: #ffffff;
  border-radius: 14px;
  overflow: hidden;
  cursor: pointer;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
  transition: transform 0.25s, box-shadow 0.25s;
  animation: cardFadeIn 0.4s ease both;
}

@keyframes cardFadeIn {
  from { opacity: 0; transform: translateY(12px); }
  to { opacity: 1; transform: translateY(0); }
}

.playlist-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.12);
}

.playlist-card:hover .cover-overlay {
  opacity: 1;
}

.card-cover {
  height: 140px;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  overflow: hidden;
}

.cover-emoji {
  font-size: 48px;
  filter: drop-shadow(0 2px 4px rgba(0,0,0,0.2));
}

.cover-overlay {
  position: absolute;
  inset: 0;
  background: rgba(0, 0, 0, 0.3);
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ffffff;
  opacity: 0;
  transition: opacity 0.2s;
}

.card-body {
  padding: 14px 16px;
}

.card-name {
  font-size: 15px;
  font-weight: 600;
  color: #303133;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  margin-bottom: 4px;
}

.card-count {
  font-size: 12px;
  color: #909399;
}

/* ===== 弹框 ===== */
.form-hint {
  font-size: 12px;
  color: #909399;
}
</style>
