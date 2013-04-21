import QtQuick 2.0

Rectangle {
	id: dock
	property alias selectedIndex: view.selectedIndex
	x: d.dest;	y: 20;	height: parent.height-40;
	color: "gray"; radius: 10; opacity: 0.8;	visible: false
	onVisibleChanged: if (visible) {d.updateWidth(); sliding.start();}
	PlaylistView {
		id: view
		anchors {
			top: parent.top; bottom: parent.bottom; left: parent.left
			topMargin: 10;	 bottomMargin: 10;		leftMargin: 10
		}
	}
	Component.onCompleted: parent.widthChanged.connect(d.updateWidth)
	NumberAnimation {
		id: sliding;		target: dock;		property: "x"
		from: dock.parent.width;	to: d.dest;	easing.type: Easing.OutBack
	}
	Item {
		id: d; visible: false
		property real dest: 0
		function updateWidth() {
			if (dock.visible) {
				var w = Math.min(dock.parent.width*0.4, view.contentWidth());
				view.width = w
				dock.width = (w+20)*1.5
				dest = dock.parent.width - w - 40
			}
		}
	}
}
