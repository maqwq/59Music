import request, { USE_MOCK, delay } from './request'
import { mockSongs } from '../mock/data'

let mockItems = [
  { type: 'song', songId: 1, song: mockSongs[0] },
  { type: 'playlist', playlistId: 1, playlistName: '测试歌单', songs: [mockSongs[1], mockSongs[2]] },
]

// ===== 接口封装 =====

/** 获取播放队列（返回 items 数组） */
export function getQueue() {
  if (USE_MOCK) {
    return delay().then(() => JSON.parse(JSON.stringify(mockItems)))
  }
  return request.get('/queue')
}

/** 添加歌曲到队列（带去重） */
export function addToQueue(songIds) {
  if (USE_MOCK) {
    return delay().then(() => {
      const added = []
      for (const id of songIds) {
        const exists = mockItems.some(item => item.type === 'song' && item.songId === id)
        if (!exists) {
          const song = mockSongs.find(s => s.id === id)
          if (song) {
            mockItems.push({ type: 'song', songId: id, song })
            added.push(id)
          }
        }
      }
      return { added: added.length, skipped: songIds.length - added.length }
    })
  }
  return request.post('/queue/add', { songIds })
}

/** 将歌单加入队列（歌单引用） */
export function addPlaylistToQueue(playlistId) {
  if (USE_MOCK) {
    return delay().then(() => {
      mockItems.push({
        type: 'playlist',
        playlistId,
        playlistName: `歌单 ${playlistId}`,
        songs: [mockSongs[0]],
      })
      return null
    })
  }
  return request.post('/queue/add-playlist', { playlistId })
}

/** 移除队列中指定索引的项 */
export function removeFromQueue(index) {
  if (USE_MOCK) {
    return delay().then(() => {
      if (index < 0 || index >= mockItems.length) throw new Error('索引超出范围')
      mockItems.splice(index, 1)
      return null
    })
  }
  return request.delete(`/queue/${index}`)
}

/** 清空播放队列 */
export function clearQueue() {
  if (USE_MOCK) {
    return delay().then(() => { mockItems = []; return null })
  }
  return request.delete('/queue')
}

/** 调整队列中项的位置 */
export function reorderQueue(from, to) {
  if (USE_MOCK) {
    return delay().then(() => {
      if (from < 0 || from >= mockItems.length || to < 0 || to >= mockItems.length) throw new Error('索引超出范围')
      const [moved] = mockItems.splice(from, 1)
      mockItems.splice(to, 0, moved)
      return null
    })
  }
  return request.post('/queue/reorder', null, { params: { from, to } })
}
