import QtQuick 2.0
import CMPlayer 1.0 as Core

Item {
	id: item
	property var bind: undefined
	property alias color: box.color
	property alias icon: icon.source
	property alias smooth: icon.smooth
	property string action: ""
	property string action2: ""
	property var _action: Core.Util.action(action)
	property var _action2: Core.Util.action(action2)
	property string tooltip: __makeToolTip(_action, _action2)
	property alias text: _text.text
	property alias textElide: _text.elide
	property alias textColor: _text.color
	property alias textAlignmentV: _text.verticalAlignment
	property alias textAlignmentH: _text.horizontalAlignment
	property alias textWidth: _text.contentWidth
	property alias textHeight: _text.contentHeight
	property alias font: _text.font
	property alias tooltipDelay: tooltipTimer.interval
	property bool checked: false
	property alias border: box.border
	property alias radius: box.radius
	property alias hovered: mouseArea.containsMouse
	property alias pressed: mouseArea.pressed
	property real paddings: 0
	property alias gradient: box.gradient
	signal clicked
	onPaddingsChanged: icon.anchors.margins = paddings
	function makeToolTip(left, right) {
		return qsTr("Left click: %1\nRight click: %2").arg(left).arg(right)
	}
	function __makeToolTip(action, action2) {
		if (action && action2)
			return makeToolTip(action.text, action2.text)
		else
			return action ? action.text : (action2 ? action2.text : "")
	}
	function getStateIconName(prefix, hovered, pressed) {
		if (!prefix || !prefix.length)
			return ""
		if (checked)
			prefix += "-checked";
		prefix += pressed ? "-pressed.png" : ( hovered ? "-hovered.png" : ".png")
		return prefix;
	}
	Rectangle {
		id: box; anchors.fill: parent; color: "transparent"
		Text { id: _text; anchors.fill: parent }
		Image { id: icon; anchors.fill: parent; smooth: true }
		MouseArea {
			id: mouseArea; anchors.fill: parent; hoverEnabled: true
			acceptedButtons: Qt.LeftButton | (item._action2 ? Qt.RightButton : 0)
			onReleased: {
				var action = mouse.button & Qt.RightButton ? item.action2 : item.action
				if (containsMouse && action.length) Core.Util.execute(action)
			}
			onClicked: item.clicked(); onExited: Core.Util.hideToolTip(); onCanceled: Core.Util.hideToolTip()
			Timer {
				id: tooltipTimer; interval: 1000
				running: parent.containsMouse && !pressed && tooltip.length
				onTriggered: Core.Util.showToolTip(parent, Qt.point(parent.mouseX, parent.mouseY), tooltip)
			}
		}
	}
}
