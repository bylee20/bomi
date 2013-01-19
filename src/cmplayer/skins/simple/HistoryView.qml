import QtQuick 2.0
import CMPlayer 1.0
import "qml"

Rectangle {
	id: root
	property real nameWidth: 100
	property real latestPlayWidth: 100
	property real locationWidth: 200
	readonly property real sepWidth: 9
	property alias contentWidth: view.contentWidth
	gradient: Gradient {
		GradientStop {position: 0.0; color: "#ccc"}
		GradientStop {position: 1.0; color: "#333"}
	}
	border {width: 1; color: "black"}
	FlickableListView {
		id: view
		contentWidth: nameWidth + latestPlayWidth + locationWidth + 3*sepWidth + (sepWidth-1)*0.5
		anchors.fill: parent
		anchors.margins: parent.border.width
		Component { id: pad; Item { width: (sepWidth-1)*0.5 } }
		Component { id: itemSep; Item { width: sepWidth } }
		header: Component {
			Rectangle {
				width: view.contentWidth
				height: 15
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#ddd"}
					GradientStop {position: 1.0; color: "#aaa"}
				}
				Row {
					Loader {sourceComponent: pad; height: parent.height}
					ColumnHeader {
						text: qsTr("Name")
						height: 15; separatorWidth: sepWidth; contentWidth: nameWidth
						onContentWidthChanged: nameWidth = contentWidth
					}
					ColumnHeader {
						text: qsTr("Latest Play")
						height: 15; separatorWidth: sepWidth; contentWidth: latestPlayWidth
						onContentWidthChanged: latestPlayWidth = contentWidth
					}
					ColumnHeader {
						text: qsTr("Location")
						height: 15; separatorWidth: sepWidth; contentWidth: locationWidth
						onContentWidthChanged: locationWidth = contentWidth
					}
				}
			}
		}

		delegate: Component {
			Rectangle {
				width: view.contentWidth
				height: 15
				color: (index%2 ? "#333" :  "#555" )
				Row {
					Loader {sourceComponent: pad; height: parent.height}
					Text { color: "white"; width: nameWidth; height: 15; text: name; elide: Text.ElideRight }
					Loader {sourceComponent: itemSep; height: parent.height}
					Text { color: "white"; width: latestPlayWidth; height: 15; text: latestplay; elide: Text.ElideRight }
					Loader {sourceComponent: itemSep; height: parent.height}
					Text { color: "white"; width: locationWidth; height: 15; text: location; elide: Text.ElideRight }
				}
				MouseArea {
					anchors.fill: parent
					onClicked: {
						view.currentIndex = index
					}
					onDoubleClicked: {
						Util.doubleClicked = true
						history.play(index)
					}
				}
			}
		}
		highlight: Rectangle { width: Math.min(view.contentWidth, view.width); color: Qt.rgba(0.0, 0.0, 1.0, 0.8); radius: 5 }
		model: history
		opacity: 0.9
		Rectangle {
			id: vscrollbar
			anchors.right: view.right
			anchors.rightMargin: 3
			y: view.visibleArea.yPosition * (view.height-view.headerItem.height - 6)+view.headerItem.height+3
			width: 6
			height: view.visibleArea.heightRatio * (view.height - view.headerItem.height - 6)
			color: "white"
			radius: 3
			smooth: true
			MouseArea {
				anchors.fill: parent
				preventStealing: true
				drag.axis: Drag.YAxis
				drag.target: vscrollbar
				drag.minimumY: view.headerItem.height+3
				drag.maximumY: view.height-vscrollbar.height-3
				onPositionChanged: view.contentY = (vscrollbar.y-view.headerItem.height - 3)*view.contentHeight/(view.height-view.headerItem.height-6)
			}
		}
	}
}
//Rectangle {
//	id: frame
//	border {width: 1; color: "black"}
//	gradient: Gradient {
//		GradientStop {position: 0.0; color: "#ccc"}
//		GradientStop {position: 1.0; color: "#333"}
//	}

//	property Gradient __playingGradient : Gradient {
//		GradientStop {position: 0.0; color: "#aaa"}
//		GradientStop {position: 1.0; color: "#555"}
//	}
//	readonly property alias heightHint: view.contentHeight
//	readonly property real widthHint: 100
//	readonly property int fontSize: 10
//	readonly property string fontName: Util.monospace
//	readonly property real padding: 5
//	function contentWidth() {
//		return 300;
////		var max = 0;
////		for (var i=0; i<view.count; ++i) {
////			var number = Util.textWidth(playlist.number(i), nameFontSize, nameFontFamily);
////			var name = Util.textWidth(playlist.name(i), nameFontSize, nameFontFamily);
////			var loc = Util.textWidth(playlist.location(i), locationFontSize, locationFontFamily);
////			max = Math.max(number + name, loc);
////		}
////		return max+2*padding+border.width*2;
//	}
//	Component {
//		id: sep
//		Rectangle {
//			color: "white";
//			height: parent.height;
//			width: 5
//		}
//	}
//	Rectangle {
//		id: headerbox
//		height: 20
//		Row {
//			Text {
//				text: qsTr("Name")
//				width: 100
//			}
//			Loader { sourceComponent: sep }
//			Text {
//				text: qsTr("Name")
//				width: 100
//			}
//			Loader { sourceComponent: sep }
//			Text {
//				text: qsTr("Name")
//				width: 100
//			}
//		}
//	}
//	ListView {
//		id: view
//		clip: true
//		anchors.fill: parent
//		anchors.margins: 1
//		header: historyheader

//		Component {
//			id: delegate
//			Rectangle {
//				id: itembg
//				anchors {left: parent.left; right: parent.right}
//				width: view.width
//				height: 20
//				color: index%2 ? "#333" :  "#555"
//				Row {
//					Text {
//						font { family: fontName; pixelSize: fontSize }
//						width: 100
//						verticalAlignment: Text.AlignVCenter
//						height: 12
//						color: "white";
//						text: name;
//						elide: Text.ElideRight
//					}
//					Text {
//						font { family: fontName; pixelSize: fontSize }
//						width: 100
//						height: 12
//						verticalAlignment: Text.AlignVCenter
//						color: "white";
//						text: location;
//						elide: Text.ElideRight
//					}
//				}

//				MouseArea {
//					anchors.fill: parent
//					onClicked: view.currentIndex = index;
//					onDoubleClicked: {
//						Util.doubleClicked = true;
//						history.play(index);
//					}
//				}
//			}
//		}
//		highlight: Rectangle { width: view.width; color: Qt.rgba(0.0, 0.0, 1.0, 0.8); radius: 5 }
//		model: history
//		delegate: delegate
//	}

////	Rectangle {
////		id: scrollbar
////		anchors.right: view.right
////		y: view.visibleArea.yPosition * view.height
////		width: 10
////		height: view.visibleArea.heightRatio * view.height
////		color: "yellow"
////		onYChanged: {
////			view.contentWidth = playlist.contentWidth+10
////			console.log(view.width/(-(view.width - view.contentWidth - view.rightMargin)+view.leftMargin+view.width))
////			console.log(view.visibleArea.xPosition + "," + view.visibleArea.widthRatio)
////		}
////	}
////	Rectangle {
////		id: scrollbar2
////		anchors.bottom: view.bottom
////		x: view.visibleArea.xPosition * view.width
////		height: 10
////		width: view.visibleArea.widthRatio * view.width
////		color: "yellow"
////	}

//}
