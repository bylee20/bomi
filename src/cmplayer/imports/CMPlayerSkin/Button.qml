import QtQuick 2.0
import CMPlayerCore 1.0 as Core

Item {
	id: item
	property alias color: box.color
	property alias icon: icon.source
	property alias smooth: icon.smooth
	property string action: ""
	property bool checked: false
	property alias border: box.border
	property alias radius: box.radius
	property alias hovered: mouseArea.containsMouse
	property alias pressed: mouseArea.pressed
	property real paddings: 0
	property alias gradient: box.gradient
	signal clicked
	onPaddingsChanged: icon.anchors.margins = paddings
	function getStateIconName(prefix) {
		if (checked)
			prefix += "-checked";
		prefix += hovered ? (pressed ? "-pressed.png" : "-hovered.png") : ".png"
		return prefix;
	}
	Rectangle {
		id: box
		anchors.fill: parent
		color: "transparent"
		Image {
			id: icon
			anchors.fill: parent
			smooth: true
		}
		MouseArea {
			id: mouseArea
			anchors.fill: parent
			hoverEnabled: true
			onReleased: if (containsMouse && action) Core.Util.execute(action)
			onClicked: item.clicked()
		}
	}
}
