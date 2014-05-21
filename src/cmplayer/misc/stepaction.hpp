#ifndef STEPACTION_HPP
#define STEPACTION_HPP

#include "enumaction.hpp"
#include "enum/changevalue.hpp"

class StepAction : public EnumAction<ChangeValue> {
    Q_OBJECT
public:
    StepAction(ChangeValue t, QObject *parent = nullptr);
    auto setRange(int min, int def, int max) -> void
        { m_min = min; m_max = max; m_default = def; }
    template<class T>
    auto clamp(const T &t) -> T { return t; } // dummy to kill compile error
    auto clamp(int value) -> int
        { return isReset() ? m_default : qBound(m_min, value, m_max); }
    auto updateStep(const QString &format, int step) -> void;
    auto updateStep(const QString &reset, const QString &format,
                    int step) -> void;
    auto isReset() const -> bool { return enum_() == ChangeValue::Reset; }
    auto default_() const -> int { return m_default; }
    auto setTextRate(qreal rate) -> void { m_textRate = rate; }
    auto setFormat(const QString &format) -> void { m_format = format; }
    template<class T>
    auto format(const T &t) const -> QString { Q_UNUSED(t); return QString(); }
    auto format(int value) const -> QString;
private:
    qreal m_textRate = -1.0;
    QString m_format;
    int m_min = 0, m_max = 100, m_default = 0;
};

#endif // STEPACTION_HPP
