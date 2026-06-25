import { onMounted, onUnmounted } from 'vue'
import { usePlayerStore } from '../stores/player'

/**
 * 全局键盘快捷键
 *
 * 空格      — 播放 / 暂停
 * ← 左箭头 — 上一首
 * → 右箭头 — 下一首
 * ↑ 上箭头 — 音量 +5
 * ↓ 下箭头 — 音量 -5
 *
 * 在输入框内自动禁用，避免干扰文字输入。
 */
export function useKeyboard() {
  const playerStore = usePlayerStore()

  function handleKeydown(e) {
    // 输入框中不触发快捷键
    const tag = e.target.tagName
    if (tag === 'INPUT' || tag === 'TEXTAREA' || e.target.isContentEditable) return

    switch (e.code) {
      case 'Space':
        e.preventDefault()
        if (playerStore.hasCurrentSong) {
          playerStore.togglePlay()
        }
        break
      case 'ArrowLeft':
        e.preventDefault()
        if (playerStore.hasCurrentSong) {
          playerStore.previous()
        }
        break
      case 'ArrowRight':
        e.preventDefault()
        if (playerStore.hasCurrentSong) {
          playerStore.next()
        }
        break
      case 'ArrowUp':
        e.preventDefault()
        playerStore.setVolume(Math.min(100, playerStore.volume + 5))
        break
      case 'ArrowDown':
        e.preventDefault()
        playerStore.setVolume(Math.max(0, playerStore.volume - 5))
        break
    }
  }

  onMounted(() => {
    document.addEventListener('keydown', handleKeydown)
  })

  onUnmounted(() => {
    document.removeEventListener('keydown', handleKeydown)
  })
}
