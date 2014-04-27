#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include "stdafx.hpp"
#include "enums.hpp"

class Mrl;

class ActionGroup : public QActionGroup {
    Q_OBJECT
public:
    ActionGroup(QObject *parent = 0);
    void setChecked(const QVariant &data, bool checked);
    void trigger(double data);
    void trigger(const QVariant &data);
    template<typename T> void trigger(const T &t) { trigger(QVariant::fromValue<T>(t)); }
    QVariant data() const;
    void clear();
    QAction *find(const QVariant &data) const;
    template<typename T> QAction *find(const T &t) const { return find(QVariant::fromValue<T>(t)); }
    QAction *firstCheckedAction() const;
    QAction *lastCheckedAction() const;
    QAction *checkedAction() const { return firstCheckedAction(); }
};

template<typename T>
class EnumAction : public QAction {
public:
    using Data = typename EnumInfo<T>::Data;
    using EnumInfo = ::EnumInfo<T>;
    EnumAction(T t, QObject *parent = nullptr)
    : QAction(parent), m_enum(t), m_data(EnumInfo::data(m_enum)) {
        QAction::setData(QVariant::fromValue<T>(m_enum));
    }
    T enum_() const { return m_enum; }
    void setData(const Data &data) { m_data = data; }
    const Data &data() const { return m_data; }
    QString description() const { return EnumInfo::description(m_enum); }
private:
    T m_enum;
    Data m_data;
};

template<typename T> static inline EnumAction<T> *_NewEnumAction(T t) { return new EnumAction<T>(t); }

class StepAction : public EnumAction<ChangeValue> {
    Q_OBJECT
public:
    StepAction(ChangeValue t, QObject *parent = nullptr): EnumAction<ChangeValue>(t, parent) {}
    void setRange(int min, int def, int max) { m_min = min; m_max = max; m_default = def; }
    template<typename T> T clamp(const T &t) { return t; } // dummy to kill compile error
    int clamp(int value) { return isReset() ? m_default : qBound(m_min, value, m_max); }
    void updateStep(const QString &format, int step) {
        m_format = format;
        const int data = step*EnumInfo::data(enum_());
        setData(data);
        if (!data)
            setText(EnumInfo::description(enum_()));
        else
            setText(m_format.arg(m_textRate < 0 ? _NS(data) : _NS(data*m_textRate)));
    }
    void updateStep(const QString &reset, const QString &format, int step) {
        updateStep(format, step);
        if (!data() && !reset.isEmpty())
            setText(reset);
    }
    bool isReset() const { return enum_() == ChangeValue::Reset; }
    int default_() const { return m_default; }
    void setTextRate(qreal rate) { m_textRate = rate; }
    void setFormat(const QString &format) { m_format = format; }
    template<typename T> QString format(const T &t) const { Q_UNUSED(t); return QString(); }
    QString format(int value) const {
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
