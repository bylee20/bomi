import QtQuick 2.0
import CMPlayer 1.0
import "qml"

Rectangle {
	id: frame
	border {width: 1; color: "black"}
//	color: "transparent"
	gradient: Gradient {
		GradientStop {position: 0.0; color: "#ccc"}
		GradientStop {position: 1.0; color: "#333"}
	}

	property Gradient __loadedGradient : Gradient {
		GradientStop {position: 0.0; color: "#aaa"}
		GradientStop {position: 1.0; color: "#555"}
	}
	readonly property alias heightHint: view.contentHeight
	readonly property real widthHint: 100
	readonly property int nameFontSize: 15
	readonly property int locationFontSize: 8
	readonly property string nameFontFamily: Util.monospace
	readonly property string locationFontFamily: Util.monospace
	readonly property real padding: 5
	function contentWidth() {
		var max = 0;
		for (var i=0; i<view.count; ++i) {
			var number = Util.textWidth(playlist.number(i), nameFontSize, nameFontFamily);
			var name = Util.textWidth(playlist.name(i), nameFontSize, nameFontFamily);
			var loc = Util.textWidth(playlist.location(i), locationFontSize, locationFontFamily);
			max = Math.max(number + name, loc);
		}
		return max+2*padding+border.width*2;
	}

	ListView {
		id: view
		clip: true
		anchors.fill: parent
		anchors.margins: 1
		Component {
			id: delegate
			Rectangle {
				id: itembg
				anchors {left: parent.left; right: parent.right}
				width: view.width
				height: 40
				color: isLoaded ? "white" : (index%2 ? "#333" :  "#555" )
				gradient: isLoaded ? __loadedGradient : undefined
				Column {
					Text {
						font { family: nameFontFamily; pixelSize: nameFontSize }
						x: padding
						width: view.width-2*padding
						verticalAlignment: Text.AlignVCenter
						height: 25
						color: "white";
						text: name;
						elide: Text.ElideRight
					}
					Text {
						font { family: locationFontFamily; pixelSize: locationFontSize }
						x: padding
						width: view.width-2*padding
						height: locationFontSize+padding
						verticalAlignment: Text.AlignTop
						color: "white";
						text: location;
						elide: Text.ElideRight
					}
				}

				MouseArea {
					anchors.fill: parent
					onClicked: view.currentIndex = index;
					onDoubleClicked: {
						Util.doubleClicked = true;
						playlist.play(index);
					}
				}
			}
		}
		highlight: Rectangle { width: view.width; color: Qt.rgba(0.0, 0.0, 1.0, 0.8); radius: 5 }
		model: playlist
		delegate: delegate
	}

	Rectangle {
		id: vscrollbar
		anchors.right: view.right
		anchors.rightMargin: 3
		y: view.visibleArea.yPosition * (view.height - 6)+3
		width: 6
		height: view.visibleArea.heightRatio * (view.height - 6)
		color: "white"
		radius: 3
		smooth: true
		visible: view.height < view.contentHeight
		MouseArea {
			anchors.fill: parent
			preventStealing: true
			drag.axis: Drag.YAxis
			drag.target: vscrollbar
			drag.minimumY: 3
			drag.maximumY: view.height-vscrollbar.height-3
			onPositionChanged: view.contentY = (vscrollbar.y-3)*view.contentHeight/(view.height-6)
		}
	}
//	Rectangle {
//		id: scrollbar
//		anchors.right: view.right
//		y: view.visibleArea.yPosition * view.height
//		width: 10
//		height: view.visibleArea.heightRatio * view.height
//		color: "yellow"
//		onYChanged: {
//			view.contentWidth = playlist.contentWidth+10
//			console.log(view.width/(-(view.width - view.contentWidth - view.rightMargin)+view.leftMargin+view.width))
//			console.log(view.visibleArea.xPosition + "," + view.visibleArea.widthRatio)
//		}
//	}
//	Rectangle {
//		id: scrollbar2
//		anchors.bottom: view.bottom
//		x: view.visibleArea.xPosition * view.width
//		height: 10
//		width: view.visibleArea.widthRatio * view.width
//		color: "yellow"
//	}
//	Rectangle {
//		id: scrollbar
//		anchors.right: view.right
//		y: view.visibleArea.yPosition * view.height
//		width: 10
//		height: view.visibleArea.heightRatio * view.height
//		color: "black"
//	}
}
