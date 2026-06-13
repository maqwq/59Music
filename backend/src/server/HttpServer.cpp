#include "server/HttpServer.h"
#include "server/JsonHelper.h"
#include "database/Database.h"
#include "scanner/LibraryManager.h"
#include "player/PlayerEngine.h"
#include "player/PlayQueue.h"
#include "models/PlayMode.h"

#include <httplib.h>
#include <iostream>
#include <chrono>

namespace Music {

// ====================================================================
// 生命周期
// ====================================================================

HttpServer::HttpServer()  = default;
HttpServer::~HttpServer() = default;

bool HttpServer::init(const std::string& dbPath) {
    // ── 数据库 + 音乐库 ──
    db_ = std::make_shared<Database>(dbPath);
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

    // ── HTTP 服务器 ──
    svr_ = std::make_unique<httplib::Server>();

    registerAllRoutes();

    // 全局 CORS 预检
    svr_->Options(R"(/api/v1/.*)", [this](const httplib::Request&, httplib::Response& res) {
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
    svr_->listen("0.0.0.0", port);

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

void HttpServer::playSongById(int songId) {
    SongInfo song = library_->getSongById(songId);
    if (song.id == 0) return;

    // 查找歌在队列中的位置
    const auto& q = queue_->getQueue();
    for (int i = 0; i < static_cast<int>(q.size()); ++i) {
        if (q[i].id == songId) {
            queue_->setCurrentIndex(i);
            engine_->play(song.filePath);
            return;
        }
    }
    // 不在队列中：直接播放，不入队
    engine_->play(song.filePath);
}

void HttpServer::stopCurrentSong() {
    engine_->stop();
}

// ====================================================================
// WebSocket 广播
// ====================================================================

void HttpServer::wsBroadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(wsMutex_);
    auto it = wsConnections_.begin();
    while (it != wsConnections_.end()) {
        if ((*it)->is_socket_open()) {
            (*it)->write(msg, httplib::WebSocketOpcode::text);
            ++it;
        } else {
            it = wsConnections_.erase(it);
        }
    }
}

void HttpServer::wsBroadcastPlayerState() {
    if (wsConnections_.empty()) return;
    Json msg;
    msg["type"] = "player_state";
    msg["data"] = buildPlayerState();
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastProgress() {
    if (wsConnections_.empty()) return;
    Json msg;
    msg["type"] = "progress";
    msg["data"] = {
        {"position", static_cast<int>(engine_->getCurrentPosition())},
        {"duration",  static_cast<int>(engine_->getDuration())}
    };
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastQueueChanged() {
    if (wsConnections_.empty()) return;
    Json msg;
    msg["type"] = "queue_changed";
    msg["data"] = songsToJsonArray(queue_->getQueue());
    wsBroadcast(msg.dump());
}

void HttpServer::wsBroadcastLibraryUpdated(int addedCount) {
    if (wsConnections_.empty()) return;
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
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::lock_guard<std::mutex> lock(stateMutex_);

        if (!engine_->isInitialized() || !engine_->hasSong()) {
            wasPlaying_ = false;
            continue;
        }

        bool nowPlaying = engine_->isPlaying();

        // 每秒推送播放进度
        if (nowPlaying) {
            wsBroadcastProgress();
        }

        // 检测歌曲自然结束（不是用户主动暂停）
        if (wasPlaying_ && !nowPlaying && !pausedByUser_) {
            auto nextSong = queue_->next();
            if (nextSong.has_value()) {
                engine_->play(nextSong->filePath);
                wsBroadcastPlayerState();
            }
        }

        wasPlaying_ = nowPlaying;
    }
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
        playSongById(songId);
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
        res.set_content(successResponse(songsToJsonArray(queue_->getQueue())).dump(), "application/json");
    }));

    svr_->Post("/api/v1/queue/add", wrap([this](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard lock(stateMutex_);
        auto body = Json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("songIds")) {
            res.status = 400;
            res.set_content(errorResponse("请求体格式错误").dump(), "application/json");
            return;
        }
        std::vector<SongInfo> songs;
        for (const auto& idJson : body["songIds"]) {
            int id = idJson.get<int>();
            SongInfo song = library_->getSongById(id);
            if (song.id > 0) {
                songs.push_back(song);
            }
        }
        if (!songs.empty()) {
            queue_->addSongs(songs);
        }
        res.set_content(successResponse(nullptr).dump(), "application/json");
        wsBroadcastQueueChanged();
    }));

    // DELETE /queue/{index} — cpp-httplib 用正则捕获: R"(/api/v1/queue/(\d+))"
    svr_->Delete(R"(/api/v1/queue/(\d+))", wrap([this](const httplib::Request& req, httplib::Response& res) {
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
        int added = library_->scanAndImport(folder);
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

    svr_->Delete(R"(/api/v1/library/songs/(\d+))", wrap([this](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        if (!library_->deleteSong(id)) {
            res.status = 404;
            res.set_content(errorResponse("歌曲不存在").dump(), "application/json");
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

    // ======================== WebSocket ========================

    svr_->ws("/ws", [this](const httplib::Request&, httplib::DataSink& sink) {
        // 连接建立
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsConnections_.insert(&sink);
        }
        std::cout << "[WS] 客户端连接 (总数: " << wsConnections_.size() << ")" << std::endl;

        // 推送当前状态给新客户端
        {
            Json msg;
            msg["type"] = "player_state";
            msg["data"] = buildPlayerState();
            sink.write(msg.dump(), httplib::WebSocketOpcode::text);
        }
        {
            Json msg;
            msg["type"] = "queue_changed";
            msg["data"] = songsToJsonArray(queue_->getQueue());
            sink.write(msg.dump(), httplib::WebSocketOpcode::text);
        }

        // 保持连接直到客户端断开（cpp-httplib 的 ws handler 返回即断开）
        while (sink.is_socket_open()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // 断开
        {
            std::lock_guard<std::mutex> lock(wsMutex_);
            wsConnections_.erase(&sink);
        }
        std::cout << "[WS] 客户端断开 (总数: " << wsConnections_.size() << ")" << std::endl;
    });
}

} // namespace Music
