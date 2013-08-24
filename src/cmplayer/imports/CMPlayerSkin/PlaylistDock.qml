import QtQuick 2.0
import CMPlayerCore 1.0 as Core
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
	id: dock
	property alias selectedIndex: table.currentRow
	readonly property real widthHint: column.width + table.margins*2 + table.scrollArea
	property real dest: 0
	property bool show: false
	x: parent.width; y: 20; width: widthHint; height: parent.height-2*y; visible: false
	Rectangle { id: rect; anchors.fill: parent; color: "gray"; opacity: 0.8; radius: 5 }
	states: State {
		name: "show"; when: dock.show
		PropertyChanges { target: dock; visible: true }
		PropertyChanges { target: dock; x: dock.dest; explicit: true }
	}
	transitions: Transition {
		reversible: true; to: "show"
		SequentialAnimation {
			PropertyAction { property: "visible" }
			NumberAnimation { property: "x" }
		}
	}

	function updateDestination() { dock.dest = dock.parent.width-dock.width+rect.radius }
	Component.onCompleted: updateDestination()
	Connections { target: parent; onWidthChanged: { updateDestination() } }
	onWidthChanged: { updateDestination() }
	onDestChanged: { if (dock.show) dock.x = dock.dest }

	Rectangle {
		id: frame
		x: table.x-1; y: table.y-1
		width: table.width+2
		height: table.height+2
		border { color: "black"; width: 1 }
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#ccc"}
			GradientStop {position: 1.0; color: "#333"}
		}
	}

	TableView {
		id: table

		model: playlist
		headerVisible: false

		backgroundVisible: false

		readonly property real scrollArea: 10
		readonly property real margins: 15
		x: margins; y: margins;
		anchors { fill: parent; margins: table.margins }

		readonly property int nameFontSize: 15
		readonly property int locationFontSize: 10
		readonly property string nameFontFamily: Core.Util.monospace
		readonly property string locationFontFamily: Core.Util.monospace

		function contentWidth() {
			var max = 0;
			for (var i=0; i<table.rowCount; ++i) {
				var number = 0;//Core.Util.textWidth(playlist.number(i), table.nameFontSize, table.nameFontFamily);
				var name = Core.Util.textWidth(playlist.name(i), table.nameFontSize, table.nameFontFamily);
				var loc = Core.Util.textWidth(playlist.location(i), table.locationFontSize, table.locationFontFamily);
				max = Math.max(number + name, loc, max);
			}
			return max+15
		}

		onRowCountChanged:column.width = contentWidth()
		property Gradient __loadedGradient : Gradient {
			GradientStop {position: 0.0; color: "#5af"}
			GradientStop {position: 1.0; color: "#8cf"}
		}
		TableViewColumn { role: "name"; width: 0; id: column }
		rowDelegate: Rectangle {
			property bool loaded: playlist.loaded === styleData.row
			height: 40
			color: (0 <= styleData.row && styleData.row < table.rowCount)
				? styleData.selected ? Qt.rgba(0.4, 0.6, 1.0, 1.0) : (loaded ? "white" : (styleData.alternate ? "#333" :  "#555" ))
				: "transparent"
			gradient: !styleData.selected && loaded ? table.__loadedGradient : undefined
		}

		itemDelegate: Item {
			Column {
				width: parent.width
				Text {
					anchors.margins: 5
					anchors.left: parent.left
					anchors.right: parent.right
					font { family: table.nameFontFamily; pixelSize: table.nameFontSize }
					font.italic: playlist.loaded === styleData.row
					font.bold: playlist.loaded === styleData.row

					verticalAlignment: Text.AlignVCenter
					height: 25
					color: playlist.loaded === styleData.row ? "black" : "white";
					text: styleData.value;
					elide: Text.ElideRight
				}
				Text {
					anchors.margins: 5
					anchors.left: parent.left
					anchors.right: parent.right
					font { family: table.locationFontFamily; pixelSize: table.locationFontSize }
					width: parent.width
					height: table.locationFontSize
					verticalAlignment: Text.AlignTop
					color: playlist.loaded == styleData.row ? "black" : "white";
					text: playlist.location(styleData.row)
					elide: Text.ElideRight
				}
			}
		}
		onDoubleClicked: playlist.play(row)
		frameVisible: false
		style: TableViewStyle {
			backgroundColor: "#555"
			alternateBackgroundColor: "#333"
			decrementControl: Item {} incrementControl: Item {} corner: Item {}
			scrollBarBackground: Rectangle {
				color: "#ddd"
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				x: styleData.horizontal ? -1 : 1
				y: styleData.horizontal ? 1 : 0
				Rectangle {
					visible: styleData.horizontal
					color: parent.color
					x: parent.width
					width: 2
					height: table.scrollArea
				}
			}
			handle: Item {
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				Rectangle {
					anchors {
						margins: 1
						fill: parent;
						leftMargin: styleData.horizontal ? 1 : 3
						topMargin: styleData.horizontal ? 3 : 1
					}
					opacity: 0.8; radius: 3; smooth: true; color: "black"
				}
			}
		}
	}

	MouseArea {
		anchors.fill: parent
		acceptedButtons: Qt.RightButton
		onClicked: Core.Util.execute("tool/playlist")
	}
}
