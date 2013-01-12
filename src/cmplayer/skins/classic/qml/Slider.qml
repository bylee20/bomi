import QtQuick 2.0

Item {
	id: top
	property real value: 0.0
	signal pressed(real target)
	signal dragged(real target)
	implicitHeight: handle.height
	MouseArea {
		id: barMouse
		width: slider.width
		height: handle.height
		anchors.centerIn: parent
		onPressed: {top.pressed(Math.min(Math.max(0.0, mouse.x/width), 1.0))}
		onPositionChanged: top.dragged(Math.min(Math.max(0.0, mouse.x/width), 1.0))
	}
	Rectangle {
		id: slider
		anchors.centerIn: parent
		width: parent.width
		border { color: "#999"; width: 1 }
		height: 5
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#333"}
			GradientStop {position: 1.0; color: "#bbb"}
		}
		Rectangle {
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.bottom: parent.bottom
			width: top.value*(parent.width-2)
			height: parent.height
			border { color: "#6ad"; width: 1 }
			gradient: Gradient {
				GradientStop {position: 0.0; color: "#fff"}
				GradientStop {position: 1.0; color: "#ccc"}
			}
		}

		Rectangle {
			id: handle
			width: 9
			height: 9
			anchors.verticalCenter: parent.verticalCenter
			x: Math.min(Math.max(0.0, top.value*parent.width-width/2), parent.width-width)
			border.width: __pressed ? 2 : 1
			border.color: __pressed || __hovered ? "#6ad" : "#5c5c5c"

			radius: 2
			property Gradient __dark: Gradient {
				GradientStop { position: 0.0; color: "#aaa"}
				GradientStop { position: 1.0; color: "#999"}
			}
			property Gradient __bright: Gradient {
				GradientStop { position: 0.0; color: "#fff"}
				GradientStop { position: 1.0; color: "#ccc"}
			}
			property bool __pressed: barMouse.pressed || handleMouse.pressed
			property bool __hovered: barMouse.containsMouse || handleMouse.containsMouse
			gradient: __pressed || __hovered ? __bright : __dark
			MouseArea {
				id: handleMouse
				anchors.fill: parent
				hoverEnabled: true
				onPressed: mouse.accepted = false
			}
		}
	}
}
