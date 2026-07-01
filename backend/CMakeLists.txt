cmake_minimum_required(VERSION 3.16)
project(59Music LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 安全编译选项 + UTF-8 支持（源文件含中文字符串）
if(MSVC)
    add_compile_options(
        /utf-8          # 源文件 UTF-8 编码
        /GS             # 栈缓冲区溢出保护
        /DYNAMICBASE    # 地址空间随机化 (ASLR)
        /NXCOMPAT       # 数据执行保护 (DEP)
        /sdl            # 安全开发生命周期检查
    )
endif()

# ============ 第三方库（本地构建，无需 vcpkg）============

# SQLite3 — 单文件 amalgamation
add_library(sqlite3 STATIC third_party/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC third_party/sqlite3)

# nlohmann/json — 头文件库
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE third_party/json/include)

# cpp-httplib — 头文件库
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE third_party/cpp-httplib)

# TagLib — 从源码构建
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/taglib EXCLUDE_FROM_ALL)

# ============ 头文件搜索路径 ============

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/toolkit
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mpeg/id3v1
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mpeg/id3v2/frames
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ogg/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ogg/opus
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ogg/speex
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ogg/vorbis
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mpc
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/asf
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/ape
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/it
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/mod
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/dsdiff
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/taglib/dsf
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib/3rdparty/utfcpp/source
)

# ============ miniaudio 共享实现 ============

set(MINIAUDIO_LIBS winmm ole32)
set(MINIAUDIO_IMPL src/player/miniaudio_impl.cpp)

# ============ P2+P3: 音乐库服务器 ============

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
    src/player/PlayerEngine.h
    src/player/PlayerEngine.cpp
    src/player/PlayQueue.h
    src/player/PlayQueue.cpp
    src/player/AudioDecoder.h
    src/player/AudioDecoder.cpp
    src/models/SongInfo.h
    src/models/PlayerState.h
    src/models/PlayMode.h
    src/models/BackgroundInfo.h
    ${MINIAUDIO_IMPL}
    src/server/HttpServer.h
    src/server/HttpServer.cpp
    src/server/JsonHelper.h
)

target_link_libraries(59music PRIVATE
    sqlite3
    tag
    nlohmann_json
    httplib
    ${MINIAUDIO_LIBS}
    ws2_32
)

# ============ P1: 播放引擎测试 ============

add_executable(audio_test
    src/player/AudioDecoder.h
    src/player/AudioDecoder.cpp
    ${MINIAUDIO_IMPL}
    test/audio_test_main.cpp
)

add_executable(player_test
    src/player/PlayerEngine.h
    src/player/PlayerEngine.cpp
    ${MINIAUDIO_IMPL}
    test/player_test_main.cpp
)
target_link_libraries(player_test ${MINIAUDIO_LIBS})

add_executable(queue_test
    src/player/PlayQueue.h
    src/player/PlayQueue.cpp
    test/queue_test_main.cpp
)

add_executable(gen_test_wav
    test/gen_test_wav.cpp
)

