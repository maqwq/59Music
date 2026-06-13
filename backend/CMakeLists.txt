cmake_minimum_required(VERSION 3.16)
project(59Music LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============ 第三方库 ============

find_package(SQLite3 REQUIRED)
find_package(TagLib REQUIRED)
find_package(nlohmann_json REQUIRED)

# ============ 源文件 ============

set(SOURCES
    src/main.cpp
    src/database/Database.cpp
    src/scanner/FileScanner.cpp
    src/scanner/MetadataParser.cpp
    src/scanner/LibraryManager.cpp
)

set(HEADERS
    src/models/SongInfo.h
    src/database/Database.h
    src/scanner/FileScanner.h
    src/scanner/MetadataParser.h
    src/scanner/LibraryManager.h
)

# ============ 可执行文件 ============

add_executable(59music ${SOURCES} ${HEADERS})

target_include_directories(59music PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(59music PRIVATE
    SQLite::SQLite3
    TagLib::tag
    nlohmann_json::nlohmann_json
)
