import QtQuick 2.0

Rectangle {
	property var flickable
	readonly property real targetHeight: (flickable.height - anchors.topMargin -anchors.bottomMargin)
	id: vscrollbar
	anchors { right: flickable.right; rightMargin: 3; topMargin: 3; bottomMargin: 3 }
	y: flickable.visibleArea.yPosition * targetHeight + anchors.topMargin
	width: 6
	height: flickable.visibleArea.heightRatio * targetHeight
	color: "white"
	radius: 3
	smooth: true
	visible: flickable.height < flickable.contentHeight
	MouseArea {
		anchors.fill: parent
		preventStealing: true
		drag.axis: Drag.YAxis
		drag.target: vscrollbar
		drag.minimumY: vscrollbar.anchors.topMargin
		drag.maximumY: flickable.height-vscrollbar.height-vscrollbar.anchors.bottomMargin
		onPositionChanged: flickable.contentY = (vscrollbar.y-vscrollbar.anchors.topMargin)*flickable.contentHeight/targetHeight
	}
	opacity: 0.75


//	// The flickable to which the scrollbar is attached to, must be set
//	property variant flickable

//	// True for vertical ScrollBar, false for horizontal
//	property bool vertical: true

//	// If set to false, scrollbar is visible even when not scrolling
//	property bool hideScrollBarsWhenStopped: true

//	// Thickness of the scrollbar, in pixels
//	property int scrollbarWidth: 7

//	color: "black"
//	radius: vertical ? width/2 : height/2

//	function sbOpacity()
//	{
//		if (!hideScrollBarsWhenStopped) {
//			return 0.5;
//		}

//		return (flickable.flicking || flickable.moving) ? (vertical ? (height >= parent.height ? 0 : 0.5) : (width >= parent.width ? 0 : 0.5)) : 0;
//	}

//	// Scrollbar appears automatically when content is bigger than the Flickable
//	opacity: sbOpacity()

//	// Calculate width/height and position based on the content size and position of
//	// the Flickable
//	width: vertical ? scrollbarWidth : flickable.visibleArea.widthRatio * parent.width
//	height: vertical ? flickable.visibleArea.heightRatio * parent.height : scrollbarWidth
//	x: vertical ? parent.width - width : flickable.visibleArea.xPosition * parent.width
//	y: vertical ? flickable.visibleArea.yPosition * parent.height : parent.height - height

//	// Animate scrollbar appearing/disappearing
//	Behavior on opacity { NumberAnimation { duration: 200 }}
}
