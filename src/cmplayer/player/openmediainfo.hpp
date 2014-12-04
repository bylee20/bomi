#ifndef OPENMEDIAINFO_HPP
#define OPENMEDIAINFO_HPP

#include "enum/openmediabehavior.hpp"

class QString;

struct OpenMediaInfo {
    OpenMediaInfo() = default;
    OpenMediaInfo(OpenMediaBehavior behavior, bool start = true)
        : start_playback(start), behavior(behavior) { }
    auto operator == (const OpenMediaInfo &rhs) const -> bool
        { return start_playback == rhs.start_playback && behavior == rhs.behavior; }
    auto operator != (const OpenMediaInfo &rhs) const -> bool
        { return !operator == (rhs); }
    bool start_playback = true;
    OpenMediaBehavior behavior = OpenMediaBehavior::NewPlaylist;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &str) -> bool;
};

Q_DECLARE_METATYPE(OpenMediaInfo)

#endif // OPENMEDIAINFO_HPP
