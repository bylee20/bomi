import QtQuick 2.0
import CMPlayerCore 1.0
import CMPlayerSkin 1.0

Item {
	id: root
	property Engine engine: Player {}
	property Item controls: Item {}
	Binding { target: engine; property: "width"; value: root.width }
	Binding { target: engine; property: "height"; value: Util.fullScreen ? root.height : root.height - controls.height }
	Connections {
		target: Util
		onFullScreenChanged: {
			catcher.update()
			if (Util.fullScreen)
				engine.bottomPadding = catcher.height
			else
				engine.bottomPadding = 0
		}
	}

	MouseArea {
		anchors.fill: parent
		acceptedButtons: Qt.AllButtons
		onPressed: Util.trigger(Util.MousePress)
		onWheel: Util.trigger(Util.Wheel)
		onDoubleClicked: Util.trigger(Util.MouseDoubleClick)
	}

	MouseArea {
		id: catcher; z: engine.z+1;
		width: parent.width
		height: controls.height
		anchors.bottom: parent.bottom
		hoverEnabled: true
		function update() {
			if (Util.fullScreen && !catcher.containsMouse)
				sliding.start()
			else
				controls.y = 0
		}
		onContainsMouseChanged: update()
		NumberAnimation { id: sliding; target: controls; property: "y"; duration: 200; from: 0; to: controls.height }
	}
	Component.onCompleted: {engine.parent = root; controls.parent = catcher}
}
