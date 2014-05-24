#ifndef OPENMEDIAINFO_HPP
#define OPENMEDIAINFO_HPP

#include "enum/openmediabehavior.hpp"

class QString;

struct OpenMediaInfo {
    OpenMediaInfo() = default;
    OpenMediaInfo(bool start, OpenMediaBehavior behavior)
        : start_playback(start), behavior(behavior) { }
    bool start_playback = true;
    OpenMediaBehavior behavior = OpenMediaBehavior::NewPlaylist;
    auto toJson() const -> QJsonObject;
    static auto fromJson(const QJsonObject &str) -> OpenMediaInfo;
};

#endif // OPENMEDIAINFO_HPP
