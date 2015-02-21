#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include <QActionGroup>

class ActionGroup : public QActionGroup {
    Q_OBJECT
public:
    ActionGroup(QObject *parent = nullptr);
    auto trigger(const QVariant &data) -> QAction*;
    auto setChecked(const QVariant &data, bool checked) -> QAction*;
    auto find(const QVariant &data) const -> QAction*;

    template<class T>
    auto trigger(const T &t) -> QAction* { return trigger(QVariant::fromValue<T>(t)); }
    template<class T>
    auto setChecked(const T &t) -> QAction* { return setChecked(QVariant::fromValue<T>(t), true); }
    template<class T>
    auto find(const T &t) const -> QAction* { return find(QVariant::fromValue<T>(t)); }

    auto data() const -> QVariant;
    auto clear() -> void;
    auto firstCheckedAction() const -> QAction*;
    auto lastCheckedAction() const -> QAction*;
    auto checkedAction() const -> QAction* { return firstCheckedAction(); }
    auto setAllChecked(bool checked) -> void;
};

#endif // ACTIONGROUP_HPP
