<template>
  <div class="background-manage-page">
    <div class="page-header">
      <div class="header-left">
        <el-button class="back-btn" @click="router.push('/settings')">
          <el-icon><ArrowLeft /></el-icon>
          返回设置
        </el-button>
        <h1 class="page-title">背景图管理</h1>
      </div>
      <el-button type="primary" @click="handleAddBackground">
        <el-icon><Plus /></el-icon>
        添加背景图
      </el-button>
    </div>

    <!-- 隐藏的文件选择器 -->
    <input
      ref="fileInputRef"
      type="file"
      accept="image/*"
      style="display: none"
      @change="handleFileChange"
    />

    <!-- 背景图列表 -->
    <div class="background-list-container">
      <el-table
        :data="backgroundStore.backgrounds"
        style="width: 100%"
        empty-text="暂无背景图，点击上方按钮添加"
        @row-click="handleRowClick"
      >
        <el-table-column label="缩略图" width="100">
          <template #default="{ row }">
            <div class="thumbnail-wrapper">
              <img
                :src="getThumbnailUrl(row)"
                :alt="row.name"
                class="thumbnail-img"
              />
            </div>
          </template>
        </el-table-column>

        <el-table-column prop="name" label="名称" min-width="150" />

        <el-table-column prop="filePath" label="文件路径" min-width="200">
          <template #default="{ row }">
            <span class="file-path">{{ row.filePath }}</span>
          </template>
        </el-table-column>

        <el-table-column label="添加时间" width="180">
          <template #default="{ row }">
            {{ formatTime(row.addedTime) }}
          </template>
        </el-table-column>

        <el-table-column label="默认" width="80" align="center">
          <template #default="{ row }">
            <el-tag v-if="row.isDefault" type="success" size="small">默认</el-tag>
          </template>
        </el-table-column>

        <el-table-column label="操作" width="150" align="center">
          <template #default="{ row }">
            <el-button
              type="primary"
              link
              size="small"
              @click.stop="handleSetDefault(row)"
            >
              {{ row.isDefault ? '已默认' : '设为默认' }}
            </el-button>
            <el-button
              type="danger"
              link
              size="small"
              @click.stop="handleDelete(row)"
            >
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <!-- 详情抽屉 -->
    <el-drawer
      v-model="detailDrawerVisible"
      title="背景图详情"
      direction="rtl"
      size="500px"
      :append-to-body="true"
    >
      <div v-if="selectedBackground" class="detail-content">
        <!-- 原图预览 -->
        <div class="detail-preview">
          <img
            :src="getThumbnailUrl(selectedBackground)"
            :alt="selectedBackground.name"
            class="preview-img"
          />
        </div>

        <!-- 背景图信息 -->
        <div class="detail-info">
          <h3 class="info-title">基本信息</h3>
          <div class="info-grid">
            <div class="info-item">
              <span class="info-label">名称</span>
              <span class="info-value">{{ selectedBackground.name }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">文件路径</span>
              <span class="info-value path">{{ selectedBackground.filePath }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">添加时间</span>
              <span class="info-value">{{ formatTime(selectedBackground.addedTime) }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">状态</span>
              <span class="info-value">
                <el-tag v-if="selectedBackground.isDefault" type="success" size="small">默认背景</el-tag>
                <el-tag v-else size="small">普通背景</el-tag>
              </span>
            </div>
          </div>
        </div>

        <!-- 显示方式设置 -->
        <div class="detail-settings">
          <h3 class="info-title">显示方式</h3>
          <div class="display-mode-options">
            <div
              v-for="mode in displayModes"
              :key="mode.value"
              class="mode-option"
              :class="{ active: selectedBackground.displayMode === mode.value }"
              @click="handleDisplayModeChange(mode.value)"
            >
              <div class="mode-preview" :style="{ backgroundSize: mode.bgSize, backgroundRepeat: mode.bgRepeat }">
                <div class="mode-preview-inner"></div>
              </div>
              <span class="mode-label">{{ mode.label }}</span>
            </div>
          </div>
        </div>

        <!-- 透明度设置 -->
        <div class="detail-settings">
          <h3 class="info-title">透明度设置</h3>
          <div class="opacity-sliders">
            <div class="opacity-item">
              <span class="opacity-label">侧边栏</span>
              <el-slider
                v-model="selectedBackground.sidebarOpacity"
                :min="0"
                :max="100"
                :format-tooltip="(val) => `${val}%`"
                @change="handleOpacityChange"
              />
              <span class="opacity-value">{{ selectedBackground.sidebarOpacity }}%</span>
            </div>
            <div class="opacity-item">
              <span class="opacity-label">工作区</span>
              <el-slider
                v-model="selectedBackground.contentOpacity"
                :min="0"
                :max="100"
                :format-tooltip="(val) => `${val}%`"
                @change="handleOpacityChange"
              />
              <span class="opacity-value">{{ selectedBackground.contentOpacity }}%</span>
            </div>
            <div class="opacity-item">
              <span class="opacity-label">播放栏</span>
              <el-slider
                v-model="selectedBackground.playerBarOpacity"
                :min="0"
                :max="100"
                :format-tooltip="(val) => `${val}%`"
                @change="handleOpacityChange"
              />
              <span class="opacity-value">{{ selectedBackground.playerBarOpacity }}%</span>
            </div>
          </div>
        </div>

        <!-- 操作按钮 -->
        <div class="detail-actions">
          <el-button
            type="primary"
            :disabled="selectedBackground.isDefault"
            @click="handleSetDefault(selectedBackground)"
          >
            {{ selectedBackground.isDefault ? '已是默认背景' : '设为默认背景' }}
          </el-button>
          <el-button
            type="danger"
            @click="handleDelete(selectedBackground)"
          >
            删除此背景
          </el-button>
        </div>
      </div>
    </el-drawer>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useBackgroundStore } from '../stores/background'
import { ArrowLeft, Plus } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const backgroundStore = useBackgroundStore()

const fileInputRef = ref(null)
const detailDrawerVisible = ref(false)
const selectedBackground = ref(null)

// 显示方式选项
const displayModes = [
  { value: 'cover', label: '覆盖', bgSize: 'cover', bgRepeat: 'no-repeat' },
  { value: 'stretch', label: '拉伸', bgSize: '100% 100%', bgRepeat: 'no-repeat' },
  { value: 'center', label: '居中', bgSize: 'auto', bgRepeat: 'no-repeat' },
  { value: 'repeat', label: '平铺', bgSize: 'auto', bgRepeat: 'repeat' },
]

// 获取缩略图 URL
function getThumbnailUrl(bg) {
  if (!bg || !bg.filePath) return ''
  // 从完整路径中提取文件名
  const fileName = bg.filePath.split(/[/\\]/).pop()
  return `/backgrounds/${fileName}`
}

// 格式化时间
function formatTime(timestamp) {
  if (!timestamp) return ''
  const date = new Date(timestamp * 1000)
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
  })
}

