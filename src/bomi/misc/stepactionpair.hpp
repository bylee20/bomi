#ifndef STEPACTIONPAIR_HPP
#define STEPACTIONPAIR_HPP

#include "stepaction.hpp"

class StepActionPair : public QObject {
public:
    StepActionPair(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_increase = new StepAction(ChangeValue::Increase, this);
        m_decrease = new StepAction(ChangeValue::Decrease, this);
    }
    StepAction *increase() const { return m_increase; }
    StepAction *decrease() const { return m_decrease; }
    auto setValue(const StepValue &var) -> void
        { m_increase->setValue(var); m_decrease->setValue(var); }
    auto setFormatter(std::function<QString(void)> &&var) -> void
        { m_increase->setFormatter(std::move(var));
          m_decrease->setFormatter(std::move(var)); }
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
