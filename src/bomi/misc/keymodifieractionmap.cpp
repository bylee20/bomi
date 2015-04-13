#include "keymodifieractionmap.hpp"
#include "json.hpp"
#include "player/rootmenu.hpp"

KeyModifierActionMap::KeyModifierActionMap()
{
    const auto &list = EnumInfo<KeyModifier>::items();
    for (auto &item : list)
        m_map[item.value];
}

auto KeyModifierActionMap::toJson() const -> QJsonObject
{
    return json_io(&m_map)->toJson(m_map);
}

auto KeyModifierActionMap::setFromJson(const QJsonObject &json) -> bool
{
    if (!json_io(&m_map)->fromJson(m_map, json))
        return false;
    const auto &r = RootMenu::instance();
    for (auto &id : m_map)
        id = r.resolve(id);
    return true;
}
