#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// 前向声明
namespace httplib {
    class Server;
    struct Request;
    struct Response;
    namespace ws {
        class WebSocket;
    }
}

// PlayerEngine / PlayQueue 在全局命名空间
class PlayerEngine;
class PlayQueue;

namespace Music {

class Database;
class LibraryManager;
struct SongInfo;
using Json = nlohmann::json;

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    // 禁止拷贝和移动（持有线程+Socket 资源）
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) = delete;
    HttpServer& operator=(HttpServer&&) = delete;

    /** 获取默认数据库路径：%LOCALAPPDATA%/59Music/59music.db */
    static std::string defaultDbPath();

    bool init(const std::string& dbPath = "");
    void run(int port = 1059);
    void stop();

private:
    // ── 核心模块 ──
    std::shared_ptr<Database>       db_;
    std::unique_ptr<LibraryManager> library_;
    std::unique_ptr<::PlayerEngine> engine_;
    std::unique_ptr<::PlayQueue>    queue_;
    std::unique_ptr<httplib::Server> svr_;

    // ── WebSocket 连接池 ──
    std::mutex wsMutex_;
    std::set<httplib::ws::WebSocket*> wsConnections_;

    // ── 进度推送线程 ──
    std::thread progressThread_;
    std::atomic<bool> running_{false};
    std::mutex stateMutex_;
    bool pausedByUser_ = false;
    bool wasPlaying_ = false;

    // ── 路由注册 ──
    void registerAllRoutes();
    void addCorsHeaders(httplib::Response& res);

    // ── 编排辅助 ──
    bool playSongById(int songId);
    void stopCurrentSong();
    Json buildPlayerState();

    // ── WebSocket ──
    void wsBroadcast(const std::string& msg);
    void wsBroadcastPlayerState();
    void wsBroadcastProgress();
    void wsBroadcastQueueChanged();
    void wsBroadcastLibraryUpdated(int addedCount);

    // ── 进度循环 ──
    void progressLoop();
};

} // namespace Music
