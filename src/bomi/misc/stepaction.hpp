#ifndef STEPACTION_HPP
#define STEPACTION_HPP

#include "enumaction.hpp"
#include "enum/changevalue.hpp"
#include "stepinfo.hpp"

class StepAction : public EnumAction<ChangeValue> {
    Q_OBJECT
public:
    StepAction(ChangeValue t, QObject *parent = nullptr);
    auto isReset() const -> bool { return enum_() == ChangeValue::Reset; }
    auto setValue(const StepValue &value) -> void
        { if (_Change(m_value, value)) retranslate(); }
    auto value() const -> const StepValue& { return m_value; }
    auto setFormatter(std::function<QString(void)> &&formatter) -> void
        { m_formatter = formatter; }
    auto retranslate() -> void;
private:
    StepValue m_value;
    std::function<QString(void)> m_formatter;
};

#endif // STEPACTION_HPP
