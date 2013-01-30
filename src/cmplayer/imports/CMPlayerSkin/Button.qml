import QtQuick 2.0
import CMPlayerSkin 1.0

Item {
	id: item
	property alias color: box.color
	property alias icon: icon.source
	property alias smooth: icon.smooth
	property string action: ""
	property alias border: box.border
	property alias radius: box.radius
	property alias hovered: mouseArea.containsMouse
	property alias pressed: mouseArea.pressed
	property real paddings: 0
	onPaddingsChanged: icon.anchors.margins = paddings
	Rectangle {
		id: box
		anchors.fill: parent
		color: Qt.rgba(0, 0, 0, 0)
		Image {
			id: icon
			anchors.fill: parent
			smooth: true
		}
		MouseArea {
			id: mouseArea
			anchors.fill: parent
			hoverEnabled: true
			onReleased: if (containsMouse && action) Util.execute(action)
		}
	}
}
