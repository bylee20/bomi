import QtQuick 2.0
import CMPlayerCore 1.0
import CMPlayerSkin 1.0

Item {
	id: root
	property Engine engine
	property Item controls
	property string name: undefined
	MouseArea {
		id: area
		parent: engine
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
x		}
		anchors.fill: parent
		hoverEnabled: true;
		onPressed: Util.trigger(Util.MousePress)
		onDoubleClicked: Util.trigger(Util.MouseDoubleClick)
		onWheel: Util.trigger(Util.Wheel)
		onEntered: floating.hidden = false;
		onExited: floating.hidden = true
		MouseArea {
			id: floating //Math.max(0, Math.min(cx*parent.width - width/2, parent.width-width)) }//
			function setCx(cx) { x = Util.bound(0, cx*parent.width - width/2, parent.width - width) }
			function setCy(cy) { y = Util.bound(0, cy*parent.height - height/2, parent.height - height) }
			function getCx(bg) { return (x+width/2)/bg; }
			function getCy(bg) { return (y+height/2)/bg; }

			property bool hidden: false
			width: controls.width; height: controls.height
			drag.target: floating; drag.axis: Drag.XAndYAxis
			drag.minimumX: 0; drag.maximumX: root.width-width
			drag.minimumY: 0; drag.maximumY: root.height-height
			onDoubleClicked: Util.trigger(Util.MouseDoubleClick)
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
		Connections { target: Util; onCursorVisibleChanged: floating.hidden = !cursorVisible }
	}
	property real initCx: 0.5
	property real initCy: 0.0
	Component.onCompleted: {
		engine.parent = root
		engine.dockZ = 1
		Settings.open(root.name)
		initCx = Settings.getReal("cx", 0.5)
		initCy = Settings.getReal("cy", 0.0)
		Settings.close()
	}
	Component.onDestruction: {
		Settings.open(root.name)
		Settings.set("cx", floating.getCx(area.width))
		Settings.set("cy", floating.getCy(area.height))
		Settings.close()
	}
}
