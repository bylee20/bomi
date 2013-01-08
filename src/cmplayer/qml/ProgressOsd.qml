import QtQuick 2.0

Osd {
	id: osd
	property real value: 0.0
	property real borderWidth: border.border.width
	Rectangle {
		id: border
		height: parent.height*0.05
		width: parent.width*0.8
		x: (parent.width - width)*0.5
		y: (parent.height - height)*0.5
		anchors.centerIn: parent
		color: Qt.rgba(0, 0, 0, 0)
		border.color: Qt.rgba(0.0, 0.0, 0.0, 0.5)
		border.width: 5
		Rectangle {
			id: filled
			x: borderWidth
			y: borderWidth
			height: parent.height - borderWidth*2
			color: Qt.rgba(1.0, 1.0, 1.0, 0.5)
			width: (parent.width-borderWidth*2)*value
		}
		Rectangle {
			anchors.fill: parent
			anchors.left: filled.right
			anchors.margins: border.border.width
			anchors.leftMargin: 0
			color: border.border.color
		}
	}
}
