#ifndef KEYMODIFIERACTIONMAP_HPP
#define KEYMODIFIERACTIONMAP_HPP

#include "enum/keymodifier.hpp"

enum class MouseBehavior;               class Record;

struct KeyModifierActionMap {
    KeyModifierActionMap();
    auto operator[](KeyModifier m) -> QString& {return m_map[m];}
    auto operator[](KeyModifier m) const -> QString {return m_map[m];}
    auto operator[](int id) const -> QString
        {return m_map[EnumInfo<KeyModifier>::from(id, KeyModifier::None)];}
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
private:
    QMap<KeyModifier, QString> m_map;
};

using MouseActionMap = QMap<MouseBehavior, KeyModifierActionMap>;

#endif // KEYMODIFIERACTIONMAP_HPP
