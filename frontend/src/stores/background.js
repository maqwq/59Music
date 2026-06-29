/**
 * 背景图 Store
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import {
  getAllBackgrounds,
  getDefaultBackground,
  addBackground as apiAddBackground,
  deleteBackground as apiDeleteBackground,
  setDefaultBackground as apiSetDefaultBackground,
} from '../api/background'

export const useBackgroundStore = defineStore('background', () => {
  // 状态
  const backgrounds = ref([])
  const defaultBackground = ref(null)
  const customBackgroundEnabled = ref(false)

  // 本地存储的键名
  const STORAGE_KEY_ENABLED = '59music_custom_background_enabled'
  const STORAGE_KEY_SETTINGS = '59music_background_settings'

  // 从本地存储加载设置
  function loadSettings() {
    const enabled = localStorage.getItem(STORAGE_KEY_ENABLED)
    if (enabled !== null) {
      customBackgroundEnabled.value = enabled === 'true'
    }

    const settingsStr = localStorage.getItem(STORAGE_KEY_SETTINGS)
    if (settingsStr) {
      try {
        const settings = JSON.parse(settingsStr)
        // 将设置应用到背景列表
        backgrounds.value.forEach((bg) => {
          if (settings[bg.id]) {
            Object.assign(bg, settings[bg.id])
          }
        })
      } catch (e) {
        console.error('加载背景设置失败:', e)
      }
    }
  }

  // 保存设置到本地存储
  function saveSettings() {
    localStorage.setItem(STORAGE_KEY_ENABLED, String(customBackgroundEnabled.value))

    const settings = {}
    backgrounds.value.forEach((bg) => {
      settings[bg.id] = {
        displayMode: bg.displayMode || 'cover',
        sidebarOpacity: bg.sidebarOpacity ?? 50,
        contentOpacity: bg.contentOpacity ?? 50,
        playerBarOpacity: bg.playerBarOpacity ?? 50,
      }
    })
    localStorage.setItem(STORAGE_KEY_SETTINGS, JSON.stringify(settings))
  }

  // 获取所有背景图
  async function fetchBackgrounds() {
    try {
      const res = await getAllBackgrounds()
      if (res.data) {
        backgrounds.value = res.data.map((bg) => ({
          ...bg,
          displayMode: bg.displayMode || 'cover',
          sidebarOpacity: bg.sidebarOpacity ?? 50,
          contentOpacity: bg.contentOpacity ?? 50,
          playerBarOpacity: bg.playerBarOpacity ?? 50,
        }))
      }
      loadSettings()

      // 如果有默认背景，设置它
      const defaultBg = backgrounds.value.find((bg) => bg.isDefault)
      if (defaultBg) {
        defaultBackground.value = defaultBg
      }

      // 如果启用了自定义背景，应用它
      if (customBackgroundEnabled.value) {
        applyBackground()
      }
    } catch (error) {
      console.error('获取背景图列表失败:', error)
    }
  }

  // 添加背景图
  async function addBackground(file) {
    // 创建 FormData 上传文件
    const formData = new FormData()
    formData.append('file', file)

    // 使用 fetch 上传文件到后端
    const uploadRes = await fetch('/api/v1/upload/background', {
      method: 'POST',
      body: formData,
    })

    if (!uploadRes.ok) {
      throw new Error('上传文件失败')
    }

    const uploadData = await uploadRes.json()
    if (!uploadData.success) {
      throw new Error(uploadData.msg || '上传文件失败')
    }

    // 调用后端 API 添加背景记录
    const res = await apiAddBackground({
      filePath: uploadData.data.filePath,
      name: file.name.replace(/\.[^/.]+$/, ''), // 去掉扩展名作为名称
    })

    if (res.data) {
      // 添加到列表
      backgrounds.value.push({
        ...res.data,
        displayMode: 'cover',
        sidebarOpacity: 50,
        contentOpacity: 50,
        playerBarOpacity: 50,
      })
      saveSettings()
    }

    return res.data
  }

  // 删除背景图
  async function remove(id) {
    await apiDeleteBackground(id)
    backgrounds.value = backgrounds.value.filter((bg) => bg.id !== id)

    // 如果删除的是默认背景，清空默认背景
    if (defaultBackground.value?.id === id) {
      defaultBackground.value = null
    }

    saveSettings()

    // 如果启用了自定义背景，重新应用
    if (customBackgroundEnabled.value) {
      applyBackground()
    }
  }

  // 设置默认背景
  async function setDefault(id) {
    await apiSetDefaultBackground(id)

    // 更新列表中的默认状态
    backgrounds.value.forEach((bg) => {
      bg.isDefault = bg.id === id
    })

    // 设置默认背景
    defaultBackground.value = backgrounds.value.find((bg) => bg.id === id)

    saveSettings()

    // 如果启用了自定义背景，应用新的默认背景
    if (customBackgroundEnabled.value) {
      applyBackground()
    }
  }

  // 更新背景设置（显示方式、透明度）
  function updateBackground(id, settings) {
    const bg = backgrounds.value.find((b) => b.id === id)
    if (bg) {
      Object.assign(bg, settings)
      saveSettings()
    }
  }

  // 应用背景图到页面
  function applyBackground() {
    const bg = defaultBackground.value
    if (!bg || !customBackgroundEnabled.value) {
      clearBackground()
      return
    }

    // 获取背景图 URL
    const fileName = bg.filePath.split(/[/\\]/).pop()
    const url = `/backgrounds/${fileName}`

    // 设置背景图
    document.documentElement.style.setProperty('--bg-image', `url('${url}')`)

    // 设置显示方式
    let bgSize = 'cover'
    let bgRepeat = 'no-repeat'
    switch (bg.displayMode) {
      case 'cover':
        bgSize = 'cover'
        bgRepeat = 'no-repeat'
        break
      case 'stretch':
        bgSize = '100% 100%'
        bgRepeat = 'no-repeat'
        break
      case 'center':
        bgSize = 'auto'
        bgRepeat = 'no-repeat'
        break
      case 'repeat':
        bgSize = 'auto'
        bgRepeat = 'repeat'
        break
    }

    document.documentElement.style.setProperty('--bg-size', bgSize)
    document.documentElement.style.setProperty('--bg-repeat', bgRepeat)

    // 设置透明度
    const sidebarOpacity = (bg.sidebarOpacity ?? 50) / 100
    const contentOpacity = (bg.contentOpacity ?? 50) / 100
    const playerBarOpacity = (bg.playerBarOpacity ?? 50) / 100

    document.documentElement.style.setProperty('--sidebar-opacity', String(sidebarOpacity))
    document.documentElement.style.setProperty('--content-opacity', String(contentOpacity))
    document.documentElement.style.setProperty('--playerbar-opacity', String(playerBarOpacity))

    // 添加背景启用类
    document.documentElement.classList.add('custom-background-enabled')
  }

  // 清除背景图
  function clearBackground() {
    document.documentElement.style.removeProperty('--bg-image')
    document.documentElement.style.removeProperty('--bg-size')
    document.documentElement.style.removeProperty('--bg-repeat')
    document.documentElement.style.removeProperty('--sidebar-opacity')
    document.documentElement.style.removeProperty('--content-opacity')
    document.documentElement.style.removeProperty('--playerbar-opacity')
    document.documentElement.classList.remove('custom-background-enabled')
  }

  // 切换自定义背景
  function toggleCustomBackground(enabled) {
    customBackgroundEnabled.value = enabled
    saveSettings()

    if (enabled) {
      applyBackground()
    } else {
      clearBackground()
    }
  }

  return {
    backgrounds,
    defaultBackground,
    customBackgroundEnabled,
    fetchBackgrounds,
    addBackground,
    remove,
    setDefault,
    updateBackground,
    applyBackground,
    clearBackground,
    toggleCustomBackground,
  }
})
