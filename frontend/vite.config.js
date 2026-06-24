import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [vue()],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:1059',
        changeOrigin: true,
      },
      '/ws': {
        target: 'ws://localhost:1059',
        ws: true,
      },
    },
  },
})
