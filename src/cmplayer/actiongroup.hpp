#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include "stdafx.hpp"
#include "enums.hpp"

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

template<class T>
class EnumAction : public QAction {
public:
    using Data = typename EnumInfo<T>::Data;
    using EnumInfo = ::EnumInfo<T>;
    EnumAction(T t, QObject *parent = nullptr)
    : QAction(parent), m_enum(t), m_data(EnumInfo::data(m_enum)) {
        QAction::setData(QVariant::fromValue<T>(m_enum));
    }
    auto enum_() const -> T { return m_enum; }
    auto setData(const Data &data) -> void { m_data = data; }
    const Data &data() const { return m_data; }
    auto description() const -> QString { return EnumInfo::description(m_enum); }
private:
    T m_enum;
    Data m_data;
};

template<class T> static inline EnumAction<T> *_NewEnumAction(T t) { return new EnumAction<T>(t); }

class StepAction : public EnumAction<ChangeValue> {
    Q_OBJECT
public:
    StepAction(ChangeValue t, QObject *parent = nullptr): EnumAction<ChangeValue>(t, parent) {}
    auto setRange(int min, int def, int max) -> void { m_min = min; m_max = max; m_default = def; }
    template<class T> auto clamp(const T &t) -> T { return t; } // dummy to kill compile error
    auto clamp(int value) -> int { return isReset() ? m_default : qBound(m_min, value, m_max); }
    auto updateStep(const QString &format, int step) -> void {
        m_format = format;
        const int data = step*EnumInfo::data(enum_());
        setData(data);
        if (!data)
            setText(EnumInfo::description(enum_()));
        else
            setText(m_format.arg(m_textRate < 0 ? _NS(data) : _NS(data*m_textRate)));
    }
    auto updateStep(const QString &reset, const QString &format, int step) -> void {
        updateStep(format, step);
        if (!data() && !reset.isEmpty())
            setText(reset);
    }
    auto isReset() const -> bool { return enum_() == ChangeValue::Reset; }
    auto default_() const -> int { return m_default; }
    auto setTextRate(qreal rate) -> void { m_textRate = rate; }
    auto setFormat(const QString &format) -> void { m_format = format; }
    template<class T> auto format(const T &t) const -> QString { Q_UNUSED(t); return QString(); }
    auto format(int value) const -> QString {
        if (m_min < 0 && 0 < m_max)
            return m_format.arg(m_textRate < 0 ? _NS(value) : _NS(value*m_textRate));
        else
            return m_format.arg(m_textRate < 0 ? _N(value) : _N(value*m_textRate));
    }
private:
    qreal m_textRate = -1.0;
    QString m_format;
    int m_min = 0, m_max = 100, m_default = 0;
};

#endif // ACTIONGROUP_HPP
