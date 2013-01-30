import QtQuick 2.0
import CMPlayerSkin 1.0

Button {
	id: item
	radius: 3
	border.width: pressed ? 2 : (hovered ? 1 : 0)
	border.color: "#6ad"
	paddings: 2
//	MouseArea {
//		anchors.fill: parent
//		hoverEnabled: true
//		onEntered: parent.border.width = 1
//		onExited: parent.border.width = pressed ? 2 : 0;
//		onPressed: parent.border.width = 2;
//		onReleased: {parent.border.width = 0; if (containsMouse && action) player.execute(action)}
//	}
}
