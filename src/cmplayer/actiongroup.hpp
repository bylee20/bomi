#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include "stdafx.hpp"

class Mrl;

class ActionGroup : public QActionGroup {
    Q_OBJECT
public:
    ActionGroup(QObject *parent = 0);
    auto setChecked(const QVariant &data, bool checked) -> void;
    auto trigger(double data) -> void;
    auto trigger(const QVariant &data) -> void;
    template<class T> auto trigger(const T &t) -> void { trigger(QVariant::fromValue<T>(t)); }
    auto data() const -> QVariant;
    auto clear() -> void;
    QAction *find(const QVariant &data) const;
    template<class T> QAction *find(const T &t) const { return find(QVariant::fromValue<T>(t)); }
    QAction *firstCheckedAction() const;
    QAction *lastCheckedAction() const;
    QAction *checkedAction() const { return firstCheckedAction(); }
};

#endif // ACTIONGROUP_HPP
