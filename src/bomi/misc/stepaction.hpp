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
    auto step() const -> int { return m_step; }
    auto isReset() const -> bool { return enum_() == ChangeValue::Reset; }
    auto default_() const -> int { return m_default; }
    auto setTextRate(qreal rate) -> void { m_textRate = rate; }
    auto setFormat(const QString &format) -> void
        { if (_Change(m_format, format)) update(); }
    auto setStep(int step) -> void
        { if (_Change(m_step, step)) update(); }
    template<class T>
    auto format(const T &t) const -> QString { Q_UNUSED(t); return QString(); }
    auto format(int value) const -> QString;
    static auto setFormat(const QList<QAction*> &actions,
                           const QString &format) -> void;
    static auto setStep(const QList<QAction*> &actions, int step) -> void;
private:
    auto toString(bool sign, int value) const -> QString;
    auto update() -> void;
    int m_step = 0;
    qreal m_textRate = 0.0;
    QString m_format;
    int m_min = 0, m_max = 100, m_default = 0;
};

inline auto StepAction::setFormat(const QList<QAction*> &actions,
                                  const QString &format) -> void
{
    for (auto action : actions)
        static_cast<StepAction*>(action)->setFormat(format);
}

inline auto StepAction::setStep(const QList<QAction*> &actions,
                                int step) -> void
{
    for (auto action : actions)
        static_cast<StepAction*>(action)->setStep(step);
}

#endif // STEPACTION_HPP
