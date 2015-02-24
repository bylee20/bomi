#include "stepaction.hpp"

StepAction::StepAction(ChangeValue t, QObject *parent)
    : EnumAction<ChangeValue>(t, parent)
{

}

auto StepAction::retranslate() -> void
{
    const auto text = m_value.text(enum_());
    setText(m_formatter ? m_formatter().arg(text) : text);
}
