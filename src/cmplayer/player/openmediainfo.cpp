#include "openmediainfo.hpp"

auto OpenMediaInfo::toJson() const -> QJsonObject
{
    QJsonObject json;
    json["start_playback"] = start_playback;
    json["behavior"] = _EnumName(behavior);
    return json;
}

auto OpenMediaInfo::fromJson(const QJsonObject &json) -> OpenMediaInfo
{
    OpenMediaInfo info;
    info.start_playback = json.value("start_playback").toBool(info.start_playback);
    info.behavior = _EnumFrom(json.value("behavior").toString(), info.behavior);
    return info;
}
