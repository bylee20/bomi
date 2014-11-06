#include "keymodifieractionmap.hpp"
#include "json.hpp"

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
    return json_io(&m_map)->fromJson(m_map, json);
}
