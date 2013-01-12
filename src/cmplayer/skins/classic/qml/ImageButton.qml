import QtQuick 2.0

Item {
	id: item
	property alias source: image.source
	property alias smooth: image.smooth
	property string action: ""
	Rectangle {
		id: bg
		radius: 3
		anchors.fill: parent
		border.width: 0
		border.color: "#6ad"
		color: Qt.rgba(0, 0, 0, 0)
		Image {
			id: image
			anchors.fill: parent
			anchors.margins: 2
			smooth: true
		}
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			onEntered: parent.border.width = 1
			onExited: parent.border.width = pressed ? 2 : 0;
			onPressed: parent.border.width = 2;
			onReleased: {parent.border.width = 0; if (containsMouse && action) player.execute(action)}
		}
	}
}
