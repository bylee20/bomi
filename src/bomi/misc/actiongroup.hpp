#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

class ActionGroup : public QActionGroup {
    Q_OBJECT
public:
    ActionGroup(QObject *parent = nullptr);
    auto setChecked(const QVariant &data, bool checked) -> void;
    auto trigger(double data) -> void;
    auto trigger(const QVariant &data) -> void;
    template<class T>
    auto trigger(const T &t) -> void { trigger(QVariant::fromValue<T>(t)); }
    auto data() const -> QVariant;
    auto clear() -> void;
    auto find(const QVariant &data) const -> QAction*;
    template<class T>
    auto find(const T &t) const -> QAction*
        { return find(QVariant::fromValue<T>(t)); }
    auto firstCheckedAction() const -> QAction*;
    auto lastCheckedAction() const -> QAction*;
    auto checkedAction() const -> QAction* { return firstCheckedAction(); }
};

#endif // ACTIONGROUP_HPP
