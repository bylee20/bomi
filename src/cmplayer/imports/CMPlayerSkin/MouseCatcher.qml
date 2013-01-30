import QtQuick 2.0
import CMPlayerSkin 1.0

MouseArea {
	id: root
	property bool catched: false
	property alias tracking: root.hoverEnabled
	hoverEnabled: true
	onPressed: { mouse.accepted = false; }
	onEntered: catched = true
	onExited: if (!pressed) catched = false
	Component.onCompleted: Util.mouseReleased.connect(check)
	function check(scenePos) {
		if (!contains(Util.mapFromSceneTo(root, scenePos)))
			catched = false
	}
}
