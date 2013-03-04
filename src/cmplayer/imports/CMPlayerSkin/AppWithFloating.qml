import QtQuick 2.0
import CMPlayerCore 1.0 as Core
import CMPlayerSkin 1.0

Item {
	id: root
	property Player player
	property Item controls
	property string name: undefined
	MouseArea {
		id: area
		parent: player
		property real pw: -1
		property real ph: -1
		onWidthChanged: {
			floating.setCx(pw < 0 ? initCx : floating.getCx(pw))
			pw = width
		}
		onHeightChanged:  {
			floating.setCy(ph < 0 ? initCy : floating.getCy(ph))
			ph = height
		}
		Component.onCompleted: {
			controls.parent = floating
			floating.hidden = !containsMouse
		}
		anchors.fill: parent
		hoverEnabled: true; onPressed: mouse.accepted = false
		onEntered: floating.hidden = false; onExited: floating.hidden = true
		MouseArea {
			id: floating //Math.max(0, Math.min(cx*parent.width - width/2, parent.width-width)) }//
			function setCx(cx) { x = Core.Util.bound(0, cx*parent.width - width/2, parent.width - width) }
			function setCy(cy) { y = Core.Util.bound(0, cy*parent.height - height/2, parent.height - height) }
			function getCx(bg) { return (x+width/2)/bg; }
			function getCy(bg) { return (y+height/2)/bg; }

			property bool hidden: false
			width: 400; height: inner.height+24
			drag.target: floating; drag.axis: Drag.XAndYAxis
			drag.minimumX: 0; drag.maximumX: root.width-width
			drag.minimumY: 0; drag.maximumY: root.height-height
			onDoubleClicked: Util.filterDoubleClick()
			states: State {
				name: "hidden"; when: floating.hidden
				PropertyChanges { target: floating; opacity: 0.0 }
				PropertyChanges { target: floating; visible: false }
			}
			transitions: Transition {
				reversible: true; to: "hidden"
				SequentialAnimation {
					NumberAnimation { property: "opacity"; duration: 200 }
					PropertyAction { property: "visible" }
				}
			}
		}
		Connections { target: Core.Util; onCursorVisibleChanged: floating.hidden = !cursorVisible }
	}
	property real initCx: 0.5
	property real initCy: 0.0
	Component.onCompleted: {
		player.parent = root
		player.dockZ = 1
		Core.Settings.open(root.name)
		initCx = Core.Settings.getReal("cx", 0.5)
		initCy = Core.Settings.getReal("cy", 0.0)
		Core.Settings.close()
	}
	Component.onDestruction: {
		Core.Settings.open(root.name)
		Core.Settings.set("cx", floating.getCx(area.width))
		Core.Settings.set("cy", floating.getCy(area.height))
		Core.Settings.close()
	}
}
