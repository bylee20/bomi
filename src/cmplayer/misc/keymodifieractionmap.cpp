#include "keymodifieractionmap.hpp"
#include "record.hpp"

KeyModifierActionMap::KeyModifierActionMap()
{
    const auto &list = EnumInfo<KeyModifier>::items();
    for (auto &item : list)
        m_map[item.value];
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
