#include "stepaction.hpp"

StepAction::StepAction(ChangeValue t, QObject *parent)
    : EnumAction<ChangeValue>(t, parent)
{

}

auto StepAction::toString(bool sign, int value) const -> QString
{
    if (m_textRate == 0.0)
        return sign ? _NS(value) : _N(value);
    return sign ? _NS(value*m_textRate) : _N(value*m_textRate);
}

auto StepAction::update() -> void
{
    const int data = m_step*EnumInfo::data(enum_());
    setData(data);
    if (!data)
        setText(EnumInfo::description(enum_()));
    else
        setText(m_format.arg(toString(true, data)));
}

auto StepAction::format(int value) const -> QString
{
    return m_format.arg(toString(m_min < 0 && 0 < m_max, value));
}
