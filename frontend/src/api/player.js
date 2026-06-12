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

let mockPlayerState = {
  isPlaying: false,
  currentSong: null,
  currentPosition: 0,
  duration: 0,
  volume: 80,
  muted: false,
  mode: 'sequential',
}

// ===== 接口封装 =====

/**
 * 播放指定歌曲
 * @param {number} songId 歌曲 ID
 */
export function play(songId) {
  if (USE_MOCK) {
    return delay().then(() => {
      const song = mockSongs.find((s) => s.id === songId)
      if (!song) {
        throw new Error('歌曲不存在')
      }
      mockPlayerState = {
        ...mockPlayerState,
        isPlaying: true,
        currentSong: song,
        currentPosition: 0,
        duration: song.duration,
      }
      return null
    })
  }
  return request.post('/player/play', null, { params: { songId } })
}

/**
 * 播放 / 暂停切换
 * @returns {Promise<{ isPlaying: boolean }>}
 */
export function toggle() {
  if (USE_MOCK) {
    return delay().then(() => {
      mockPlayerState.isPlaying = !mockPlayerState.isPlaying
      return { isPlaying: mockPlayerState.isPlaying }
    })
  }
  return request.post('/player/toggle')
}

/**
 * 下一首
 */
export function next() {
  if (USE_MOCK) {
    return delay().then(() => {
      if (!mockPlayerState.currentSong) return null
      const currentIndex = mockSongs.findIndex(
        (s) => s.id === mockPlayerState.currentSong.id
      )
      const nextIndex = (currentIndex + 1) % mockSongs.length
      const nextSong = mockSongs[nextIndex]
      mockPlayerState.currentSong = nextSong
      mockPlayerState.currentPosition = 0
      mockPlayerState.duration = nextSong.duration
      mockPlayerState.isPlaying = true
      return null
    })
  }
  return request.post('/player/next')
}

/**
 * 上一首
 */
export function previous() {
  if (USE_MOCK) {
    return delay().then(() => {
      if (!mockPlayerState.currentSong) return null
      const currentIndex = mockSongs.findIndex(
        (s) => s.id === mockPlayerState.currentSong.id
      )
      const prevIndex =
        (currentIndex - 1 + mockSongs.length) % mockSongs.length
      const prevSong = mockSongs[prevIndex]
      mockPlayerState.currentSong = prevSong
      mockPlayerState.currentPosition = 0
      mockPlayerState.duration = prevSong.duration
      mockPlayerState.isPlaying = true
      return null
    })
  }
  return request.post('/player/previous')
}

/**
 * 跳转到指定进度
 * @param {number} position 目标位置（秒）
 */
export function seek(position) {
  if (USE_MOCK) {
    return delay().then(() => {
      const duration = mockPlayerState.duration || 0
      mockPlayerState.currentPosition = Math.max(0, Math.min(position, duration))
      return null
    })
  }
  return request.post('/player/seek', null, { params: { position } })
}

/**
 * 设置音量
 * @param {number} volume 音量值 0-100
 */
export function setVolume(volume) {
  if (USE_MOCK) {
    return delay().then(() => {
      mockPlayerState.volume = Math.max(0, Math.min(volume, 100))
      return null
    })
  }
  return request.post('/player/volume', null, { params: { volume } })
}

/**
 * 设置播放模式
 * @param {'sequential' | 'single_loop' | 'list_loop' | 'shuffle'} mode 播放模式
 */
export function setMode(mode) {
  if (USE_MOCK) {
    return delay().then(() => {
      mockPlayerState.mode = mode
      return null
    })
  }
  return request.post('/player/mode', null, { params: { mode } })
}

/**
 * 获取播放器完整状态
 */
export function getState() {
  if (USE_MOCK) {
    return delay().then(() => ({ ...mockPlayerState }))
  }
  return request.get('/player/state')
}
