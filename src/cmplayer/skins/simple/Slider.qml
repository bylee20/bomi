import QtQuick 2.0

Rectangle {
	id: slider
	anchors.verticalCenter: parent.verticalCenter
	property real value: 0.0
	color: "#fff"
	border.color: "#999"
	border.width: 1
	gradient: Gradient {
		GradientStop {position: 0.0; color: "#555"}
		GradientStop {position: 1.0; color: "#bbb"}
	}
	signal pressed(real target)
	signal dragged(real target)
	Rectangle {
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.bottom: parent.bottom
		anchors.margins: 1
		width: slider.value*(slider.width-2)
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#fff"}
			GradientStop {position: 1.0; color: "#ccc"}
		}
	}
	MouseArea {
		id: area
		anchors.fill: parent
		anchors.margins: 1
		onPressed: {slider.pressed(Math.min(Math.max(0.0, mouse.x/width), 1.0))}
		onPositionChanged: slider.dragged(Math.min(Math.max(0.0, mouse.x/width), 1.0))
	}
}
