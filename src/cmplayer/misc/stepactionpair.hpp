#ifndef STEPACTIONPAIR_HPP
#define STEPACTIONPAIR_HPP

#include "stepaction.hpp"

class StepActionPair : public QObject {
    Q_OBJECT
public:
    StepActionPair(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_increase = new StepAction(ChangeValue::Increase, this);
        m_decrease = new StepAction(ChangeValue::Decrease, this);
    }
    StepAction *increase() const { return m_increase; }
    StepAction *decrease() const { return m_decrease; }
    auto setRange(int min, int def, int max) -> void
    {
        m_increase->setRange(min, def, max);
        m_decrease->setRange(min, def, max);
    }
#define DEC_SET(f, type) \
    auto f(type var) -> void { m_increase->f(var); m_decrease->f(var); }
    DEC_SET(setTextRate, qreal)
    DEC_SET(setFormat, const QString&)
    DEC_SET(setStep, int)
#undef DEC_SET
private:
    template<class... Args>
    auto apply(void(StepAction::*set)(Args...), Args... t) -> void
    {
        (m_increase->*set)(t...);
        (m_decrease->*set)(t...);
    }
    StepAction *m_increase = nullptr;
    StepAction *m_decrease = nullptr;
};

#endif // STEPACTIONPAIR_HPP
