import { createRouter, createWebHistory } from 'vue-router'
import LibraryView from '../views/LibraryView.vue'
import PlaylistListView from '../views/PlaylistListView.vue'
import PlaylistDetailView from '../views/PlaylistDetailView.vue'
import SettingsView from '../views/SettingsView.vue'
import BackgroundManageView from '../views/BackgroundManageView.vue'

const routes = [
  { path: '/', name: 'library', component: LibraryView },
  { path: '/playlists', name: 'playlists', component: PlaylistListView },
  { path: '/playlists/:id', name: 'playlistDetail', component: PlaylistDetailView },
  { path: '/settings', name: 'settings', component: SettingsView },
  { path: '/settings/backgrounds', name: 'backgroundManage', component: BackgroundManageView },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
