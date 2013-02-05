import QtQuick 2.0
import CMPlayerSkin 1.0

Button {
	id: item
	radius: 3
	border.width: pressed ? 2 : (hovered ? 1 : 0)
	border.color: "#6ad"
	paddings: 2
}