// 添加背景图
function handleAddBackground() {
  fileInputRef.value?.click()
}

// 文件选择变化
async function handleFileChange(event) {
  const file = event.target.files?.[0]
  if (!file) return

  // 重置 input 以便再次选择同一文件
  event.target.value = ''

  try {
    // 复制文件到 public/backgrounds 目录
    const fileName = `${Date.now()}_${file.name}`

    // 使用 fetch 上传文件到后端
    const formData = new FormData()
    formData.append('file', file)
    formData.append('name', file.name)

    // 调用后端 API 添加背景
    await backgroundStore.addBackground(file)
    ElMessage.success('背景图添加成功')
  } catch (error) {
    console.error('添加背景图失败:', error)
    ElMessage.error('添加背景图失败')
  }
}

// 点击行查看详情
function handleRowClick(row) {
  selectedBackground.value = { ...row }
  // 设置默认值
  if (selectedBackground.value.displayMode === undefined) {
    selectedBackground.value.displayMode = 'cover'
  }
  if (selectedBackground.value.sidebarOpacity === undefined) {
    selectedBackground.value.sidebarOpacity = 50
  }
  if (selectedBackground.value.contentOpacity === undefined) {
    selectedBackground.value.contentOpacity = 50
  }
  if (selectedBackground.value.playerBarOpacity === undefined) {
    selectedBackground.value.playerBarOpacity = 50
  }
  detailDrawerVisible.value = true
}

// 设置默认背景
async function handleSetDefault(row) {
  try {
    await backgroundStore.setDefault(row.id)
    ElMessage.success('已设置为默认背景')
    // 更新选中的背景
    if (selectedBackground.value?.id === row.id) {
      selectedBackground.value.isDefault = true
    }
  } catch (error) {
    ElMessage.error('设置默认背景失败')
  }
}

