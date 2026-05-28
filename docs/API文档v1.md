# 59Music API 文档

> 版本：v1 | 基础路径：`http://localhost:8080/api/v1`

---

## 1. 约定

### 1.1 响应格式

**成功响应：**
```json
{
  "success": true,
  "data": { ... }
}
```

**失败响应：**
```json
{
  "success": false,
  "msg": "错误描述"
}
```

### 1.2 参数传递

- 少量简单参数：URL 查询参数（`?key=value`）
- 批量数据/复杂对象：JSON 请求体

### 1.3 错误码约定

| HTTP 状态码 | 说明 |
|-------------|------|
| 200 | 请求成功 |
| 400 | 参数错误 |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |

---

## 2. 播放器控制 (Player)

### 2.1 播放指定歌曲

播放音乐库中的指定歌曲。

```
POST /player/play?songId={id}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| songId | int | 是 | 歌曲 ID |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**错误情况：**
- songId 不存在：`{"success": false, "msg": "歌曲不存在"}`

---

### 2.2 播放/暂停切换

切换当前播放状态，播放中变为暂停，暂停中变为播放。

```
POST /player/toggle
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": {
    "isPlaying": true
  }
}
```

---

### 2.3 下一首

切换到播放队列中的下一首歌曲，根据播放模式决定下一首。

```
POST /player/next
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

---

### 2.4 上一首

切换到播放队列中的上一首歌曲。

```
POST /player/previous
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

---

### 2.5 跳转进度

跳转到当前歌曲的指定时间位置。

```
POST /player/seek?position={seconds}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| position | int | 是 | 目标位置（秒） |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**错误情况：**
- position 超出歌曲时长：自动跳转到末尾

---

### 2.6 设置音量

设置播放音量。

```
POST /player/volume?volume={level}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| volume | int | 是 | 音量值，范围 0-100 |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

---

### 2.7 设置播放模式

切换播放模式。

```
POST /player/mode?mode={mode}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| mode | string | 是 | 播放模式 |

**mode 取值：**

| 值 | 说明 |
|-----|------|
| sequential | 顺序播放：队列播完即停 |
| single_loop | 单曲循环：当前曲播完从头开始 |
| list_loop | 列表循环：队列播完回到第一首继续 |
| shuffle | 随机播放：打乱队列顺序 |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

---

### 2.8 获取播放状态

获取当前播放器的完整状态信息。

```
GET /player/state
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": {
    "isPlaying": true,
    "currentSong": {
      "id": 1,
      "title": "稻香",
      "artist": "周杰伦",
      "album": "魔杰座",
      "duration": 223,
      "filePath": "D:/Music/稻香.mp3",
      "addedTime": 1716883200
    },
    "currentPosition": 60,
    "duration": 223,
    "volume": 80,
    "muted": false,
    "mode": "sequential"
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| isPlaying | bool | 是否正在播放 |
| currentSong | object | 当前歌曲信息，无歌曲时为 null |
| currentSong.id | int | 歌曲 ID |
| currentSong.title | string | 歌名 |
| currentSong.artist | string | 歌手 |
| currentSong.album | string | 专辑 |
| currentSong.duration | int | 歌曲时长（秒） |
| currentSong.filePath | string | 文件路径 |
| currentSong.addedTime | int | 加入时间戳（Unix 秒） |
| currentPosition | int | 当前播放进度（秒） |
| duration | int | 当前歌曲总时长（秒） |
| volume | int | 音量 0-100 |
| muted | bool | 是否静音 |
| mode | string | 播放模式 |

---

## 3. 播放队列 (Queue)

### 3.1 获取播放队列

获取当前播放队列中的所有歌曲。

```
GET /queue
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "title": "稻香",
      "artist": "周杰伦",
      "album": "魔杰座",
      "duration": 223,
      "filePath": "D:/Music/稻香.mp3",
      "addedTime": 1716883200
    },
    {
      "id": 2,
      "title": "晴天",
      "artist": "周杰伦",
      "album": "叶惠美",
      "duration": 269,
      "filePath": "D:/Music/晴天.mp3",
      "addedTime": 1716883200
    }
  ]
}
```

---

### 3.2 添加歌曲到队列

批量添加歌曲到播放队列末尾。

```
POST /queue/add
```

**请求体：**
```json
{
  "songIds": [1, 2, 3]
}
```

**参数说明：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| songIds | int[] | 是 | 要添加的歌曲 ID 列表 |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**错误情况：**
- songIds 中有不存在的 ID：忽略不存在的 ID，添加存在的

---

### 3.3 移除队列中的歌曲

根据索引移除队列中的指定歌曲。

```
DELETE /queue/{index}
```

**路径参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| index | int | 队列中的位置索引，从 0 开始 |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**错误情况：**
- index 超出范围：`{"success": false, "msg": "索引超出范围"}`

---

### 3.4 清空播放队列

清空整个播放队列，停止当前播放。

```
DELETE /queue
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

---

### 3.5 队列排序

调整队列中歌曲的位置。

