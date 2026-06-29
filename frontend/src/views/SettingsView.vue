<template>
  <div class="settings-page">
    <div class="settings-header">
      <h1 class="settings-title">设置</h1>
    </div>

    <div class="settings-content">
      <!-- 背景设置区域 -->
      <div class="settings-section">
        <h2 class="section-title">背景设置</h2>
        <div class="setting-item">
          <div class="setting-info">
            <span class="setting-label">自定义背景</span>
            <span class="setting-desc">开启后可设置自定义背景图片</span>
          </div>
          <el-switch
            v-model="backgroundStore.customBackgroundEnabled"
            @change="handleToggleCustomBackground"
          />
        </div>

        <div class="setting-item clickable" @click="goToBackgroundManage">
          <div class="setting-info">
            <span class="setting-label">背景图管理</span>
            <span class="setting-desc">管理背景图片，设置显示方式和透明度</span>
          </div>
          <el-icon class="setting-arrow"><ArrowRight /></el-icon>
        </div>
      </div>

      <!-- 快捷键提示 -->
      <div class="settings-section">
        <h2 class="section-title">快捷键</h2>
        <div class="shortcuts-grid">
          <div class="shortcut-item">
            <kbd>Space</kbd>
            <span>播放 / 暂停</span>
          </div>
          <div class="shortcut-item">
            <kbd>←</kbd>
            <span>上一首</span>
          </div>
          <div class="shortcut-item">
            <kbd>→</kbd>
            <span>下一首</span>
          </div>
          <div class="shortcut-item">
            <kbd>↑</kbd>
            <span>增加音量</span>
          </div>
          <div class="shortcut-item">
            <kbd>↓</kbd>
            <span>减少音量</span>
          </div>
          <div class="shortcut-item">
            <kbd>M</kbd>
            <span>静音</span>
          </div>
        </div>
      </div>

      <!-- 关于 -->
      <div class="settings-section">
        <h2 class="section-title">关于</h2>
        <div class="about-info">
          <div class="about-item">
            <span class="about-label">应用名称</span>
            <span class="about-value">59Music</span>
          </div>
          <div class="about-item">
            <span class="about-label">版本</span>
            <span class="about-value">1.0.0</span>
          </div>
          <div class="about-item">
            <span class="about-label">技术栈</span>
            <span class="about-value">Vue 3 + Element Plus + C++</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { useRouter } from 'vue-router'
import { useBackgroundStore } from '../stores/background'
import { ArrowRight } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

const router = useRouter()
const backgroundStore = useBackgroundStore()

function goToBackgroundManage() {
  router.push('/settings/backgrounds')
}

function handleToggleCustomBackground(enabled) {
  backgroundStore.toggleCustomBackground(enabled)
  ElMessage.success(enabled ? '已开启自定义背景' : '已关闭自定义背景')
}
</script>

<style scoped>
.settings-page {
  padding: 24px 32px;
  max-width: 800px;
  margin: 0 auto;
}

.settings-header {
  margin-bottom: 32px;
}

.settings-title {
  font-size: 28px;
  font-weight: 700;
  color: #303133;
  margin: 0;
}

.settings-content {
  display: flex;
  flex-direction: column;
  gap: 32px;
}

.settings-section {
  background: #ffffff;
  border-radius: 12px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.04);
}

.section-title {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 20px 0;
  padding-bottom: 12px;
  border-bottom: 1px solid #ebeef5;
}

.setting-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 0;
  border-bottom: 1px solid #f0f2f5;
}

.setting-item:last-child {
  border-bottom: none;
}

.setting-item.clickable {
  cursor: pointer;
  transition: background-color 0.2s;
  margin: 0 -24px;
  padding: 16px 24px;
  border-radius: 8px;
}

.setting-item.clickable:hover {
  background-color: #f5f7fa;
}

.setting-info {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.setting-label {
  font-size: 15px;
  font-weight: 500;
  color: #303133;
}

.setting-desc {
  font-size: 13px;
  color: #909399;
}

.setting-arrow {
  font-size: 16px;
  color: #c0c4cc;
}

.shortcuts-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.shortcut-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  background: #f5f7fa;
  border-radius: 8px;
}

.shortcut-item kbd {
  display: inline-block;
  padding: 4px 10px;
  font-size: 12px;
  font-family: inherit;
  color: #606266;
  background: #ffffff;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  min-width: 32px;
  text-align: center;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.shortcut-item span {
  font-size: 14px;
  color: #606266;
}

.about-info {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.about-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 0;
}

.about-label {
  font-size: 14px;
  color: #909399;
}

.about-value {
  font-size: 14px;
  color: #303133;
  font-weight: 500;
}
</style>
