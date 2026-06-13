#pragma once

#include <nlohmann/json.hpp>
#include "models/SongInfo.h"
#include "models/PlayerState.h"
#include "models/PlayMode.h"

namespace Music {

using Json = nlohmann::json;

// ============ SongInfo ↔ JSON ============

inline void to_json(Json& j, const SongInfo& s) {
    j = Json{
        {"id", s.id},
        {"filePath", s.filePath},
        {"title", s.title},
        {"artist", s.artist},
        {"album", s.album},
        {"duration", s.duration},
        {"addedTime", s.addedTime}
    };
}

inline void from_json(const Json& j, SongInfo& s) {
    j.at("id").get_to(s.id);
    j.at("filePath").get_to(s.filePath);
    j.at("title").get_to(s.title);
    j.at("artist").get_to(s.artist);
    j.at("album").get_to(s.album);
    j.at("duration").get_to(s.duration);
    j.at("addedTime").get_to(s.addedTime);
}

// ============ 响应构建辅助 ============

inline Json successResponse(const Json& data = nullptr) {
    Json resp;
    resp["success"] = true;
    if (!data.is_null()) {
        resp["data"] = data;
    } else {
        resp["data"] = nullptr;
    }
    return resp;
}

inline Json errorResponse(const std::string& msg) {
    Json resp;
    resp["success"] = false;
    resp["msg"] = msg;
    return resp;
}

// ============ 歌曲列表 → JSON 数组 ============

inline Json songsToJsonArray(const std::vector<SongInfo>& songs) {
    Json arr = Json::array();
    for (const auto& s : songs) {
        arr.push_back(s);
    }
    return arr;
}

} // namespace Music

// ============ PlayerState → JSON（全局命名空间，ADL 匹配）============

inline void to_json(Music::Json& j, const PlayerState& s) {
    j = Music::Json{
        {"isPlaying", s.isPlaying},
        {"currentSong", s.currentSong.has_value() ? Music::Json(*s.currentSong) : nullptr},
        {"currentPosition", s.currentPosition},
        {"duration", s.duration},
        {"volume", s.volume},
        {"muted", s.muted},
        {"mode", playModeToString(s.mode)}
    };
}
