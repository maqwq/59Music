/**
 * API 模块统一入口
 *
 * 使用示例：
 *   import { playerApi, queueApi, libraryApi } from '@/api'
 *   const state = await playerApi.getState()
 */
export * as playerApi from './player'
export * as queueApi from './queue'
export * as libraryApi from './library'
export * as backgroundApi from './background'
export { USE_MOCK } from './request'
