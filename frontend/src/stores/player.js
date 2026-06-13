import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import * as playerApi from '../api/player'
import * as queueApi from '../api/queue'

/**
 * 播放器全局状态管理
 *
 * 集中维护：
 * - 当前播放歌曲、播放/暂停状态、进度
 * - 音量、静音、播放模式
 * - 当前播放队列及当前索引
 *
 * 所有页面（播放栏、音乐库、队列页）都通过该 store 访问和修改播放状态。
 */
export const usePlayerStore = defineStore('player', () => {
  // ========== State ==========

  /** 是否正在播放 */
  const isPlaying = ref(false)

  /** 当前播放歌曲信息，无歌曲时为 null */
  const currentSong = ref(null)

  /** 当前播放进度（秒） */
  const currentPosition = ref(0)

  /** 当前歌曲总时长（秒） */
  const duration = ref(0)

  /** 音量 0-100 */
  const volume = ref(80)

  /** 是否静音 */
  const muted = ref(false)

  /** 播放模式：sequential / single_loop / list_loop / shuffle */
  const mode = ref('sequential')

  /** 当前播放队列 */
  const queue = ref([])

  /** 当前歌曲在队列中的索引，-1 表示无 */
  const currentIndex = ref(-1)

  // ========== Getters ==========

  /** 是否有正在播放的歌曲 */
  const hasCurrentSong = computed(() => currentSong.value !== null)

  /** 播放进度百分比 0-100 */
  const progressPercent = computed(() => {
    if (!duration.value || duration.value <= 0) return 0
    return Math.min(100, (currentPosition.value / duration.value) * 100)
  })

  /** 格式化后的当前进度，如 "01:23" */
  const formattedPosition = computed(() => formatTime(currentPosition.value))

  /** 格式化后的总时长 */
  const formattedDuration = computed(() => formatTime(duration.value))

  // ========== Actions ==========

  /**
   * 从后端加载完整播放状态（页面初始化或轮询时使用）
   */
  async function loadState() {
    try {
      const state = await playerApi.getState()
      syncState(state)
    } catch (error) {
      console.error('加载播放器状态失败:', error)
    }
  }

  /**
   * 用后端返回的完整状态更新本地状态
   * @param {Object} state 后端 PlayerState
   */
  function syncState(state) {
    if (!state) return
    isPlaying.value = state.isPlaying ?? false
    currentSong.value = state.currentSong ?? null
    currentPosition.value = state.currentPosition ?? 0
    duration.value = state.duration ?? 0
    volume.value = state.volume ?? 80
    muted.value = state.muted ?? false
    mode.value = state.mode ?? 'sequential'
    updateCurrentIndex()
  }

  /**
   * 播放/暂停切换
   */
  async function togglePlay() {
    try {
      const result = await playerApi.toggle()
      isPlaying.value = result.isPlaying
    } catch (error) {
      console.error('切换播放状态失败:', error)
    }
  }

  /**
   * 播放指定歌曲对象（通常从音乐库或队列中点击播放）
   * @param {Object} song SongInfo
   */
  async function playSong(song) {
    try {
      await playerApi.play(song.id)
      currentSong.value = song
      currentPosition.value = 0
      duration.value = song.duration
      isPlaying.value = true
      updateCurrentIndex()
    } catch (error) {
      console.error('播放歌曲失败:', error)
    }
  }

  /**
   * 按 ID 播放歌曲
   * @param {number} songId
   */
  async function playById(songId) {
    try {
      await playerApi.play(songId)
      await loadState()
    } catch (error) {
      console.error('播放歌曲失败:', error)
    }
  }

  /**
   * 下一首
   */
  async function next() {
    try {
      await playerApi.next()
      await loadState()
    } catch (error) {
      console.error('切换到下一首失败:', error)
    }
  }

  /**
   * 上一首
   */
  async function previous() {
    try {
      await playerApi.previous()
      await loadState()
    } catch (error) {
      console.error('切换到上一首失败:', error)
    }
  }

  /**
   * 跳转到指定进度
   * @param {number} position 秒
   */
  async function seek(position) {
    try {
      await playerApi.seek(position)
      currentPosition.value = position
    } catch (error) {
      console.error('跳转进度失败:', error)
    }
  }

  /**
   * 设置音量
   * @param {number} value 0-100
   */
  async function setVolume(value) {
    try {
      await playerApi.setVolume(value)
      volume.value = Math.max(0, Math.min(100, value))
    } catch (error) {
      console.error('设置音量失败:', error)
    }
  }

  /**
   * 设置静音状态
   * @param {boolean} value
   */
  function setMuted(value) {
    muted.value = value
  }

  /**
   * 切换静音
   */
  function toggleMuted() {
    muted.value = !muted.value
  }

  /**
   * 设置播放模式
   * @param {'sequential' | 'single_loop' | 'list_loop' | 'shuffle'} newMode
   */
  async function setMode(newMode) {
    try {
      await playerApi.setMode(newMode)
      mode.value = newMode
    } catch (error) {
      console.error('设置播放模式失败:', error)
    }
  }

  /**
   * 更新当前歌曲在队列中的索引
   */
  function updateCurrentIndex() {
    if (!currentSong.value) {
      currentIndex.value = -1
      return
    }
    const index = queue.value.findIndex((s) => s.id === currentSong.value.id)
    currentIndex.value = index
  }

  /**
   * 用后端返回的新队列更新本地队列
   * @param {Object[]} newQueue SongInfo[]
   */
  function setQueue(newQueue) {
    queue.value = newQueue ?? []
    updateCurrentIndex()
  }

  /**
   * 刷新队列（从后端重新获取）
   */
  async function refreshQueue() {
    try {
      const data = await queueApi.getQueue()
      setQueue(data)
    } catch (error) {
      console.error('刷新队列失败:', error)
    }
  }

  /**
   * 仅更新进度（用于 WebSocket progress 消息或本地定时器）
   * @param {number} position 秒
   */
  function updateProgress(position) {
    if (!duration.value) return
    currentPosition.value = Math.max(0, Math.min(position, duration.value))
  }

  /**
   * 播放进度本地自增（mock 阶段用于模拟播放进度推进）
   */
  function tick() {
    if (isPlaying.value && duration.value > 0) {
      if (currentPosition.value < duration.value) {
        currentPosition.value += 1
      } else {
        // 播放到末尾，简单处理为停止
        isPlaying.value = false
      }
    }
  }

  // ========== Helpers ==========

  /**
   * 格式化秒数为 mm:ss
   * @param {number} seconds
   */
  function formatTime(seconds) {
    if (!seconds || seconds < 0) return '00:00'
    const m = Math.floor(seconds / 60)
    const s = Math.floor(seconds % 60)
    return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`
  }

  return {
    // state
    isPlaying,
    currentSong,
    currentPosition,
    duration,
    volume,
    muted,
    mode,
    queue,
    currentIndex,
    // getters
    hasCurrentSong,
    progressPercent,
    formattedPosition,
    formattedDuration,
    // actions
    loadState,
    syncState,
    togglePlay,
    playSong,
    playById,
    next,
    previous,
    seek,
    setVolume,
    setMuted,
    toggleMuted,
    setMode,
    updateCurrentIndex,
    setQueue,
    refreshQueue,
    updateProgress,
    tick,
  }
})
