# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

59Music 是一个本地音乐播放器，采用 Vue 3 前端 + C++ 后端 + SQLite 数据库的架构。当前处于早期开发阶段：后端播放引擎（P1）和基础数据模型已实现并可通过独立测试程序验证；前端仅完成 Vite 脚手架；数据库、音乐库扫描、HTTP/WebSocket 服务层尚未实现。

## Repository Layout

- `frontend/` — Vue 3 前端（Vite 脚手架）
- `backend/` — C++ 后端源码与 CMake 构建配置
- `backend/src/player/` — 播放引擎：AudioDecoder、PlayerEngine、PlayQueue、miniaudio 单头文件
- `backend/src/models/` — 共享数据结构：SongInfo、PlayerState、PlayMode
- `backend/src/database/`、`scanner/`、`server/` — 目前仅占位（.gitkeep），待实现
- `backend/test/` — 播放引擎独立测试程序
- `docs/` — API 文档（`API文档v1.md`）与开发计划（`开发计划v1.md`）

## Common Commands

### Frontend

```bash
cd frontend
npm install
npm run dev
```

> 注意：`package-lock.json` 已被 `.gitignore` 忽略，不要提交。

### Backend

进入后端目录后用 CMake 配置并构建（当前无需 vcpkg，miniaudio 已以单头文件形式内置在 `backend/src/player/miniaudio.h`）：

```bash
cd backend
mkdir -p build
cd build
cmake ..
cmake --build .
```

构建产物（Windows 下为 `.exe`）会生成在 `build/` 中。

### Running Tests

后端目前没有统一的测试框架，使用独立的可执行测试程序。构建完成后：

- **PlayQueue 逻辑测试**（无需音频文件，推荐首先运行）：
  ```bash
  cd backend/build
  ./queue_test
  ```

- **AudioDecoder 解码测试**（需要本地音频文件）：
  ```bash
  cd backend/build
  ./audio_test <音频文件路径>
  ```

- **PlayerEngine 播放测试**（需要本地音频文件，会出声约 10 秒）：
  ```bash
  cd backend/build
  ./player_test <音频文件路径>
  ```

- **生成测试用 WAV 文件**（15 秒 440Hz 正弦波）：
  ```bash
  cd backend/build
  ./gen_test_wav
  # 生成 test_sine.wav，可用于 audio_test / player_test
  ```

## Architecture

### 模块划分（按 `docs/开发计划v1.md`）

- **P1 播放引擎**（`backend/src/player/`）：已完成核心实现
  - `AudioDecoder`：基于 miniaudio 的音频文件解码，支持读取 PCM、seek、查询时长/采样率/声道数。
  - `PlayerEngine`：基于 miniaudio 的 `ma_engine` + `ma_sound`，直接驱动扬声器，支持播放/暂停/seek/音量/静音/停止。
  - `PlayQueue`：纯逻辑，维护歌曲队列、当前索引和 4 种播放模式（顺序/单曲循环/列表循环/随机）。
- **P2 音乐库**（`backend/src/scanner/`、`database/`）：待实现，负责文件夹扫描、TagLib 元数据解析、SQLite 入库与查询。
- **P3 服务层**（`backend/src/server/`）：待实现，负责通过 cpp-httplib 提供 REST API 和 WebSocket 状态推送。
- **P4 前端**（`frontend/`）：待实现，Vue 3 + Vite + Element Plus + Pinia + vue-router。

### API 与数据契约

- API 规范见 `docs/API文档v1.md`：基础路径 `http://localhost:8080/api/v1`，响应统一为 `{success, data}` 或 `{success, msg}`。
- WebSocket 地址 `ws://localhost:8080/ws`，消息类型包括 `player_state`、`progress`、`library_updated`、`queue_changed`。
- 共享模型定义在 `backend/src/models/`，与 API 文档中的字段保持一致：
  - `SongInfo`：`id`、`title`、`artist`、`album`、`duration`、`filePath`、`addedTime`。
  - `PlayMode`：`Sequential`、`SingleLoop`、`ListLoop`、`Shuffle`，字符串映射见 `PlayMode.h`。
  - `PlayerState`：包含当前歌曲、播放状态、进度、音量、静音、播放模式。

### 依赖关系

```
Database (P0)
   ├─ P1 播放引擎（独立可测）
   ├─ P2 音乐库（依赖 Database）
   └─ P4 前端（基于 API 文档可 mock 并行开发）
           │
           ▼
        P3 服务层（整合 P1 + P2，暴露 HTTP/WebSocket）
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
- Windows 下构建 `player_test` 需要链接系统库 `winmm` 和 `ole32`（已在 `CMakeLists.txt` 中配置）。
- `AudioDecoder` / `PlayerEngine` 使用 PImpl 隐藏 miniaudio 类型，并禁用拷贝、允许移动。
- `PlayQueue` 是独立的纯逻辑类，不直接播放音频；切歌后由上层调用 `PlayerEngine::play(filePath)` 出声。
