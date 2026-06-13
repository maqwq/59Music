import { ref, onUnmounted } from 'vue'
import { usePlayerStore } from '../stores/player'
import { useLibraryStore } from '../stores/library'

/**
 * WebSocket 连接管理
 *
 * 连接 ws://localhost:8080/ws（已通过 Vite 代理），
 * 接收 4 类推送消息：player_state / progress / queue_changed / library_updated
 *
 * 用法：
 *   const { connected } = useWebSocket()
 *   // 连接在 setup 中自动建立，组件卸载时自动关闭
 */
export function useWebSocket() {
  const connected = ref(false)
  let ws = null
  let reconnectTimer = null
  let reconnectDelay = 1000

  const playerStore = usePlayerStore()
  let libraryStore = null
  try { libraryStore = useLibraryStore() } catch { /* 可能不在 library 页面 */ }

  function connect() {
    // 使用当前页面的 host，通过 Vite proxy 到后端
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const url = `${protocol}//${window.location.host}/ws`

    ws = new WebSocket(url)

    ws.onopen = () => {
      connected.value = true
      reconnectDelay = 1000
      console.log('[WS] 已连接')
    }

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data)
        handleMessage(msg)
      } catch (e) {
        console.error('[WS] 消息解析失败:', e)
      }
    }

    ws.onclose = () => {
      connected.value = false
      console.log('[WS] 已断开，${reconnectDelay}ms 后重连')
      scheduleReconnect()
    }

    ws.onerror = (e) => {
      console.error('[WS] 连接错误')
      ws?.close()
    }
  }

  function handleMessage(msg) {
    switch (msg.type) {
      case 'player_state':
        playerStore.syncState(msg.data)
        break

      case 'progress':
        if (msg.data) {
          playerStore.updateProgress(msg.data.position)
          // 后端可能推送更准确的 duration
          if (msg.data.duration && msg.data.duration > 0) {
            playerStore.duration = msg.data.duration
          }
        }
        break

      case 'queue_changed':
        playerStore.setQueue(msg.data)
        break

      case 'library_updated':
        if (libraryStore) {
          libraryStore.loadSongs()
          libraryStore.loadStats()
        }
        // 跨页面场景：libraryStore 不存在就忽略
        break

      default:
        break
    }
  }

  function scheduleReconnect() {
    if (reconnectTimer) clearTimeout(reconnectTimer)
    reconnectTimer = setTimeout(() => {
      reconnectDelay = Math.min(reconnectDelay * 2, 30000)
      connect()
    }, reconnectDelay)
  }

  function disconnect() {
    if (reconnectTimer) clearTimeout(reconnectTimer)
    if (ws) {
      ws.onclose = null  // 防止触发重连
      ws.close()
      ws = null
    }
  }

  // 启动连接
  connect()

  // 组件卸载时清理
  onUnmounted(() => {
    disconnect()
  })

  return { connected }
}
