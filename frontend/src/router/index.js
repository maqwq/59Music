import { createRouter, createWebHistory } from 'vue-router'
import LibraryView from '../views/LibraryView.vue'
import QueueView from '../views/QueueView.vue'
import PlaylistListView from '../views/PlaylistListView.vue'
import PlaylistDetailView from '../views/PlaylistDetailView.vue'

const routes = [
  { path: '/', name: 'library', component: LibraryView },
  { path: '/playlists', name: 'playlists', component: PlaylistListView },
  { path: '/playlists/:id', name: 'playlistDetail', component: PlaylistDetailView },
  { path: '/queue', name: 'queue', component: QueueView },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
