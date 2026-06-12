import { createRouter, createWebHistory } from 'vue-router'
import LibraryView from '../views/LibraryView.vue'
import QueueView from '../views/QueueView.vue'

const routes = [
  { path: '/', name: 'library', component: LibraryView },
  { path: '/queue', name: 'queue', component: QueueView },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
