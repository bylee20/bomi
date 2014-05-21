#include "stepaction.hpp"

StepAction::StepAction(ChangeValue t, QObject *parent)
    : EnumAction<ChangeValue>(t, parent)
{

}

auto StepAction::updateStep(const QString &format, int step) -> void
{
    m_format = format;
    const int data = step*EnumInfo::data(enum_());
    setData(data);
    if (!data)
        setText(EnumInfo::description(enum_()));
    else
        setText(m_format.arg(m_textRate < 0 ? _NS(data) : _NS(data*m_textRate)));
}

auto StepAction::updateStep(const QString &reset, const QString &format,
                            int step) -> void {
    updateStep(format, step);
    if (!data() && !reset.isEmpty())
        setText(reset);
}

auto StepAction::format(int value) const -> QString
{
    if (m_min < 0 && 0 < m_max)
        return m_format.arg(m_textRate < 0 ? _NS(value) : _NS(value*m_textRate));
    else
        return m_format.arg(m_textRate < 0 ? _N(value) : _N(value*m_textRate));
}
