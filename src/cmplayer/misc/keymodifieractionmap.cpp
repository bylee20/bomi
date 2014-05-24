#include "keymodifieractionmap.hpp"
#include "record.hpp"

KeyModifierActionMap::KeyModifierActionMap()
{
    const auto &list = EnumInfo<KeyModifier>::items();
    for (auto &item : list)
        m_map[item.value];
}

auto KeyModifierActionMap::toJson() const -> QJsonObject
{
    QJsonObject json;
    const auto &items = EnumInfo<KeyModifier>::items();
    for (auto &item : items) {
        const auto &info = m_map[item.value];
        QJsonObject obj;
        obj["enabled"] = info.enabled;
        obj["id"] = info.id;
        json[item.name] = obj;
    }
    return json;
}

auto KeyModifierActionMap::fromJson(const QJsonObject &json)
-> KeyModifierActionMap {
    const auto &items = EnumInfo<KeyModifier>::items();
    KeyModifierActionMap map;
    for (auto &item : items) {
        auto &info = map.m_map[item.value];
        auto obj = json[item.name].toObject();
        info.enabled = obj.value("enabled").toBool(info.enabled);
        info.id = obj.value("id").toString(info.id);
    }
    return map;
}

auto KeyModifierActionMap::save(Record &r, const QString &group) const -> void
{
    r.beginGroup(group);
    const auto &items = EnumInfo<KeyModifier>::items();
    for (auto &item : items) {
        const auto &info = m_map[item.value];
        r.beginGroup(item.name);
        r.write(info.enabled, "enabled");
        r.write(info.id, "id");
        r.endGroup();
    }
    r.endGroup();
}

auto KeyModifierActionMap::load(Record &r, const QString &group) -> void
{
    r.beginGroup(group);
    const auto &items = EnumInfo<KeyModifier>::items();
    for (auto &item : items) {
        auto &info = m_map[item.value];
        r.beginGroup(item.name);
        r.read(info.enabled, "enabled");
        r.read(info.id, "id");
        r.endGroup();
    }
    r.endGroup();
}
