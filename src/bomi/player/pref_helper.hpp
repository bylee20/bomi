#ifndef PREF_HELPER_HPP
#define PREF_HELPER_HPP

#include "misc/json.hpp"
#include "player/mrlstate.hpp"

template<class T>
struct PrefEditorProperty {
    static constexpr const auto name = "value";
};

#define DEC_SP(t, n) template<> struct PrefEditorProperty<t> \
    { static constexpr const auto name = n; };
DEC_SP(bool, "checked")
DEC_SP(QString, "text")
DEC_SP(QColor, "color")

//struct PrefFieldInfo {
//    auto setFromEditor(QObject *p, const QObject *e) const -> void;
//    auto setToEditor(const QObject *p, QObject *e) const -> void;
//    auto editor() const { return m_editor; }
//    auto property() const -> const QMetaProperty& { return m_property; }
//    static auto getList() -> const QVector<const PrefFieldInfo*>&;
//private:
//    friend class Pref;
//    PrefFieldInfo(QObject *p, const char *property, const char *editor);
//    QMetaProperty m_property;
//    const char *const m_editor = nullptr;
//};

//Q_DECLARE_METATYPE(const PrefFieldInfo*)

#endif // PREF_HELPER_HPP
