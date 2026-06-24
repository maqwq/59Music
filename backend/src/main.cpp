#include <iostream>
#include <csignal>
#include "server/HttpServer.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace Music;

static HttpServer* g_server = nullptr;

static void signalHandler(int /*sig*/) {
    std::cout << "\n[Server] 收到退出信号，正在关闭..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    int port = 1059;
    if (argc >= 2) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "端口号格式错误: " << argv[1] << std::endl;
            return 1;
        }
    }

    // 注册信号处理
    std::signal(SIGINT, signalHandler);
#ifdef _WIN32
    std::signal(SIGBREAK, signalHandler);
#endif

    HttpServer server;
    g_server = &server;

    if (!server.init()) {
        std::cerr << "服务器初始化失败" << std::endl;
        return 1;
    }

    std::cout << "59Music Server" << std::endl;
    std::cout << "  HTTP:      http://localhost:" << port << "/api/v1" << std::endl;
    std::cout << "  WebSocket: ws://localhost:" << port << "/ws" << std::endl;

    server.run(port);

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}
