import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin

Skin.Button {
	property int size: 32; width: size; height: size
	property string iconName: ""
	icon: getStateIconName(iconName, hovered || (bind && bind.hovered), pressed || (bind && bind.pressed))
}
