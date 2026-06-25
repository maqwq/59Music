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

let mockLibrary = [...mockSongs]

// ===== 接口封装 =====

/**
 * 扫描指定文件夹，将发现的音频文件添加到音乐库
 * @param {string} folder 文件夹路径
 * @returns {Promise<{ addedCount: number }>}
 */
export function scanFolder(folder) {
  if (USE_MOCK) {
    return delay(800).then(() => {
      console.log(`[mock] 扫描文件夹：${folder}`)
      return { addedCount: 5 }
    })
  }
  // 扫描可能耗时较长，不限超时
  return request.post('/library/scan', null, { params: { folder }, timeout: 0 })
}

/**
 * 分页查询音乐库歌曲
 * @param {Object} params
 * @param {number} [params.page=1] 页码
 * @param {number} [params.size=50] 每页数量
 * @param {string} [params.keyword] 搜索关键词
 * @returns {Promise<{ total: number, page: number, size: number, songs: SongInfo[] }>}
 */
export function getSongs(params = {}) {
  if (USE_MOCK) {
    return delay().then(() => {
      const page = params.page || 1
      const size = params.size || 50
      const keyword = (params.keyword || '').toLowerCase()

      let list = mockLibrary
      if (keyword) {
        list = list.filter(
          (s) =>
            s.title.toLowerCase().includes(keyword) ||
            s.artist.toLowerCase().includes(keyword) ||
            s.album.toLowerCase().includes(keyword)
        )
      }

      const total = list.length
      const start = (page - 1) * size
      const songs = list.slice(start, start + size)

      return { total, page, size, songs }
    })
  }
  return request.get('/library/songs', { params })
}

/**
 * 从音乐库中移除歌曲（不删除原文件）
 * @param {number} id 歌曲 ID
 */
export function deleteSong(id) {
  if (USE_MOCK) {
    return delay().then(() => {
      const index = mockLibrary.findIndex((s) => s.id === id)
      if (index === -1) {
        throw new Error('歌曲不存在')
      }
      mockLibrary.splice(index, 1)
      return null
    })
  }
  return request.delete(`/library/songs/${id}`)
}

/**
 * 调整音乐库歌曲顺序
 * @param {number} from 原位置索引
 * @param {number} to 目标位置索引
 */
export function reorderSongs(from, to) {
  if (USE_MOCK) {
    return delay().then(() => {
      if (
        from < 0 ||
        from >= mockLibrary.length ||
        to < 0 ||
        to >= mockLibrary.length
      ) {
        throw new Error('索引超出范围')
      }
      const [moved] = mockLibrary.splice(from, 1)
      mockLibrary.splice(to, 0, moved)
      return null
    })
  }
  return request.post('/library/songs/reorder', null, { params: { from, to } })
}

/**
 * 获取音乐库统计信息
 * @returns {Promise<{ totalSongs: number, totalDuration: number, totalArtists: number, totalAlbums: number }>}
 */
export function getStats() {
  if (USE_MOCK) {
    return delay().then(() => {
      const artists = new Set(mockLibrary.map((s) => s.artist))
      const albums = new Set(mockLibrary.map((s) => s.album))
      const totalDuration = mockLibrary.reduce((sum, s) => sum + s.duration, 0)

      return {
        totalSongs: mockLibrary.length,
        totalDuration,
        totalArtists: artists.size,
        totalAlbums: albums.size,
      }
    })
  }
  return request.get('/library/stats')
}
