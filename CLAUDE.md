# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

59Music 是一个本地音乐播放器，采用 Vue 3 前端 + C++ 后端 + SQLite 数据库的架构。项目已完整实现所有核心模块：播放引擎、音乐库扫描与管理、歌单系统、背景管理、HTTP REST API + WebSocket 服务层、以及完整的 Vue 3 前端界面。

- 后端监听端口：**1059**（可通过命令行参数指定）
- 前端开发端口：**5173**（Vite 默认）
- 数据库：SQLite3，文件路径 `data/59music.db`（自动创建）

## Repository Layout

- `frontend/` — Vue 3 前端（完整应用）
  - `src/api/` — API 请求封装（player / library / queue / playlist / background）
  - `src/stores/` — Pinia 状态管理（player / library / playlist / background）
  - `src/views/` — 页面组件（音乐库 / 歌单列表 / 歌单详情 / 队列 / 设置 / 背景管理）
  - `src/composables/` — 组合式函数（快捷键 / WebSocket / 多选 / 拖拽排序）
  - `src/utils/` — 工具函数（时间格式化）
  - `src/router/` — Vue Router 路由定义
  - `src/mock/` — Mock 数据（开发调试用）
- `backend/` — C++ 后端源码与 CMake 构建配置
  - `backend/src/main.cpp` — 入口：启动 HTTP 服务
  - `backend/src/server/` — HTTP 服务层：REST API 路由注册、WebSocket 广播、JSON 响应封装、CORS
  - `backend/src/database/` — SQLite3 数据库层：建表（songs / playlists / playlist_songs / backgrounds）、CRUD、分页、搜索
  - `backend/src/scanner/` — 音乐库扫描：递归文件夹扫描 + TagLib 元数据解析
  - `backend/src/player/` — 播放引擎：AudioDecoder、PlayerEngine、PlayQueue、miniaudio 单头文件
  - `backend/src/models/` — 共享数据结构：SongInfo、PlayerState、PlayMode、BackgroundInfo
  - `backend/test/` — 播放引擎独立测试程序
- `docs/` — API 文档（`API文档v1.md`）与开发计划（`开发计划v1.md`）

## Common Commands

### Frontend

```bash
cd frontend
npm install
npm run dev        # 开发模式，访问 http://localhost:5173
npm run build      # 生产构建，输出到 dist/
```

> 注意：`package-lock.json` 已被 `.gitignore` 忽略，不要提交。

### Backend

进入后端目录后用 CMake 配置并构建（CMake 会自动 FetchContent 下载 TagLib 和 SQLite 源码）：

```bash
cd backend
mkdir -p build
cd build
cmake ..
cmake --build .
```

构建产物（Windows 下为 `.exe`）会生成在 `build/Debug/` 中，主程序为 `59music.exe`。

> 注意：`59music_server.exe` 与 `59music.exe` 源码完全相同，是历史遗留的重复目标，**使用 `59music.exe` 即可**。

启动服务：

```bash
cd backend/build/Debug
./59music.exe           # 默认端口 1059
./59music.exe 8899      # 指定端口
```

### Running Tests

后端目前没有统一的测试框架，使用独立的可执行测试程序。构建完成后：

- **PlayQueue 逻辑测试**（无需音频文件，推荐首先运行）：
  ```bash
  cd backend/build/Debug
  ./queue_test
  ```

- **AudioDecoder 解码测试**（需要本地音频文件）：
  ```bash
  cd backend/build/Debug
  ./audio_test <音频文件路径>
  ```

- **PlayerEngine 播放测试**（需要本地音频文件，会出声约 10 秒）：
  ```bash
  cd backend/build/Debug
  ./player_test <音频文件路径>
  ```

- **生成测试用 WAV 文件**（15 秒 440Hz 正弦波）：
  ```bash
  cd backend/build/Debug
  ./gen_test_wav
  # 生成 test_sine.wav，可用于 audio_test / player_test
  ```

## Architecture

### 模块划分

- **播放引擎**（`backend/src/player/`）：已完成
  - `AudioDecoder`：基于 miniaudio 的音频文件解码，支持读取 PCM、seek、查询时长/采样率/声道数。
  - `PlayerEngine`：基于 miniaudio 的 `ma_engine` + `ma_sound`，直接驱动扬声器，支持播放/暂停/seek/音量/静音/停止。
  - `PlayQueue`：纯逻辑，维护歌曲队列、当前索引和 5 种播放模式（顺序/倒序/单曲循环/列表循环/随机）。
