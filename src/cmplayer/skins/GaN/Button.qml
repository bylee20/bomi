import QtQuick 2.0
import CMPlayer 1.0 as Cp

Cp.Button {
    property int size: 32; width: size; height: size
    property string iconName: ""
    icon: getStateIconName(iconName, hovered || (bind && bind.hovered), pressed || (bind && bind.pressed))
}
