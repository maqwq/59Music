/**
 * 格式化秒数为 mm:ss
 * @param {number} seconds
 * @returns {string}
 */
export function formatTime(seconds) {
  if (!seconds || seconds < 0) return '00:00'
  const m = Math.floor(seconds / 60)
  const s = Math.floor(seconds % 60)
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`
}

/**
 * 格式化总秒数为可读时长（用于统计展示）
 * @param {number} seconds
 * @returns {string}
 */
export function formatDuration(seconds) {
  if (!seconds || seconds < 0) return '0分钟'
  const h = Math.floor(seconds / 3600)
  const m = Math.floor((seconds % 3600) / 60)
  if (h > 0) return `${h}小时${m}分钟`
  return `${m}分钟`
}
