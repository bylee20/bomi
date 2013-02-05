import QtQuick 2.0

Item {
	id: root
	height: 5
	readonly property real dw: handle.width/2
	property real value: 0
	readonly property bool grabbed: handleMouse.drag.active
	signal dragging(var value)
	onValueChanged: {
		if (!handleMouse.drag.active && !handleMouse.pressed)
			handle.x = value*groove.width-dw
	}
	Rectangle {
		id: groove
		width: parent.width; height: 5; radius: 2; border { color: "#ccc"; width: 1 }
		anchors.verticalCenter: parent.verticalCenter
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#333"}
			GradientStop {position: 1.0; color: "#bbb"}
		}
		Rectangle {
			id: filled
			height: parent.height; width: handle.x + dw
			radius: parent.radius; border {width: 1; color: "#5af"}
			gradient: Gradient {
				GradientStop { position: 0.0; color: "white" }
				GradientStop { position: 1.0; color: "skyblue" }
			}
		}
		Image {
			id: handle
			x: -10/2; width: 10; height: 10
			source: handleMouse.pressed ? "handle-pressed.png" : handleMouse.containsMouse ? "handle-hovered.png" : "handle.png"
			anchors.verticalCenter: parent.verticalCenter
			onXChanged: {
				if (handleMouse.drag.active || handleMouse.pressed) {
					root.dragging((handle.x+dw)/groove.width)
				}
			}
		}
		MouseArea {
			id: handleMouse
			width: parent.width+handle.width; height: handle.height; anchors.centerIn: parent
			hoverEnabled: true
			drag.target: handle; drag.axis: Drag.XAxis
			drag.minimumX: -dw; drag.maximumX: groove.width-dw
			onPressed: {handle.x = Math.min(Math.max(drag.minimumX, mouse.x-2*dw), drag.maximumX);}
		}
	}
}
