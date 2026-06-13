import request, { USE_MOCK, delay } from './request'

// ===== Mock 数据 =====

const mockSongs = [
  {
    id: 1,
    title: '稻香',
    artist: '周杰伦',
    album: '魔杰座',
    duration: 223,
    filePath: 'D:/Music/周杰伦/魔杰座/稻香.mp3',
    addedTime: 1716883200,
  },
  {
    id: 2,
    title: '晴天',
    artist: '周杰伦',
    album: '叶惠美',
    duration: 269,
    filePath: 'D:/Music/周杰伦/叶惠美/晴天.mp3',
    addedTime: 1716883200,
  },
  {
    id: 3,
    title: '七里香',
    artist: '周杰伦',
    album: '七里香',
    duration: 299,
    filePath: 'D:/Music/周杰伦/七里香/七里香.mp3',
    addedTime: 1716883200,
  },
  {
    id: 4,
    title: '夜曲',
    artist: '周杰伦',
    album: '十一月的萧邦',
    duration: 226,
    filePath: 'D:/Music/周杰伦/十一月的萧邦/夜曲.mp3',
    addedTime: 1716883200,
  },
  {
    id: 5,
    title: '青花瓷',
    artist: '周杰伦',
    album: '我很忙',
    duration: 239,
    filePath: 'D:/Music/周杰伦/我很忙/青花瓷.mp3',
    addedTime: 1716883200,
  },
]

let mockQueue = [mockSongs[0], mockSongs[1], mockSongs[2]]

// ===== 接口封装 =====

/**
 * 获取当前播放队列
 * @returns {Promise<SongInfo[]>}
 */
export function getQueue() {
  if (USE_MOCK) {
    return delay().then(() => [...mockQueue])
  }
  return request.get('/queue')
}

/**
 * 批量添加歌曲到队列末尾
 * @param {number[]} songIds 歌曲 ID 列表
 */
export function addToQueue(songIds) {
  if (USE_MOCK) {
    return delay().then(() => {
      const newSongs = mockSongs.filter(
        (s) => songIds.includes(s.id) && !mockQueue.some((q) => q.id === s.id)
      )
      mockQueue.push(...newSongs)
      return null
    })
  }
  return request.post('/queue/add', { songIds })
}

/**
 * 移除队列中指定索引的歌曲
 * @param {number} index 队列索引，从 0 开始
 */
export function removeFromQueue(index) {
  if (USE_MOCK) {
    return delay().then(() => {
      if (index < 0 || index >= mockQueue.length) {
        throw new Error('索引超出范围')
      }
      mockQueue.splice(index, 1)
      return null
    })
  }
  return request.delete(`/queue/${index}`)
}

/**
 * 清空播放队列
 */
export function clearQueue() {
  if (USE_MOCK) {
    return delay().then(() => {
      mockQueue = []
      return null
    })
  }
  return request.delete('/queue')
}

/**
 * 调整队列中歌曲位置
 * @param {number} from 原位置索引
 * @param {number} to 目标位置索引
 */
export function reorderQueue(from, to) {
  if (USE_MOCK) {
    return delay().then(() => {
      if (
        from < 0 ||
        from >= mockQueue.length ||
        to < 0 ||
        to >= mockQueue.length
      ) {
        throw new Error('索引超出范围')
      }
      const [moved] = mockQueue.splice(from, 1)
      mockQueue.splice(to, 0, moved)
      return null
    })
  }
  return request.post('/queue/reorder', null, { params: { from, to } })
}
