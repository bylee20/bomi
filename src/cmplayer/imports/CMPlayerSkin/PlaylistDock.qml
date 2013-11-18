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
	onVisibleChanged: console.log(visible)
	function updateDestination() { dock.dest = dock.parent.width-dock.width }
	Component.onCompleted: updateDestination()
	Connections { target: parent; onWidthChanged: { updateDestination() } }
	onWidthChanged: { updateDestination() }
	onDestChanged: { if (dock.show) dock.x = dock.dest }

	Rectangle {
		id: frame
		x: table.x-1; y: table.y-1
		width: table.width+2
		height: table.height+2
		border { color: "white"; width: 1 }
		color: Qt.rgba(0, 0, 0, 0.4)
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
		TableViewColumn { role: "name"; width: 0; id: column }

		onDoubleClicked: playlist.play(row)
		frameVisible: false
		style: TableViewStyle {
			backgroundColor: "#555"
			alternateBackgroundColor: "#333"
			decrementControl: Item {} incrementControl: Item {} corner: Item {}
			scrollBarBackground: Item {
				implicitWidth:  table.scrollArea
				implicitHeight: table.scrollArea
				x: styleData.horizontal ? -2 :  2
				y: styleData.horizontal ?  2 : -2
				Rectangle {
					border { color: "white"; width: 1 } color: Qt.rgba(0, 0, 0, 0)
					x: styleData.horizontal ?  0 : -2
					y: styleData.horizontal ? -2 :  0
					width:  parent.width  + (styleData.horizontal ?  3 : 2)
					height: parent.height + (styleData.horizontal ?  2 : 3)
				}
			}
			handle: Item {
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				Rectangle {
					anchors {
						fill: parent
						leftMargin: styleData.horizontal ? 1 : 3
						rightMargin: styleData.horizontal ? 3 : 1
						topMargin: styleData.horizontal ? 3 : 1
						bottomMargin: styleData.horizontal ? 1 : 3
					}
					radius: 3; smooth: true;
					color: styleData.pressed ? Qt.rgba(0, 0.93, 1, 0.5) : (styleData.hovered ? Qt.rgba(0, 0.73, 1, 0.5) : Qt.rgba(1, 1, 1, 0.5))
				}
			}

			rowDelegate: Rectangle {
				property bool loaded: playlist.loaded === styleData.row
				height: 40
				function plain(alternate) { return styleData.alternate ? Qt.rgba(0.0, 0.0, 0.0, 0.5) : Qt.rgba(0.25, 0.25, 0.25, 0.5) }
				function selected(ok, fallback) { return ok ? Qt.rgba(0, 0.93, 1.0, 0.5) : fallback }
				function current(ok, fallback) { return ok ? Qt.rgba(0, 0.73, 1, .5) : fallback }
				function inside(row, fallback) { return (0 <= row && row < table.rowCount) ? fallback : Qt.rgba(0, 0, 0, 0) }
				color: inside(styleData.row, selected(styleData.selected, current(loaded, plain(styleData.alternate))))
			}

			itemDelegate: Item {
				Column {
					width: parent.width
					Text {
						anchors { margins: 5; left: parent.left; right: parent.right }
						font {
							family: table.nameFontFamily; pixelSize: table.nameFontSize
							italic: playlist.loaded === styleData.row
							bold: playlist.loaded === styleData.row
						}
						verticalAlignment: Text.AlignVCenter; height: 25
						color: "white"; text: styleData.value; elide: Text.ElideRight
					}
					Text {
						anchors { margins: 5; left: parent.left; right: parent.right }
						font { family: table.locationFontFamily; pixelSize: table.locationFontSize }
						width: parent.width; height: table.locationFontSize; verticalAlignment: Text.AlignTop
						color: "white"; text: playlist.location(styleData.row); elide: Text.ElideRight
					}
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
