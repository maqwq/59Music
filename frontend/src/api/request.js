import axios from 'axios'

/**
 * API 请求基础配置
 *
 * Vite 开发服务器已配置代理：
 *   /api -> http://localhost:8080
 * 所以前端请求 /api/v1/xxx 会被转发到后端服务。
 */
const request = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
})

/**
 * 响应拦截器：统一处理 {success, data} / {success, msg} 格式
 */
request.interceptors.response.use(
  (response) => {
    const body = response.data
    if (body && body.success) {
      return body.data
    }
    const error = new Error(body?.msg || '请求失败')
    error.code = response.status
    return Promise.reject(error)
  },
  (error) => {
    const message = error.response?.data?.msg || error.message || '网络错误'
    return Promise.reject(new Error(message))
  }
)

/**
 * Mock 开关
 *
 * 设为 true 时使用本地 mock 数据，不发送真实请求；
 * 后端 P3 服务层完成后，改为 false 即可切换到真实 API。
 */
export const USE_MOCK = false

/**
 * 辅助函数：模拟网络延迟
 * @param {number} ms 延迟毫秒数
 */
export function delay(ms = 300) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

export default request
