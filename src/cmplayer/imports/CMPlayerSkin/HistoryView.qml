import QtQuick 2.0
import CMPlayerCore 1.0

Rectangle {
	id: root
	property real nameWidth: 100
	property real latestPlayWidth: 100
	property real locationWidth: 200
	readonly property real sepWidth: 9
	property alias contentWidth: view.contentWidth
	property real rowHeight: 20
	gradient: Gradient {
		GradientStop {position: 0.0; color: "#ccc"}
		GradientStop {position: 1.0; color: "#333"}
	}
	border {width: 1; color: "black"}
	opacity: 0.9
	FlickableListView {
		id: view
		model: history
		contentWidth: nameWidth + latestPlayWidth + locationWidth + 3*sepWidth + (sepWidth-1)*0.5
		anchors { fill: parent; margins: parent.border.width }
        highlight: Rectangle { width: view.contentWidth; color: Qt.rgba(0.0, 0.0, 1.0, 0.8); radius: 5 }
		Component { id: pad; Item { width: (sepWidth-1)*0.5 } }
		Component { id: itemSep; Item { width: sepWidth } }
		header: Component {
			Rectangle {
				width: view.contentWidth; height: rowHeight
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#ddd"}
					GradientStop {position: 0.9; color: "#aaa"}
					GradientStop {position: 1.0; color: "#333"}
				}
				Row {
					Loader {sourceComponent: pad; height: parent.height}
					ColumnHeader {
						text: qsTr("Name")
						height: rowHeight; separatorWidth: sepWidth; contentWidth: nameWidth
						onContentWidthChanged: nameWidth = contentWidth
					}
					ColumnHeader {
						text: qsTr("Latest Play")
						height: rowHeight; separatorWidth: sepWidth; contentWidth: latestPlayWidth
						onContentWidthChanged: latestPlayWidth = contentWidth
					}
					ColumnHeader {
						text: qsTr("Location")
						height: rowHeight; separatorWidth: sepWidth; contentWidth: locationWidth
						onContentWidthChanged: locationWidth = contentWidth
					}
				}
			}
		}

		delegate: Component {
			Rectangle {
				width: view.contentWidth; height: rowHeight
				color: (index%2 ? "#333" :  "#555" )
				Row {
					Loader {sourceComponent: pad; height: parent.height}
					Text {
						color: "white"; width: nameWidth; height: rowHeight; text: name
						elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
					}
					Loader {sourceComponent: itemSep; height: parent.height}
					Text {
						color: "white"; width: latestPlayWidth; height: rowHeight; text: latestplay
						elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
					}
					Loader {sourceComponent: itemSep; height: parent.height}
					Text {
						color: "white"; width: locationWidth; height: rowHeight; text: location
						elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
					}
				}
				MouseArea {
					anchors.fill: parent
					onClicked: { view.currentIndex = index }
					onDoubleClicked: { Util.filterDoubleClick(); history.play(index) }
				}
			}
		}
		ScrollBar { flickable: view; anchors.topMargin: view.headerItem.height+3 }
	}
}