```
POST /queue/reorder?from={from}&to={to}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| from | int | 是 | 原位置索引 |
| to | int | 是 | 目标位置索引 |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**示例：**
- `POST /queue/reorder?from=3&to=1`：将第 4 首移到第 2 位

---

## 4. 音乐库 (Library)

### 4.1 扫描文件夹

扫描指定文件夹，将发现的音频文件添加到音乐库。

```
POST /library/scan?folder={path}
```

**参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| folder | string | 是 | 文件夹路径 |

**成功响应：**
```json
{
  "success": true,
  "data": {
    "addedCount": 15
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| addedCount | int | 本次新增歌曲数量 |

**说明：**
- 支持递归扫描子文件夹
- 支持的格式：`.mp3`、`.flac`、`.wav`
- 已存在的歌曲（根据 file_path 去重）不会重复添加

---

### 4.2 查询歌曲列表

分页查询音乐库中的歌曲，支持关键词搜索。

```
GET /library/songs?page={page}&size={size}&keyword={keyword}
```

**参数：**

| 参数 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| page | int | 否 | 1 | 页码，从 1 开始 |
| size | int | 否 | 50 | 每页数量 |
| keyword | string | 否 | - | 搜索关键词，搜索歌名/歌手/专辑 |

**成功响应：**
```json
{
  "success": true,
  "data": {
    "total": 150,
    "page": 1,
    "size": 50,
    "songs": [
      {
        "id": 1,
        "title": "稻香",
        "artist": "周杰伦",
        "album": "魔杰座",
        "duration": 223,
        "filePath": "D:/Music/稻香.mp3",
        "addedTime": 1716883200
      }
    ]
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| total | int | 符合条件的歌曲总数 |
| page | int | 当前页码 |
| size | int | 每页数量 |
| songs | array | 歌曲列表 |

---

### 4.3 删除歌曲

从音乐库中移除歌曲（不删除原文件）。

```
DELETE /library/songs/{id}
```

**路径参数：**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | int | 歌曲 ID |

**成功响应：**
```json
{
  "success": true,
  "data": null
}
```

**错误情况：**
- id 不存在：`{"success": false, "msg": "歌曲不存在"}`

**注意：** 只从数据库中删除记录，不会删除本地音频文件。

---

### 4.4 获取统计信息

获取音乐库的统计数据。

```
GET /library/stats
```

**参数：** 无

**成功响应：**
```json
{
  "success": true,
  "data": {
    "totalSongs": 150,
    "totalDuration": 324000,
    "totalArtists": 45,
    "totalAlbums": 30
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| totalSongs | int | 歌曲总数 |
| totalDuration | int | 总时长（秒） |
| totalArtists | int | 歌手数量 |
| totalAlbums | int | 专辑数量 |

---

## 5. WebSocket 推送

### 5.1 连接信息

- **地址：** `ws://localhost:8080/ws`
- **协议：** WebSocket

### 5.2 消息格式

所有推送消息均为 JSON 格式，包含 `type` 字段标识消息类型：

```json
{
  "type": "消息类型",
  "data": { ... }
}
```

### 5.3 消息类型

#### player_state

播放状态变化时推送（播放/暂停/切歌/连接时）。

**触发时机：**
- 播放/暂停
- 切换歌曲
- WebSocket 连接建立时

**数据格式：**
```json
{
  "type": "player_state",
  "data": {
    "isPlaying": true,
    "currentSong": {
      "id": 1,
      "title": "稻香",
      "artist": "周杰伦",
      "album": "魔杰座",
      "duration": 223,
      "filePath": "D:/Music/稻香.mp3",
      "addedTime": 1716883200
    },
    "currentPosition": 60,
    "duration": 223,
    "volume": 80,
    "muted": false,
    "mode": "sequential"
  }
}
```

---

#### progress

播放进度更新，每秒推送一次。

**触发时机：** 播放中每秒触发

**数据格式：**
```json
{
  "type": "progress",
  "data": {
    "position": 61,
    "duration": 240
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| position | int | 当前播放位置（秒） |
| duration | int | 歌曲总时长（秒） |

---

#### library_updated

音乐库更新通知。

**触发时机：** 文件夹扫描完成

**数据格式：**
```json
{
  "type": "library_updated",
  "data": {
    "addedCount": 15
  }
}
```

**字段说明：**

| 字段 | 类型 | 说明 |
|------|------|------|
| addedCount | int | 本次新增歌曲数量 |

---

#### queue_changed

播放队列变化通知。

**触发时机：** 队列增删改操作后

**数据格式：**
```json
{
  "type": "queue_changed",
  "data": [
    {
      "id": 1,
      "title": "稻香",
      "artist": "周杰伦",
      "album": "魔杰座",
      "duration": 223,
      "filePath": "D:/Music/稻香.mp3",
      "addedTime": 1716883200
    }
  ]
}
```

---

## 附录：数据结构

### SongInfo（歌曲信息）

| 字段 | 类型 | 说明 |
|------|------|------|
| id | int | 数据库主键，自增 |
| filePath | string | 音频文件绝对路径 |
| title | string | 歌名 |
| artist | string | 歌手 |
| album | string | 专辑 |
| duration | int | 时长（秒） |
| addedTime | int | 加入时间戳（Unix 秒） |

### PlayMode（播放模式）

| 值 | 说明 |
|-----|------|
| sequential | 顺序播放：队列播完即停 |
| single_loop | 单曲循环：当前曲播完从头开始 |
| list_loop | 列表循环：队列播完回到第一首继续 |
| shuffle | 随机播放：打乱队列顺序 |
