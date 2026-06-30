#include "server/HttpServer.h"
#include "server/JsonHelper.h"
#include "database/Database.h"
#include "scanner/LibraryManager.h"
#include "player/PlayerEngine.h"
#include "player/PlayQueue.h"
#include "models/PlayMode.h"
#include "models/BackgroundInfo.h"

#include <httplib.h>
#include <sqlite3.h>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
static std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    return wide;
}
#endif

namespace Music {

// ====================================================================
// 生命周期
// ====================================================================

HttpServer::HttpServer()  = default;
HttpServer::~HttpServer() = default;

std::string HttpServer::defaultDbPath() {
    // 数据库放在 exe 同级目录，避免中文路径编码问题
    return "59music.db";
}

bool HttpServer::init(const std::string& dbPath) {
    std::string resolvedPath = dbPath.empty() ? defaultDbPath() : dbPath;
    // ── 数据库 + 音乐库 ──
    db_ = std::make_shared<Database>(resolvedPath);
    if (!db_->open()) {
        std::cerr << "[Server] 数据库打开失败" << std::endl;
        return false;
    }
    library_ = std::make_unique<LibraryManager>(db_);

    // ── 播放引擎 ──
    engine_ = std::make_unique<PlayerEngine>();
    if (!engine_->init()) {
        std::cerr << "[Server] 音频引擎初始化失败" << std::endl;
        return false;
    }

    // ── 播放队列 ──
    queue_ = std::make_unique<PlayQueue>();
    queue_->setSongResolver([this](int songId) {
        return db_->getSongById(songId);
    });
    queue_->setPlaylistResolver([this](int playlistId) {
        return db_->getPlaylistSongs(playlistId);
    });

    // ── HTTP 服务器 ──
    svr_ = std::make_unique<httplib::Server>();

    // 请求日志
    svr_->set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "[HTTP] " << req.method << " " << req.path
                  << " -> " << res.status << std::endl;
    });

    registerAllRoutes();

    // 全局 CORS 预检
    svr_->Options("/api/v1/.*", [this](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        res.status = 204;
    });

    std::cout << "[Server] 初始化完成" << std::endl;
    return true;
}

void HttpServer::run(int port) {
    if (!svr_) return;

    running_ = true;
    progressThread_ = std::thread(&HttpServer::progressLoop, this);

    std::cout << "[Server] 监听端口: " << port << std::endl;
    svr_->listen("127.0.0.1", port);

    // listen 返回后才执行
    running_ = false;
    if (progressThread_.joinable()) {
        progressThread_.join();
    }
}

void HttpServer::stop() {
    running_ = false;
    if (svr_) {
        svr_->stop();
    }
}

// ====================================================================
// CORS
// ====================================================================

void HttpServer::addCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ====================================================================
// 编排辅助
// ====================================================================

bool HttpServer::playSongById(int songId) {
    SongInfo song = library_->getSongById(songId);
    std::clog << "[LOG] playSongById id=" << songId << " found=" << (song.id != 0)
              << " path=" << song.filePath << std::endl;
    if (song.id == 0) return false;

    // 查找歌在队列 Song 类型项中的位置
    const auto& items = queue_->getItems();
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        if (items[i].type == QueueItemType::Song && items[i].songId == songId) {
            std::clog << "[LOG] 在队列中第 " << i << " 位" << std::endl;
            queue_->setCurrentIndex(i);
            bool ok = engine_->play(song.filePath);
            std::clog << "[LOG] 播放结果: " << ok << std::endl;
            return ok;
        }
    }
    // 不在队列中：加入队列末尾，再设为当前
    std::clog << "[LOG] 不在播放队列中，自动加入队列" << std::endl;
    queue_->addSongs({songId});
    queue_->setCurrentIndex(queue_->size() - 1);
    bool ok = engine_->play(song.filePath);
    std::clog << "[LOG] 播放结果: " << ok << std::endl;
    return ok;
}

void HttpServer::stopCurrentSong() {
    engine_->stop();
}

// ====================================================================
// WebSocket 广播
// ====================================================================

