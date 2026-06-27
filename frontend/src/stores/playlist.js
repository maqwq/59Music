import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import * as playlistApi from '../api/playlist'
import { addPlaylistToQueue } from '../api/queue'

export const usePlaylistStore = defineStore('playlist', () => {
  // ===== State =====
  const playlists = ref([])
  const current = ref(null)       // { id, name, songCount, songs[] }
  const loading = ref(false)

  // ===== 我喜欢 =====
  const favoritePlaylistId = ref(null)
  const favoriteSongIds = ref(new Set())

  const isFavorite = computed(() => {
    return (songId) => favoriteSongIds.value.has(songId)
  })

  async function ensureFavoritePlaylist() {
    try {
      const allPlaylists = await playlistApi.getPlaylists()
      const existing = allPlaylists.find(p => p.name === '我喜欢')
      if (existing) {
        favoritePlaylistId.value = existing.id
      } else {
        const result = await playlistApi.createPlaylist({ name: '我喜欢' })
        favoritePlaylistId.value = result.id
      }
      await loadFavoriteSongs()
    } catch (e) {
      console.error('[playlist] ensureFavoritePlaylist failed:', e)
    }
  }

  async function loadFavoriteSongs() {
    if (!favoritePlaylistId.value) return
    try {
      const detail = await playlistApi.getPlaylist(favoritePlaylistId.value)
      favoriteSongIds.value = new Set((detail.songs || []).map(s => s.id))
    } catch {
      favoriteSongIds.value = new Set()
    }
  }

  async function toggleFavorite(songId) {
    if (!favoritePlaylistId.value) return
    if (favoriteSongIds.value.has(songId)) {
      await playlistApi.removeFromPlaylist(favoritePlaylistId.value, songId)
      favoriteSongIds.value.delete(songId)
      // 触发响应式更新
      favoriteSongIds.value = new Set(favoriteSongIds.value)
    } else {
      await playlistApi.addToPlaylist(favoritePlaylistId.value, [songId])
      favoriteSongIds.value.add(songId)
      favoriteSongIds.value = new Set(favoriteSongIds.value)
    }
    // 刷新歌单列表（更新歌曲数量）
    await loadPlaylists()
  }

  // ===== Actions =====

  async function loadPlaylists() {
    loading.value = true
    try {
      playlists.value = await playlistApi.getPlaylists()
    } catch {
      playlists.value = []
    } finally {
      loading.value = false
    }
  }

  async function loadPlaylist(id) {
    loading.value = true
    try {
      current.value = await playlistApi.getPlaylist(id)
    } catch {
      current.value = null
    } finally {
      loading.value = false
    }
  }

  async function create(name, songIds = []) {
    const result = await playlistApi.createPlaylist({ name, songIds })
    await loadPlaylists()
    return result
  }

  async function remove(id) {
    await playlistApi.deletePlaylist(id)
    await loadPlaylists()
  }

  async function rename(id, name) {
    await playlistApi.renamePlaylist(id, name)
    if (current.value && current.value.id === id) {
      current.value.name = name
    }
    await loadPlaylists()
  }

  async function addSongs(id, songIds) {
    await playlistApi.addToPlaylist(id, songIds)
    if (current.value && current.value.id === id) {
      await loadPlaylist(id)
    }
    await loadPlaylists()
  }

  async function removeSong(playlistId, songId) {
    await playlistApi.removeFromPlaylist(playlistId, songId)
    if (current.value && current.value.id === playlistId) {
      current.value.songs = current.value.songs.filter(s => s.id !== songId)
      current.value.songCount = current.value.songs.length
    }
    await loadPlaylists()
  }

  async function reorder(playlistId, from, to) {
    await playlistApi.reorderPlaylist(playlistId, from, to)
  }

  async function generateRandom(count, name) {
    const result = await playlistApi.generatePlaylist(count, name)
    await loadPlaylists()
    return result
  }

  async function addToQueue(playlistId) {
    const result = await addPlaylistToQueue(playlistId)
    return result
  }

  return {
    playlists,
    current,
    loading,
    favoritePlaylistId,
    favoriteSongIds,
    isFavorite,
    ensureFavoritePlaylist,
    loadFavoriteSongs,
    toggleFavorite,
    loadPlaylists,
    loadPlaylist,
    create,
    remove,
    rename,
    addSongs,
    removeSong,
    reorder,
    generateRandom,
    addToQueue,
  }
})
