import QtQuick 2.0

Item {
	id: root
	property real value: 0
	readonly property bool grabbed: handleMouse.drag.active
	signal dragging(var value)
	onValueChanged: {
		if (!handleMouse.drag.active && !handleMouse.pressed)
			handle.x = value*groove.width-handleMouse.dw
				 + (groove.anchors.rightMargin - filled.anchors.rightMargin)
				 + (groove.anchors.leftMargin - filled.anchors.leftMargin)
	}
	Component.onCompleted: {
		groove.parent = root
		filled.parent = handle.parent = groove
		filled.z = groove.z + 1
		handle.z = filled.z + 1
		handle.x = value*groove.width-handleMouse.dw
	}
	property Item handle: Rectangle { width: 1; height: 1; color: "transparent" }
	property Item groove: Rectangle { anchors.fill: parent; color: "black" }
	property Item filled: Rectangle { anchors.fill: parent; color: "white" }
	property alias pressed: handleMouse.pressed
	property alias hovered: handleMouse.containsMouse
	MouseArea {
		readonly property real dw: handle.width/2
		id: handleMouse; parent: groove
		width: parent.width+handle.width; height: Math.max(groove.height, handle.height); anchors.centerIn: parent
		hoverEnabled: true
		drag.target: handle; drag.axis: Drag.XAxis
		drag.minimumX: -dw; drag.maximumX: groove.width-dw + (groove.anchors.rightMargin - filled.anchors.rightMargin) + (groove.anchors.leftMargin - filled.anchors.leftMargin)
		onPressed: {handle.x = Math.min(Math.max(drag.minimumX, mouse.x-2*handleMouse.dw), drag.maximumX);}
	}

	Connections {
		target: handle
		onXChanged: {
			if (handleMouse.drag.active || handleMouse.pressed) {
				root.dragging((handle.x+handleMouse.dw)/groove.width)
			}
		}
	}
	Binding {
		target: groove; property: "width"; value: groove.parent.width
	}
	Binding {
		target: filled; property: "width"; value: handle.x+handleMouse.dw
	}
}
