cmake_minimum_required(VERSION 3.16)
project(59Music LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============ 第三方库 ============

find_package(SQLite3 REQUIRED)
find_package(TagLib REQUIRED)
find_package(nlohmann_json REQUIRED)

# ============ 头文件搜索路径 ============

# src/ 用于 #include "database/xxx.h"、"scanner/xxx.h"、"player/xxx.h"、"models/xxx.h"
# src/player/ 用于 miniaudio.h
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player
)

# ============ miniaudio 共享实现 ============

# miniaudio engine 需要的系统库（Windows）
set(MINIAUDIO_LIBS winmm ole32)

# 所有用到 miniaudio 的目标共用此实现文件（避免重复编译）
set(MINIAUDIO_IMPL src/player/miniaudio_impl.cpp)

# ============ P2: 音乐库 CLI ============

add_executable(59music
    src/main.cpp
    src/database/Database.h
    src/database/Database.cpp
    src/scanner/FileScanner.h
    src/scanner/FileScanner.cpp
    src/scanner/MetadataParser.h
    src/scanner/MetadataParser.cpp
    src/scanner/LibraryManager.h
    src/scanner/LibraryManager.cpp
    src/models/SongInfo.h
)

target_link_libraries(59music PRIVATE
    SQLite::SQLite3
    TagLib::tag
    nlohmann_json::nlohmann_json
)

# ============ P1: 播放引擎测试 ============

# audio_test：验证 AudioDecoder（解码，不出声）
add_executable(audio_test
    src/player/AudioDecoder.h
    src/player/AudioDecoder.cpp
    ${MINIAUDIO_IMPL}
    test/audio_test_main.cpp
)

# player_test：验证 PlayerEngine（播放，出声）
add_executable(player_test
    src/player/PlayerEngine.h
    src/player/PlayerEngine.cpp
    ${MINIAUDIO_IMPL}
    test/player_test_main.cpp
)
target_link_libraries(player_test ${MINIAUDIO_LIBS})

# queue_test：验证 PlayQueue（队列 + 播放模式，纯逻辑）
add_executable(queue_test
    src/player/PlayQueue.h
    src/player/PlayQueue.cpp
    test/queue_test_main.cpp
)

# 生成测试音频文件的工具
add_executable(gen_test_wav
    test/gen_test_wav.cpp
)
