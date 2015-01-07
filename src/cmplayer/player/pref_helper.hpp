#ifndef PREF_HELPER_HPP
#define PREF_HELPER_HPP

#include "misc/json.hpp"
#include "player/mrlstate.hpp"
#include "video/hwacc.hpp"

template<class T>
struct PrefEditorProperty {
    static constexpr const auto name = "value";
};

#define DEC_SP(t, n) template<> struct PrefEditorProperty<t> \
    { static constexpr const auto name = n; };
DEC_SP(bool, "checked")
DEC_SP(QString, "text")
DEC_SP(QColor, "color")

template<class T>
struct PrefFieldIO {
    static auto toJson(const QVariant &t) -> QJsonValue
    {
        Q_ASSERT(t.canConvert<T>());
        return json_io<T>()->toJson(t.template value<T>());
    }
    static auto fromJson(QVariant &val, const QJsonValue &json) -> bool
    {
        auto t = val.template value<T>();
        auto ret = json_io<T>()->fromJson(t, json);
        val.setValue(t);
        return ret;
    }
};

template<>
struct JsonIO<QMetaProperty> {
    auto toJson(const QMetaProperty &prop) const -> QJsonValue
    { return _L(prop.name()); }
    auto fromJson(QMetaProperty &prop, const QJsonValue &json) const -> bool
    {
        const auto &mo = MrlState::staticMetaObject;
        const int idx = mo.indexOfProperty(json.toString().toLatin1());
        if (idx < 0)
            return false;
        prop = mo.property(idx);
        return true;
    }
    SCA qt_type = QJsonValue::String;
};

struct PrefFieldInfo {
    auto setFromEditor(QObject *p, const QObject *e) const -> void;
    auto setToEditor(const QObject *p, QObject *e) const -> void;
    auto editorName() const { return m_editor; }
    auto propertyName() const { return m_propertyName; }
    auto editorPropertyName() const { return m_editorProperty; }
    auto property() const -> const QMetaProperty& { return m_property; }
    auto toJson(const QObject *p) const -> QJsonValue;
    auto setFromJson(QObject *p, const QJsonValue &json) const -> bool;
    static auto getList() -> const QVector<const PrefFieldInfo*>&;
private:
    using ToJson = QJsonValue (*)(const QVariant&);
    using SetFromJson = bool (*)(QVariant&, const QJsonValue&);
    friend class Pref;
    PrefFieldInfo(QObject *p, ToJson toJson, SetFromJson setFromJson,
                  const char *property, const char *editorProperty);
    PrefFieldInfo(QObject *p, ToJson toJson, SetFromJson setFromJson,
                  const char *property, const char *editor, const char *editorProperty);
    QMetaProperty m_property;
    const char *const m_propertyName = nullptr;
    const char *const m_editor = nullptr;
    const char *const m_editorProperty = nullptr;
    ToJson m_toJson = nullptr;
    SetFromJson m_setFromJson = nullptr;
};

Q_DECLARE_METATYPE(const PrefFieldInfo*)

#endif // PREF_HELPER_HPP
