#include "stepaction.hpp"

StepAction::StepAction(ChangeValue t, QObject *parent)
    : EnumAction<ChangeValue>(t, parent)
{

}
