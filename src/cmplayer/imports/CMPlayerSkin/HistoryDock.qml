import QtQuick 2.0

Rectangle {
	id: dock
	x: d.dest;	y: 20;	width: (view.width+20)*1.5;	height: parent.height-40
	color: "gray";	opacity: 0.8;	radius: 10;	visible: false
	HistoryView {
		id: view
		anchors {
			top: parent.top; bottom: parent.bottom; right: parent.right
			topMargin: 10; bottomMargin: 10; rightMargin: 10;
		}
		width: Math.min(dock.parent.width*0.4, view.contentWidth)
	}
	onVisibleChanged: if (visible) { sliding.start(); }
	NumberAnimation {
		id: sliding;		target: dock;		property: "x"
		from: -dock.width;	to: d.dest;	easing.type: Easing.OutBack
	}
	Item { id: d; visible: false
		property real dest: 20-(dock.width - view.width)
	}
}
