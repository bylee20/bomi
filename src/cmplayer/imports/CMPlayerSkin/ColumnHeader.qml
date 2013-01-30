import QtQuick 2.0
import CMPlayerSkin 1.0

Item {
	id: root
	property alias text: caption.text
	property alias contentWidth: separator.x
	property alias separatorWidth: separator.width
	property real contentWidthHint: -1
	width: contentWidth + separatorWidth
	height: parent.height
	Text {
		id: caption
		height: root.height
		elide: Text.ElideRight
		anchors {left: parent.left; right: separator.left}
		verticalAlignment: Text.AlignVCenter
	}
	Item {
		id: separator
		height: root.height
		x: contentWidth
		Rectangle {
			height: parent.height*0.6
			anchors.centerIn: parent
			width: 1
			color: "gray"
		}
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			preventStealing: true
			drag.target: parent
			drag.axis: Drag.XAxis
			drag.minimumX: 0
			cursorShape: pressed || containsMouse ? Qt.SplitHCursor : Qt.ArrowCursor;
			onDoubleClicked: {
				if (contentWidthHint > 0) {
					contentWidth = contentWidthHint
					Util.filterDoubleClick()
				}
			}
		}
	}
}
