#include <iostream>
#include <string>
#include <memory>
#include <iomanip>
#include "database/Database.h"
#include "scanner/LibraryManager.h"

using namespace Music;

static void printUsage() {
    std::cout << "59Music - 音乐库管理工具" << std::endl;
    std::cout << std::endl;
    std::cout << "用法:" << std::endl;
    std::cout << "  59music scan <文件夹路径>        扫描文件夹并导入歌曲" << std::endl;
    std::cout << "  59music list [页码] [每页数量]   分页列出歌曲" << std::endl;
    std::cout << "  59music search <关键词> [页码] [每页数量]  搜索歌曲" << std::endl;
    std::cout << "  59music stats                    查看统计信息" << std::endl;
    std::cout << "  59music delete <歌曲ID>          删除歌曲" << std::endl;
}

static void printSong(const SongInfo& s) {
    std::cout << "  [" << s.id << "] "
              << s.title << " - " << s.artist
              << "  (" << s.album << ")"
              << "  " << s.duration << "s"
              << std::endl;
}

static void printStats(std::shared_ptr<Database> db) {
    std::cout << "=== 音乐库统计 ===" << std::endl;
    std::cout << "歌曲总数: " << db->getTotalSongs() << std::endl;
    std::cout << "总时长:   " << db->getTotalDuration() << " 秒" << std::endl;
    std::cout << "歌手数:   " << db->getTotalArtists() << std::endl;
    std::cout << "专辑数:   " << db->getTotalAlbums() << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    // 打开数据库
    auto db = std::make_shared<Database>("data/59music.db");
    if (!db->open()) {
        std::cerr << "无法打开数据库，程序退出" << std::endl;
        return 1;
    }

    LibraryManager lib(db);

    if (command == "scan") {
        // ====== 扫描文件夹 ======
        if (argc < 3) {
            std::cerr << "用法: 59music scan <文件夹路径>" << std::endl;
            return 1;
        }
        std::string folderPath = argv[2];
        std::cout << "正在扫描: " << folderPath << " ..." << std::endl;

        int added = lib.scanAndImport(folderPath);
        std::cout << "扫描完成，新增 " << added << " 首歌曲" << std::endl;

    } else if (command == "list") {
        // ====== 分页列表 ======
        int page = (argc >= 3) ? std::stoi(argv[2]) : 1;
        int size = (argc >= 4) ? std::stoi(argv[3]) : 50;
        int total = 0;

        auto songs = lib.getSongsByPage(page, size, total);
        std::cout << "=== 歌曲列表 (第 " << page << " 页, 共 " << total << " 首) ===" << std::endl;
        for (const auto& s : songs) {
            printSong(s);
        }

    } else if (command == "search") {
        // ====== 关键词搜索 ======
        if (argc < 3) {
            std::cerr << "用法: 59music search <关键词> [页码] [每页数量]" << std::endl;
            return 1;
        }
        std::string keyword = argv[2];
        int page = (argc >= 4) ? std::stoi(argv[3]) : 1;
        int size = (argc >= 5) ? std::stoi(argv[4]) : 50;
        int total = 0;

        auto songs = lib.searchSongs(keyword, page, size, total);
        std::cout << "=== 搜索 \"" << keyword << "\" (第 " << page << " 页, 共 " << total << " 条) ===" << std::endl;
        for (const auto& s : songs) {
            printSong(s);
        }

    } else if (command == "stats") {
        // ====== 统计 ======
        printStats(db);

    } else if (command == "delete") {
        // ====== 删除 ======
        if (argc < 3) {
            std::cerr << "用法: 59music delete <歌曲ID>" << std::endl;
            return 1;
        }
        int id = std::stoi(argv[2]);
        if (lib.deleteSong(id)) {
            std::cout << "歌曲 ID=" << id << " 已删除" << std::endl;
        } else {
            std::cerr << "删除失败，歌曲 ID=" << id << " 可能不存在" << std::endl;
        }

    } else {
        std::cerr << "未知命令: " << command << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}
