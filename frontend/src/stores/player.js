import { defineStore } from 'pinia'
import { ref } from 'vue'

/**
 * 播放器全局状态
 *
 * 目前先放一个简单的状态，后续会逐渐扩展：
 * - 当前播放歌曲
 * - 播放/暂停状态
 * - 播放队列
 * - 播放模式
 * - 音量
 */
export const usePlayerStore = defineStore('player', () => {
  const isPlaying = ref(false)

  function togglePlay() {
    isPlaying.value = !isPlaying.value
  }

  return {
    isPlaying,
    togglePlay,
  }
})