- **音乐库**（`backend/src/scanner/`、`database/`）：已完成
  - `FileScanner`：递归扫描文件夹，支持 `.mp3` / `.flac` / `.wav`
  - `MetadataParser`：基于 TagLib 读取歌名/歌手/专辑/时长
  - `LibraryManager`：整合扫描 + 解析 + 入库，提供查询/删除接口
  - `Database`：SQLite3 封装，管理 songs / playlists / playlist_songs / backgrounds 四张表
- **服务层**（`backend/src/server/`）：已完成
  - HTTP REST API：播放控制、队列管理、音乐库管理、歌单管理、背景管理、文件上传
  - WebSocket：实时推送 player_state / progress / queue_changed / library_updated
  - CORS 支持，统一 JSON 响应格式 `{success, data/msg}`
- **前端**（`frontend/`）：已完成
  - Vue 3 + Vite + Element Plus + Pinia + vue-router
  - 页面：音乐库（搜索/扫描/编辑/拖拽排序/批量操作）、歌单（创建/详情/随机生成）、播放队列（拖拽排序/批量操作）、设置、背景管理
  - 播放栏：歌曲信息、播放/暂停/上下首、进度条、音量、播放模式切换、收藏、队列抽屉
  - 快捷键：空格（播放/暂停）、方向键（上下首/音量）
  - WebSocket 自动重连，状态实时同步

### API 与数据契约

- API 规范见 `docs/API文档v1.md`：基础路径 `http://localhost:1059/api/v1`，响应统一为 `{success, data}` 或 `{success, msg}`。
- WebSocket 地址 `ws://localhost:1059/ws`，消息类型包括 `player_state`、`progress`、`library_updated`、`queue_changed`。
- 共享模型定义在 `backend/src/models/`：
  - `SongInfo`：`id`、`title`、`artist`、`album`、`duration`、`filePath`、`addedTime`。
  - `PlayMode`：`Sequential`、`Reverse`、`SingleLoop`、`ListLoop`、`Shuffle`，字符串映射见 `PlayMode.h`。
  - `PlayerState`：包含当前歌曲、播放状态、进度、音量、静音、播放模式。
  - `BackgroundInfo`：`id`、`filePath`、`name`、`thumbnail`、`addedTime`、`isDefault`。

### 依赖关系

```
Database (P0) ── 最先完成，为上层的共同依赖
   ├─ 播放引擎（独立可测，不依赖数据库）
   ├─ 音乐库（依赖 Database）
   └─ 前端（依赖 API 文档，可用 mock 并行开发）
           │
           ▼
        服务层（整合所有模块，暴露 HTTP/WebSocket）
```

### 桌面应用套壳

- 前端最终目标形态为**桌面应用**（非浏览器访问），计划在 Vue 3 页面开发完成后通过套壳方案（Electron / Tauri / WebView2 等）打包为独立可执行文件。
- **当前阶段按常规 Vue 3 页面开发即可**，套壳集成放在项目后期统一处理，无需为套壳写特殊代码。
- 开发中预留以下套壳友好习惯：
  - 页面跳转使用 `vue-router`，不直接操作 `window.location`；
  - 静态资源与 API 路径使用相对路径，避免硬编码开发服务器地址；
  - Vite 构建的 `base` 当前为默认 `/`，后续套壳时可根据需要切换为 `./`；
  - 文件/文件夹选择功能先按浏览器能力实现（`<input type="file" webkitdirectory>`），套壳后可替换为原生对话框，但 `scanFolder(path)` 等接口契约保持不变；
  - 核心状态放在 Pinia 管理，不依赖浏览器专属 API（如 `document.cookie`）。

### 重要实现细节

- `miniaudio` 以单头文件形式内置，`MINIAUDIO_IMPLEMENTATION` 只在 `backend/src/player/miniaudio_impl.cpp` 中定义一次，其他文件不得重复定义。
- Windows 下构建需要链接系统库 `winmm` 和 `ole32`（已在 `CMakeLists.txt` 中配置）。
- `AudioDecoder` / `PlayerEngine` 使用 PImpl 隐藏 miniaudio 类型，并禁用拷贝、允许移动。
- `PlayQueue` 是独立的纯逻辑类，不直接播放音频；切歌后由上层调用 `PlayerEngine::play(filePath)` 出声。
- 后端默认监听 `127.0.0.1:1059`，仅本地访问；前端 Vite 开发服务器通过 `vite.config.js` 中的 proxy 配置将 `/api`、`/ws` 请求代理到后端。
- 生产环境下后端直接托管前端 `dist/` 目录的静态文件，无需分别启动前后端。
- `frontend/src/api/request.js` 中 `USE_MOCK` 标志控制是否使用 mock 数据，当前为 `false`（使用真实后端）。
