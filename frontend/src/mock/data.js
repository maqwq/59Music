/**
 * 共享 Mock 数据
 *
 * 所有 API 模块共用同一份歌曲数据，
 * 避免 library.js / player.js / queue.js 重复定义。
 */

export const mockSongs = [
  { id: 1, title: '稻香', artist: '周杰伦', album: '魔杰座', duration: 223, filePath: 'D:/Music/周杰伦/魔杰座/稻香.mp3', addedTime: 1716883200 },
  { id: 2, title: '晴天', artist: '周杰伦', album: '叶惠美', duration: 269, filePath: 'D:/Music/周杰伦/叶惠美/晴天.mp3', addedTime: 1716883200 },
  { id: 3, title: '七里香', artist: '周杰伦', album: '七里香', duration: 299, filePath: 'D:/Music/周杰伦/七里香/七里香.mp3', addedTime: 1716883200 },
  { id: 4, title: '夜曲', artist: '周杰伦', album: '十一月的萧邦', duration: 226, filePath: 'D:/Music/周杰伦/十一月的萧邦/夜曲.mp3', addedTime: 1716883200 },
  { id: 5, title: '青花瓷', artist: '周杰伦', album: '我很忙', duration: 239, filePath: 'D:/Music/周杰伦/我很忙/青花瓷.mp3', addedTime: 1716883200 },
  { id: 6, title: '一路向北', artist: '周杰伦', album: '头文字D', duration: 252, filePath: 'D:/Music/周杰伦/头文字D/一路向北.mp3', addedTime: 1716883200 },
  { id: 7, title: '蒲公英的约定', artist: '周杰伦', album: '我很忙', duration: 246, filePath: 'D:/Music/周杰伦/我很忙/蒲公英的约定.mp3', addedTime: 1716883200 },
  { id: 8, title: '听妈妈的话', artist: '周杰伦', album: '依然范特西', duration: 258, filePath: 'D:/Music/周杰伦/依然范特西/听妈妈的话.mp3', addedTime: 1716883200 },
]
