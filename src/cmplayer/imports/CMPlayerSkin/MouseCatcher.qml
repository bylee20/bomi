import QtQuick 2.0
import CMPlayerCore 1.0 as Core

MouseArea {
	id: root
	property bool catched: false
	property alias tracking: root.hoverEnabled
	hoverEnabled: true
	onPressed: { mouse.accepted = false; }
	onEntered: catched = true
	onExited: if (!pressed) catched = false
	Component.onCompleted: Core.Util.mouseReleased.connect(check)
	function check(scenePos) {
		if (!contains(Core.Util.mapFromSceneTo(root, scenePos)))
			catched = false
	}
}
