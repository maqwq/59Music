cmake_minimum_required(VERSION 3.16)
project(59Music LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件搜索路径：src/ 用于 #include "player/xxx.h"，src/player/ 用于 miniaudio.h
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player
)

# miniaudio engine 需要的系统库（Windows）
set(MINIAUDIO_LIBS winmm ole32)

# 后续其他库通过 vcpkg 引入
# find_package(miniaudio CONFIG ...)

# 所有用到 miniaudio 的目标共用此实现文件（避免重复编译）
set(MINIAUDIO_IMPL src/player/miniaudio_impl.cpp)

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
