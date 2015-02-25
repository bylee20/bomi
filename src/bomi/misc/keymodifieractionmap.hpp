#ifndef KEYMODIFIERACTIONMAP_HPP
#define KEYMODIFIERACTIONMAP_HPP

#include "enum/keymodifier.hpp"

enum class MouseBehavior;

struct KeyModifierActionMap {
    using Map = QMap<KeyModifier, QString>;
    KeyModifierActionMap();
    DECL_EQ(KeyModifierActionMap, &T::m_map)
    auto operator[](KeyModifier m) -> QString& {return m_map[m];}
    auto operator[](KeyModifier m) const -> QString {return m_map[m];}
    auto operator[](int id) const -> QString
        {return m_map[EnumInfo<KeyModifier>::from(id, KeyModifier::None)];}
    auto begin() const -> Map::const_iterator { return m_map.begin(); }
    auto end() const -> Map::const_iterator { return m_map.end(); }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
private:
    QMap<KeyModifier, QString> m_map;
};

using MouseActionMap = QMap<MouseBehavior, KeyModifierActionMap>;

Q_DECLARE_METATYPE(MouseActionMap)

#endif // KEYMODIFIERACTIONMAP_HPP