void HttpServer::wsBroadcast(const std::string& msg) {
    try {
        std::lock_guard<std::mutex> lock(wsMutex_);
        auto it = wsConnections_.begin();
        while (it != wsConnections_.end()) {
            try {
                if ((*it)->send(msg)) {
                    ++it;
                } else {
                    it = wsConnections_.erase(it);
                }
            } catch (...) {
                it = wsConnections_.erase(it);
            }
        }
    } catch (...) {
        // ignore
    }
}

void HttpServer::wsBroadcastPlayerState() {
    Json msg;
    msg["type"] = "player_state";
    msg["data"] = buildPlayerState();
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastProgress() {
    Json msg;
    msg["type"] = "progress";
    msg["data"] = {
        {"position", static_cast<int>(engine_->getCurrentPosition())},
        {"duration",  static_cast<int>(engine_->getDuration())}
    };
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastQueueChanged() {
    Json arr = Json::array();
    for (const auto& item : queue_->getItems()) {
        Json obj;
        obj["type"] = (item.type == QueueItemType::Song) ? "song" : "playlist";
        if (item.type == QueueItemType::Song) {
            obj["songId"] = item.songId;
            SongInfo s = db_->getSongById(item.songId);
            obj["song"] = s.id ? Json::parse(songsToJsonArray({s}).dump()).at(0) : Json(nullptr);
        } else {
            obj["playlistId"] = item.playlistId;
            obj["playlistName"] = item.playlistName;
            auto songs = db_->getPlaylistSongs(item.playlistId);
            obj["songs"] = Json::parse(songsToJsonArray(songs).dump());
        }
        arr.push_back(obj);
    }
    Json msg;
    msg["type"] = "queue_changed";
    msg["data"] = arr;
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastLibraryUpdated(int addedCount) {
    Json msg;
    msg["type"] = "library_updated";
    msg["data"] = {{"addedCount", addedCount}};
    wsBroadcast(msg.dump());
}

// ====================================================================
// PlayerState 快照
// ====================================================================

Json HttpServer::buildPlayerState() {
    PlayerState state;
    state.isPlaying  = engine_->isPlaying();
    state.currentSong = queue_->current();
    state.currentPosition = static_cast<int>(engine_->getCurrentPosition());
    state.duration   = static_cast<int>(engine_->getDuration());
    state.volume     = engine_->getVolume();
    state.muted      = engine_->isMuted();
    state.mode       = queue_->getPlayMode();
    return Json(state);
}

// ====================================================================
// 进度循环 + 歌曲结束自动切歌
// ====================================================================

void HttpServer::progressLoop() {
    std::clog << "[LOG] progressLoop 启动" << std::endl;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        try {
            std::lock_guard<std::mutex> lock(stateMutex_);

            engine_->updatePosition();   // 每秒刷新位置缓存

            if (!engine_->isInitialized() || !engine_->hasSong()) {
                wasPlaying_ = false;
                continue;
            }

            bool nowPlaying = engine_->isPlaying();
            std::clog << "[LOG] progress tick: nowPlaying=" << nowPlaying
                      << " wasPlaying=" << wasPlaying_
                      << " pausedByUser=" << pausedByUser_ << std::endl;

            if (nowPlaying) {
                std::clog << "[LOG] 广播进度: pos="
                          << static_cast<int>(engine_->getCurrentPosition())
                          << " dur=" << static_cast<int>(engine_->getDuration())
                          << std::endl;
                wsBroadcastProgress();
            }

            if (wasPlaying_ && !nowPlaying && !pausedByUser_) {
                std::clog << "[LOG] 歌曲结束，尝试切歌" << std::endl;
                auto nextSong = queue_->next();
                if (nextSong.has_value()) {
                    std::clog << "[LOG] 播放下一首: " << nextSong->title << std::endl;
                    engine_->play(nextSong->filePath);
                    wsBroadcastPlayerState();
                } else {
                    std::clog << "[LOG] 没有下一首" << std::endl;
                }
            }

            wasPlaying_ = nowPlaying;
        } catch (const std::exception& e) {
            std::cerr << "[LOG] progressLoop 异常: " << e.what() << std::endl;
        }
    }
    std::clog << "[LOG] progressLoop 退出" << std::endl;
}

// ====================================================================
// 路由注册
// ====================================================================

void HttpServer::registerAllRoutes() {
    // 辅助宏简化 handler 的 CORS + try 包装
    auto wrap = [this](std::function<void(const httplib::Request&, httplib::Response&)> fn) {
        return [this, fn](const httplib::Request& req, httplib::Response& res) {
            addCorsHeaders(res);
            try {
                fn(req, res);
            } catch (const std::exception& e) {
                res.status = 500;
                res.set_content(errorResponse(e.what()).dump(), "application/json");
            }
        };
    };

    // ======================== 播放器 API ========================

    svr_->Post("/api/v1/player/play", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::clog << "[LOG] POST /player/play" << std::endl;
        std::lock_guard lock(stateMutex_);
        auto songIdStr = req.get_param_value("songId");
        if (songIdStr.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 songId 参数").dump(), "application/json");
            return;
        }
        int songId = std::stoi(songIdStr);
        SongInfo song = library_->getSongById(songId);
        if (song.id == 0) {
            res.status = 404;
            res.set_content(errorResponse("歌曲不存在").dump(), "application/json");
            return;
        }
        if (!playSongById(songId)) {
            res.status = 400;
            res.set_content(errorResponse("播放失败，无法打开音频文件").dump(), "application/json");
            return;
        }
        pausedByUser_ = false;
        wasPlaying_ = true;
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/toggle", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        if (!engine_->hasSong()) {
            res.status = 400;
            res.set_content(errorResponse("没有加载歌曲").dump(), "application/json");
            return;
        }
        bool nowPlaying = engine_->toggle();
        pausedByUser_ = !nowPlaying;
        wasPlaying_ = nowPlaying;
        res.set_content(successResponse({{"isPlaying", nowPlaying}}).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/next", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto nextSong = queue_->next();
        if (!nextSong.has_value()) {
            res.status = 400;
            res.set_content(errorResponse("没有下一首").dump(), "application/json");
            return;
        }
        engine_->play(nextSong->filePath);
        pausedByUser_ = false;
        wasPlaying_ = true;
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/previous", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto prevSong = queue_->previous();
        if (!prevSong.has_value()) {
            res.status = 400;
            res.set_content(errorResponse("没有上一首").dump(), "application/json");
            return;
        }
        engine_->play(prevSong->filePath);
        pausedByUser_ = false;
        wasPlaying_ = true;
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/seek", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        if (!engine_->hasSong()) {
            res.status = 400;
            res.set_content(errorResponse("没有加载歌曲").dump(), "application/json");
            return;
        }
        auto posStr = req.get_param_value("position");
        if (posStr.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 position 参数").dump(), "application/json");
            return;
        }
        double pos = std::stod(posStr);
        double dur = engine_->getDuration();
        if (pos < 0) pos = 0;
        if (pos > dur) pos = dur;
        engine_->seek(pos);
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastProgress();
    }));

    svr_->Post("/api/v1/player/volume", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto volStr = req.get_param_value("volume");
        if (volStr.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 volume 参数").dump(), "application/json");
            return;
        }
        int vol = std::stoi(volStr);
        if (vol < 0) vol = 0;
        if (vol > 100) vol = 100;
        engine_->setVolume(vol);
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/mute", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto toggleStr = req.get_param_value("toggle");
        if (!toggleStr.empty() && toggleStr == "true") {
            // 切换静音
            bool muted = engine_->toggleMute();
            res.set_content(successResponse({{"muted", muted}}).dump(), "application/json");
        } else {
            // 设置静音状态
            auto mutedStr = req.get_param_value("muted");
            bool mute = (mutedStr == "true" || mutedStr == "1");
            engine_->setMuted(mute);
            res.set_content(successResponse(nullptr).dump(), "application/json");
        }
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/player/mode", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto modeStr = req.get_param_value("mode");
        if (modeStr.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 mode 参数").dump(), "application/json");
            return;
        }
        PlayMode mode = stringToPlayMode(modeStr);
        queue_->setPlayMode(mode);
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastPlayerState();
    }));

    svr_->Get("/api/v1/player/state", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        res.set_content(successResponse(buildPlayerState()).dump(), "application/json");
    }));

    // ======================== 播放队列 API ========================

    svr_->Get("/api/v1/queue", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        Json arr = Json::array();
        for (const auto& item : queue_->getItems()) {
            Json obj;
            if (item.type == QueueItemType::Song) {
                obj["type"] = "song";
                obj["songId"] = item.songId;
                SongInfo s = db_->getSongById(item.songId);
                if (s.id) {
                    obj["song"] = Json::parse(songsToJsonArray({s}).dump()).at(0);
                } else {
                    obj["song"] = nullptr;
                }
            } else {
                obj["type"] = "playlist";
                obj["playlistId"] = item.playlistId;
                obj["playlistName"] = item.playlistName;
                auto songs = db_->getPlaylistSongs(item.playlistId);
                obj["songs"] = Json::parse(songsToJsonArray(songs).dump());
            }
            arr.push_back(obj);
        }
        res.set_content(successResponse(arr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/queue/add", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("songIds")) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::vector<int> ids;
        for (const auto& idJson : body["songIds"]) {
            int id = idJson.get<int>();
            if (library_->getSongById(id).id > 0) {
                ids.push_back(id);
            }
        }
        int added = 0, skipped = static_cast<int>(ids.size());
        if (!ids.empty()) {
            added = queue_->addSongs(ids);
            skipped = static_cast<int>(ids.size()) - added;
        }
        res.set_content(successResponse({{"added", added}, {"skipped", skipped}}).dump(), "application/json");
        wsBroadcastQueueChanged();
    }));

    svr_->Post("/api/v1/queue/add-playlist", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("playlistId")) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        int playlistId = body["playlistId"].get<int>();
        auto info = db_->getPlaylistById(playlistId);
        if (info.id == 0) {
            res.status = 404;
            res.set_content(errorResponse("歌单不存在").dump(), "application/json");
            return;
        }
        auto songs = db_->getPlaylistSongs(playlistId);
        std::vector<int> songIds;
        for (auto& s : songs) songIds.push_back(s.id);
        int added = queue_->addSongs(songIds);
        int skipped = static_cast<int>(songIds.size()) - added;
        res.set_content(successResponse({
            {"added", added},
            {"skipped", skipped}
        }).dump(), "application/json");
        wsBroadcastQueueChanged();
    }));

    // DELETE /queue/{index} — cpp-httplib 用正则捕获
    svr_->Delete("/api/v1/queue/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        int index = std::stoi(req.matches[1]);
        if (!queue_->removeAt(index)) {
            res.status = 400;
            res.set_content(errorResponse("索引超出范围").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastQueueChanged();
    }));

    svr_->Delete("/api/v1/queue", wrap([this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        stopCurrentSong();
        queue_->clear();
        pausedByUser_ = true;
        wasPlaying_ = false;
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastQueueChanged();
        wsBroadcastPlayerState();
    }));

    svr_->Post("/api/v1/queue/reorder", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto fromStr = req.get_param_value("from");
        auto toStr   = req.get_param_value("to");
        if (fromStr.empty() || toStr.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 from/to 参数").dump(), "application/json");
            return;
        }
        int from = std::stoi(fromStr);
        int to   = std::stoi(toStr);
        if (!queue_->reorder(from, to)) {
            res.status = 400;
            res.set_content(errorResponse("索引超出范围").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastQueueChanged();
    }));

    // ======================== 音乐库 API ========================

    svr_->Post("/api/v1/library/scan", wrap([this](const httplib::Request& req, httplib::Response& res) {
        auto folder = req.get_param_value("folder");
        if (folder.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 folder 参数").dump(), "application/json");
            return;
        }
        // 路径穿越校验
        if (folder.find("..") != std::string::npos) {
            res.status = 400;
            res.set_content(errorResponse("非法路径").dump(), "application/json");
            return;
        }
        std::error_code ec;
#ifdef _WIN32
        std::wstring wFolder = utf8ToWide(folder);
        std::wstring wCanonical = std::filesystem::weakly_canonical(wFolder, ec).wstring();
        if (ec || !std::filesystem::exists(wCanonical, ec) || !std::filesystem::is_directory(wCanonical, ec)) {
            res.status = 400;
            res.set_content(errorResponse("文件夹不存在或不可访问").dump(), "application/json");
            return;
        }
        // 转回 UTF-8 给 LibraryManager 使用
        int wideLen = static_cast<int>(wCanonical.size());
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wCanonical.c_str(), wideLen, nullptr, 0, nullptr, nullptr);
        std::string canonical(utf8Len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wCanonical.c_str(), wideLen, &canonical[0], utf8Len, nullptr, nullptr);
#else
        std::string canonical = std::filesystem::weakly_canonical(folder, ec).string();
        if (ec || !std::filesystem::exists(canonical) || !std::filesystem::is_directory(canonical)) {
            res.status = 400;
            res.set_content(errorResponse("文件夹不存在或不可访问").dump(), "application/json");
            return;
        }
#endif
        int added = library_->scanAndImport(canonical);
        res.set_content(successResponse({{"addedCount", added}}).dump(), "application/json");
        wsBroadcastLibraryUpdated(added);
    }));

    svr_->Get("/api/v1/library/songs", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int page = 1, size = 50;
        auto p = req.get_param_value("page");
        auto s = req.get_param_value("size");
        auto keyword = req.get_param_value("keyword");

        if (!p.empty()) page = std::stoi(p);
        if (!s.empty()) size = std::stoi(s);

        int total = 0;
        std::vector<SongInfo> songs;
        if (keyword.empty()) {
            songs = library_->getSongsByPage(page, size, total);
        } else {
            songs = library_->searchSongs(keyword, page, size, total);
        }

        res.set_content(successResponse({
            {"total", total},
            {"page", page},
            {"size", size},
            {"songs", songsToJsonArray(songs)}
        }).dump(), "application/json");
    }));

    svr_->Delete("/api/v1/library/songs/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        if (!library_->deleteSong(id)) {
            res.status = 404;
            res.set_content(errorResponse("歌曲不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Put("/api/v1/library/songs/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::string title  = body.value("title", "");
        std::string artist = body.value("artist", "");
        if (!db_->updateSongMeta(id, title, artist)) {
            res.status = 404;
            res.set_content(errorResponse("歌曲不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/library/songs/reorder", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int from = std::stoi(req.get_param_value("from"));
        int to   = std::stoi(req.get_param_value("to"));

        if (!library_->reorderSongs(from, to)) {
            res.status = 400;
            res.set_content(errorResponse("索引超出范围").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Get("/api/v1/library/stats", wrap([this](const httplib::Request&, httplib::Response& res) {
        res.set_content(successResponse({
            {"totalSongs",    library_->getTotalSongs()},
            {"totalDuration", library_->getTotalDuration()},
            {"totalArtists",  library_->getTotalArtists()},
            {"totalAlbums",   library_->getTotalAlbums()}
        }).dump(), "application/json");
    }));

    // ======================== 歌单 ========================

    // 随机生成歌单 — 放在 /:id 之前，避免 "generate" 被当成 id
    svr_->Post("/api/v1/playlists/generate", wrap([this](const httplib::Request& req, httplib::Response& res) {
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        int count = body.value("count", 0);
        std::string name = body.value("name", "随机歌单");
        int total = db_->getTotalSongs();
        if (count <= 0 || count > total) {
            res.status = 400;
            res.set_content(errorResponse("数字需在 1 到 " + std::to_string(total) + " 之间").dump(), "application/json");
            return;
        }
        auto songs = db_->getRandomSongs(count);
        int playlistId = db_->createPlaylist(name);
        std::vector<int> songIds;
        for (auto& s : songs) songIds.push_back(s.id);
        db_->addSongsToPlaylist(playlistId, songIds);
        res.set_content(successResponse({{"id", playlistId}, {"songCount", static_cast<int>(songs.size())}}).dump(), "application/json");
    }));

    svr_->Get("/api/v1/playlists", wrap([this](const httplib::Request&, httplib::Response& res) {
        auto playlists = db_->getAllPlaylists();
        Json arr = Json::array();
        for (auto& p : playlists) {
            arr.push_back({
                {"id",        p.id},
                {"name",      p.name},
                {"songCount", p.songCount},
                {"createdAt", p.createdAt},
                {"updatedAt", p.updatedAt}
            });
        }
        res.set_content(successResponse(arr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/playlists", wrap([this](const httplib::Request& req, httplib::Response& res) {
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::string name = body.value("name", "新建歌单");
        int id = db_->createPlaylist(name);
        if (body.contains("songIds") && body["songIds"].is_array()) {
            std::vector<int> songIds;
            for (auto& sid : body["songIds"]) songIds.push_back(sid.get<int>());
            db_->addSongsToPlaylist(id, songIds);
        }
        auto info = db_->getPlaylistById(id);
        res.set_content(successResponse({
            {"id",        info.id},
            {"name",      info.name},
            {"songCount", info.songCount}
        }).dump(), "application/json");
    }));

    svr_->Get("/api/v1/playlists/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto info = db_->getPlaylistById(id);
        if (info.id == 0) {
            res.status = 404;
            res.set_content(errorResponse("歌单不存在").dump(), "application/json");
            return;
        }
        auto songs = db_->getPlaylistSongs(id);
        res.set_content(successResponse({
            {"id",        info.id},
            {"name",      info.name},
            {"songCount", info.songCount},
            {"songs",     songsToJsonArray(songs)}
        }).dump(), "application/json");
    }));

    svr_->Put("/api/v1/playlists/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::string name = body.value("name", "");
        if (!db_->renamePlaylist(id, name)) {
            res.status = 404;
            res.set_content(errorResponse("歌单不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Delete("/api/v1/playlists/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        if (!db_->deletePlaylist(id)) {
            res.status = 404;
            res.set_content(errorResponse("歌单不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/playlists/(\\d+)/songs", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("songIds")) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::vector<int> songIds;
        for (auto& sid : body["songIds"]) songIds.push_back(sid.get<int>());
        int added = db_->addSongsToPlaylist(id, songIds);
        res.set_content(successResponse({{"addedCount", added}}).dump(), "application/json");
    }));

    svr_->Delete("/api/v1/playlists/(\\d+)/songs/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int playlistId = std::stoi(req.matches[1]);
        int songId     = std::stoi(req.matches[2]);
        if (!db_->removeSongFromPlaylist(playlistId, songId)) {
            res.status = 404;
            res.set_content(errorResponse("歌曲不在歌单中").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/playlists/(\\d+)/reorder", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id   = std::stoi(req.matches[1]);
        int from = std::stoi(req.get_param_value("from"));
        int to   = std::stoi(req.get_param_value("to"));
        if (!db_->reorderPlaylistSongs(id, from, to)) {
            res.status = 400;
            res.set_content(errorResponse("索引超出范围").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Post("/api/v1/playlists/(\\d+)/add-to-queue", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto info = db_->getPlaylistById(id);
        if (info.id == 0) {
            res.status = 404;
            res.set_content(errorResponse("歌单不存在").dump(), "application/json");
            return;
        }
        auto songs = db_->getPlaylistSongs(id);
        std::vector<int> songIds;
        for (auto& s : songs) songIds.push_back(s.id);
        int added = queue_->addSongs(songIds);
        int skipped = static_cast<int>(songIds.size()) - added;
        wsBroadcastQueueChanged();
        res.set_content(successResponse({
            {"added", added},
            {"skipped", skipped}
        }).dump(), "application/json");
    }));

    // ======================== 背景 API ========================

    svr_->Get("/api/v1/backgrounds", wrap([this](const httplib::Request&, httplib::Response& res) {
        auto bgs = db_->getAllBackgrounds();
        Json arr = Json::array();
        for (const auto& bg : bgs) {
            arr.push_back(bg);
        }
        res.set_content(successResponse(arr).dump(), "application/json");
    }));

    svr_->Get("/api/v1/backgrounds/default", wrap([this](const httplib::Request&, httplib::Response& res) {
        auto bg = db_->getDefaultBackground();
        if (bg.id == 0) {
            res.set_content(successResponse(nullptr).dump(), "application/json");
        } else {
            res.set_content(successResponse(bg).dump(), "application/json");
        }
    }));

    svr_->Post("/api/v1/backgrounds", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.get_param_value("name");
        std::string filePath = req.get_param_value("filePath");

        if (filePath.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少 filePath 参数").dump(), "application/json");
            return;
        }

        if (filePath.find("..") != std::string::npos) {
            res.status = 400;
            res.set_content(errorResponse("非法路径").dump(), "application/json");
            return;
        }

        // 获取项目根目录
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path projectRoot = exePath;
        if (exePath.filename() == "Debug" && exePath.parent_path().filename() == "build") {
            projectRoot = exePath.parent_path().parent_path().parent_path();
        }

        // 判断是完整路径还是文件名
        std::string canonical;
        std::error_code ec;
        if (filePath.find('/') != std::string::npos || filePath.find('\\') != std::string::npos) {
            // 完整路径
#ifdef _WIN32
            std::wstring wPath = utf8ToWide(filePath);
            std::wstring wCanonical = std::filesystem::weakly_canonical(wPath, ec).wstring();
            if (ec || !std::filesystem::exists(wCanonical, ec) || !std::filesystem::is_regular_file(wCanonical, ec)) {
                res.status = 400;
                res.set_content(errorResponse("文件不存在或不可访问").dump(), "application/json");
                return;
            }
            int wideLen = static_cast<int>(wCanonical.size());
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wCanonical.c_str(), wideLen, nullptr, 0, nullptr, nullptr);
            canonical.resize(utf8Len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wCanonical.c_str(), wideLen, &canonical[0], utf8Len, nullptr, nullptr);
#else
            canonical = std::filesystem::weakly_canonical(filePath, ec).string();
            if (ec || !std::filesystem::exists(canonical) || !std::filesystem::is_regular_file(canonical)) {
                res.status = 400;
                res.set_content(errorResponse("文件不存在或不可访问").dump(), "application/json");
                return;
            }
#endif
        } else {
            // 文件名，拼接完整路径
            std::filesystem::path bgDir = projectRoot / "frontend" / "public" / "backgrounds";
            std::filesystem::path fullPath = bgDir / filePath;
            if (!std::filesystem::exists(fullPath, ec) || !std::filesystem::is_regular_file(fullPath, ec)) {
                res.status = 400;
                res.set_content(errorResponse("文件不存在或不可访问").dump(), "application/json");
                return;
            }
            canonical = filePath;  // 只保存文件名
        }

        if (name.empty()) {
            name = std::filesystem::path(canonical).filename().string();
        }

        BackgroundInfo bg;
        bg.filePath = canonical;
        bg.name = name;
        bg.addedTime = static_cast<int64_t>(std::time(nullptr));
        bg.isDefault = false;

        if (!db_->insertBackground(bg)) {
            res.status = 400;
            res.set_content(errorResponse("背景已存在").dump(), "application/json");
            return;
        }

        auto savedBg = db_->getBackgroundById(static_cast<int>(sqlite3_last_insert_rowid(db_->getDb())));
        res.set_content(successResponse(savedBg).dump(), "application/json");
    }));

    svr_->Post("/api/v1/backgrounds/(\\d+)/default", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        if (!db_->setDefaultBackground(id)) {
            res.status = 404;
            res.set_content(errorResponse("背景不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    svr_->Delete("/api/v1/backgrounds/(\\d+)", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        if (!db_->deleteBackground(id)) {
            res.status = 404;
            res.set_content(errorResponse("背景不存在").dump(), "application/json");
            return;
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
    }));

    // ======================== 文件上传 API ========================

    svr_->Post("/api/v1/upload/background", [this](const httplib::Request& req, httplib::Response& res) {
        // 检查是否有文件
        if (!req.form.has_file("file")) {
            res.status = 400;
            res.set_content(errorResponse("缺少文件").dump(), "application/json");
            return;
        }

        auto file = req.form.get_file("file");
        if (file.filename.empty()) {
            res.status = 400;
            res.set_content(errorResponse("缺少文件").dump(), "application/json");
            return;
        }

        // 生成唯一文件名
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::string ext = std::filesystem::path(file.filename).extension().string();
        std::string newFileName = std::to_string(timestamp) + "_" + std::filesystem::path(file.filename).stem().string() + ext;

        // 获取项目根目录（从可执行文件向上两级）
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path projectRoot = exePath;
        // 如果从 backend/build/Debug 启动，向上三级到项目根目录
        if (exePath.filename() == "Debug" && exePath.parent_path().filename() == "build") {
            projectRoot = exePath.parent_path().parent_path().parent_path();
        }

        // 确定保存目录（前端 public/backgrounds）
        std::filesystem::path saveDir = projectRoot / "frontend" / "public" / "backgrounds";
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);

        // 保存文件
        std::filesystem::path savePath = saveDir / newFileName;
        std::ofstream ofs(savePath, std::ios::binary);
        if (!ofs) {
            res.status = 500;
            res.set_content(errorResponse("保存文件失败").dump(), "application/json");
            return;
        }
        ofs.write(file.content.data(), file.content.size());
        ofs.close();

        // 返回文件路径（只返回文件名，不返回完整路径）
        Json data;
        data["fileName"] = newFileName;
        data["filePath"] = newFileName;  // 只保存文件名
        data["url"] = "/backgrounds/" + newFileName;
        res.set_content(successResponse(data).dump(), "application/json");
    });

    // 静态文件服务：背景图（使用绝对路径）
    {
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path projectRoot = exePath;
        if (exePath.filename() == "Debug" && exePath.parent_path().filename() == "build") {
            projectRoot = exePath.parent_path().parent_path().parent_path();
        }
        std::filesystem::path bgDir = projectRoot / "frontend" / "public" / "backgrounds";
        std::error_code ec;
        std::filesystem::create_directories(bgDir, ec);
        svr_->set_mount_point("/backgrounds", bgDir.string());
    }

    // ======================== WebSocket ========================

    svr_->WebSocket("/ws", [this](const httplib::Request&, httplib::ws::WebSocket& ws) {
        // 连接建立
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsConnections_.insert(&ws);
        }
        std::cout << "[WS] client connected (total: " << wsConnections_.size() << ")" << std::endl;

        // 推送当前状态（拿锁保护 engine_/queue_ 读取）
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            Json msg;
            msg["type"] = "player_state";
            msg["data"] = buildPlayerState();
            ws.send(msg.dump());
        }
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            Json arr = Json::array();
            for (const auto& item : queue_->getItems()) {
                Json obj;
                if (item.type == QueueItemType::Song) {
                    obj["type"] = "song";
                    obj["songId"] = item.songId;
                    SongInfo s = db_->getSongById(item.songId);
                    obj["song"] = s.id ? Json::parse(songsToJsonArray({s}).dump()).at(0) : Json(nullptr);
                } else {
                    obj["type"] = "playlist";
                    obj["playlistId"] = item.playlistId;
                    obj["playlistName"] = item.playlistName;
                    obj["songs"] = Json::parse(songsToJsonArray(db_->getPlaylistSongs(item.playlistId)).dump());
                }
                arr.push_back(obj);
            }
            Json msg;
            msg["type"] = "queue_changed";
            msg["data"] = arr;
            ws.send(msg.dump());
        }

        // 保持连接（阻塞读，客户端断开时 read 返回 false）
        std::string dummy;
        while (ws.read(dummy)) {
            // 忽略客户端消息，仅维持连接
        }

        // 断开
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsConnections_.erase(&ws);
        }
        std::cout << "[WS] client disconnected (total: " << wsConnections_.size() << ")" << std::endl;
    });
}

} // namespace Music
