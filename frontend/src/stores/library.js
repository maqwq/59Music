import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import * as libraryApi from '../api/library'

/**
 * 音乐库全局状态管理
 *
 * 管理：
 * - 歌曲列表、分页、搜索关键词
 * - 音乐库统计信息
 * - 扫描文件夹的状态
 */
export const useLibraryStore = defineStore('library', () => {
  // ========== State ==========

  /** 歌曲列表 */
  const songs = ref([])

  /** 总歌曲数 */
  const total = ref(0)

  /** 当前页码，从 1 开始 */
  const page = ref(1)

  /** 每页数量 */
  const pageSize = ref(50)

  /** 搜索关键词 */
  const keyword = ref('')

  /** 加载中状态 */
  const loading = ref(false)

  /** 扫描中状态 */
  const scanning = ref(false)

  /** 统计信息 */
  const stats = ref({
    totalSongs: 0,
    totalDuration: 0,
    totalArtists: 0,
    totalAlbums: 0,
  })

  // ========== Getters ==========

  /** 是否有歌曲 */
  const hasSongs = computed(() => songs.value.length > 0)

  /** 总页数 */
  const totalPages = computed(() => Math.ceil(total.value / pageSize.value))

  // ========== Actions ==========

  /**
   * 加载音乐库歌曲列表
   */
  async function loadSongs() {
    loading.value = true
    try {
      const result = await libraryApi.getSongs({
        page: page.value,
        size: pageSize.value,
        keyword: keyword.value,
      })
      songs.value = result.songs ?? []
      total.value = result.total ?? 0
      page.value = result.page ?? page.value
      pageSize.value = result.size ?? pageSize.value
    } catch (error) {
      console.error('加载音乐库失败:', error)
      songs.value = []
      total.value = 0
    } finally {
      loading.value = false
    }
  }

  /**
   * 设置搜索关键词并重新加载
   * @param {string} value
   */
  async function setKeyword(value) {
    keyword.value = value
    page.value = 1
    await loadSongs()
  }

  /**
   * 设置页码并重新加载
   * @param {number} newPage
   */
  async function setPage(newPage) {
    page.value = newPage
    await loadSongs()
  }

  /**
   * 设置每页数量并重新加载
   * @param {number} newSize
   */
  async function setPageSize(newSize) {
    pageSize.value = newSize
    page.value = 1
    await loadSongs()
  }

  /**
   * 扫描文件夹
   * @param {string} folder 文件夹路径
   */
  async function scanFolder(folder) {
    scanning.value = true
    try {
      const result = await libraryApi.scanFolder(folder)
      await loadSongs()
      await loadStats()
      return result
    } catch (error) {
      console.error('扫描文件夹失败:', error)
      throw error
    } finally {
      scanning.value = false
    }
  }

  /**
   * 从音乐库删除歌曲
   * @param {number} id 歌曲 ID
   */
  async function deleteSong(id) {
    try {
      await libraryApi.deleteSong(id)
      await loadSongs()
      await loadStats()
    } catch (error) {
      console.error('删除歌曲失败:', error)
      throw error
    }
  }

  /**
   * 加载统计信息
   */
  async function loadStats() {
    try {
      const data = await libraryApi.getStats()
      stats.value = data
    } catch (error) {
      console.error('加载统计信息失败:', error)
    }
  }

  /**
   * 重置搜索条件
   */
  async function resetSearch() {
    keyword.value = ''
    page.value = 1
    await loadSongs()
  }

  return {
    // state
    songs,
    total,
    page,
    pageSize,
    keyword,
    loading,
    scanning,
    stats,
    // getters
    hasSongs,
    totalPages,
    // actions
    loadSongs,
    setKeyword,
    setPage,
    setPageSize,
    scanFolder,
    deleteSong,
    loadStats,
    resetSearch,
  }
})
