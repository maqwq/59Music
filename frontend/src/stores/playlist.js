import { defineStore } from 'pinia'
import { ref } from 'vue'
import * as playlistApi from '../api/playlist'
import { addPlaylistToQueue } from '../api/queue'

export const usePlaylistStore = defineStore('playlist', () => {
  // ===== State =====
  const playlists = ref([])
  const current = ref(null)       // { id, name, songCount, songs[] }
  const loading = ref(false)

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
