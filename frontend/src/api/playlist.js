import request from './request'

// ===== 歌单 API =====

/** 获取所有歌单 */
export function getPlaylists() {
  return request.get('/playlists')
}

/** 创建歌单 */
export function createPlaylist(data) {
  return request.post('/playlists', data)
}

/** 获取歌单详情（含歌曲列表） */
export function getPlaylist(id) {
  return request.get(`/playlists/${id}`)
}

/** 修改歌单名称 */
export function renamePlaylist(id, name) {
  return request.put(`/playlists/${id}`, { name })
}

/** 删除歌单 */
export function deletePlaylist(id) {
  return request.delete(`/playlists/${id}`)
}

/** 向歌单添加歌曲 */
export function addToPlaylist(id, songIds) {
  return request.post(`/playlists/${id}/songs`, { songIds })
}

/** 从歌单移除歌曲 */
export function removeFromPlaylist(playlistId, songId) {
  return request.delete(`/playlists/${playlistId}/songs/${songId}`)
}

/** 调整歌单内歌曲顺序 */
export function reorderPlaylist(id, from, to) {
  return request.post(`/playlists/${id}/reorder`, null, { params: { from, to } })
}

/** 随机生成歌单 */
export function generatePlaylist(count, name) {
  return request.post('/playlists/generate', { count, name })
}