// 删除背景
async function handleDelete(row) {
  try {
    await ElMessageBox.confirm(
      `确定删除背景图「${row.name}」？`,
      '删除确认',
      {
        confirmButtonText: '删除',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
    await backgroundStore.remove(row.id)
    ElMessage.success('已删除')
    // 关闭详情抽屉
    if (selectedBackground.value?.id === row.id) {
      detailDrawerVisible.value = false
      selectedBackground.value = null
    }
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

// 显示方式变化
function handleDisplayModeChange(mode) {
  if (selectedBackground.value) {
    selectedBackground.value.displayMode = mode
    // 保存到后端
    backgroundStore.updateBackground(selectedBackground.value.id, {
      displayMode: mode,
    })
    // 如果是当前默认背景，立即应用
    if (selectedBackground.value.isDefault) {
      backgroundStore.applyBackground()
    }
  }
}

// 透明度变化
function handleOpacityChange() {
  if (selectedBackground.value) {
    // 保存到后端
    backgroundStore.updateBackground(selectedBackground.value.id, {
      sidebarOpacity: selectedBackground.value.sidebarOpacity,
      contentOpacity: selectedBackground.value.contentOpacity,
      playerBarOpacity: selectedBackground.value.playerBarOpacity,
    })
    // 如果是当前默认背景，立即应用
    if (selectedBackground.value.isDefault) {
      backgroundStore.applyBackground()
    }
  }
}

onMounted(() => {
  backgroundStore.fetchBackgrounds()
})
</script>

<style scoped>
.background-manage-page {
  padding: 24px 32px;
}

.page-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24px;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 16px;
}

.back-btn {
  border: none;
  background: transparent;
  color: #606266;
}

.back-btn:hover {
  color: #409eff;
}

.page-title {
  font-size: 24px;
  font-weight: 700;
  color: #303133;
  margin: 0;
}

.background-list-container {
  background: #ffffff;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.04);
}

.thumbnail-wrapper {
  width: 60px;
  height: 40px;
  border-radius: 4px;
  overflow: hidden;
  background: #f5f7fa;
}

.thumbnail-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.file-path {
  font-size: 12px;
  color: #909399;
  word-break: break-all;
}

/* 详情抽屉样式 */
.detail-content {
  padding: 0 0 20px 0;
}

.detail-preview {
  width: 100%;
  height: 200px;
  border-radius: 8px;
  overflow: hidden;
  background: #f5f7fa;
  margin-bottom: 24px;
}

.preview-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.detail-info {
  margin-bottom: 24px;
}

.info-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 16px 0;
  padding-bottom: 8px;
  border-bottom: 1px solid #ebeef5;
}

.info-grid {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.info-item {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
}

.info-label {
  font-size: 14px;
  color: #909399;
  flex-shrink: 0;
}

.info-value {
  font-size: 14px;
  color: #303133;
  text-align: right;
  word-break: break-all;
}

.info-value.path {
  font-size: 12px;
  color: #606266;
  max-width: 250px;
}

.detail-settings {
  margin-bottom: 24px;
}

.display-mode-options {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
}

.mode-option {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 12px;
  border: 2px solid #ebeef5;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
}

.mode-option:hover {
  border-color: #c0c4cc;
}

.mode-option.active {
  border-color: #409eff;
  background: #ecf5ff;
}

.mode-preview {
  width: 48px;
  height: 36px;
  border-radius: 4px;
  background-color: #dcdfe6;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='40' height='40' viewBox='0 0 40 40'%3E%3Ccircle cx='20' cy='20' r='15' fill='%23a0a0b8'/%3E%3C/svg%3E");
  overflow: hidden;
}

.mode-label {
  font-size: 12px;
  color: #606266;
}

.opacity-sliders {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.opacity-item {
  display: flex;
  align-items: center;
  gap: 12px;
}

.opacity-label {
  font-size: 14px;
  color: #606266;
  width: 60px;
  flex-shrink: 0;
}

.opacity-item .el-slider {
  flex: 1;
}

.opacity-value {
  font-size: 13px;
  color: #909399;
  width: 40px;
  text-align: right;
}

.detail-actions {
  display: flex;
  gap: 12px;
  margin-top: 24px;
  padding-top: 16px;
  border-top: 1px solid #ebeef5;
}
</style>
