/**
 * 背景图 API
 */

import request from './request'

/**
 * 获取所有背景图
 */
export function getAllBackgrounds() {
  return request.get('/backgrounds')
}

/**
 * 获取默认背景图
 */
export function getDefaultBackground() {
  return request.get('/backgrounds/default')
}

/**
 * 添加背景图
 * @param {Object} data - { filePath, name }
 */
export function addBackground(data) {
  return request.post('/backgrounds', null, { params: data })
}

/**
 * 删除背景图
 * @param {number} id
 */
export function deleteBackground(id) {
  return request.delete(`/backgrounds/${id}`)
}

/**
 * 设置默认背景图
 * @param {number} id
 */
export function setDefaultBackground(id) {
  return request.post(`/backgrounds/${id}/default`)
}
