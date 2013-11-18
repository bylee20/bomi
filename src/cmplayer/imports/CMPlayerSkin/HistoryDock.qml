import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayerCore 1.0 as Core

Item {
	id: dock
	x: -dock.width; y: 20; visible: false
	width: 300; height: parent.height-y*2
	readonly property real widthHint: table.contentWidth+table.margins*2
	property bool show: false
	states: State {
		name: "show"; when: dock.show
		PropertyChanges { target: dock; explicit: true; x: 0 }
		PropertyChanges { target: dock; visible: true }
	}
	transitions: Transition {
		reversible: true; to: "show"
		PropertyAction { property: "visible" }
		NumberAnimation { property: "x" }
	}

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
		readonly property real scrollArea: 10
		readonly property real margins: 15
		readonly property real contentWidth: name.width + latest.width + location.width
		anchors { fill: parent; margins: table.margins }
		model: history
		frameVisible: false
		onDoubleClicked: history.play(row)
		backgroundVisible: false

		TableViewColumn { id: name; role: "name"  ; title: qsTr("Name") ; width: 200 }
		TableViewColumn { id: latest; role: "latestplay"  ; title: qsTr("Latest Playback") ; width: 150 }
		TableViewColumn { id: location; role: "location"  ; title: qsTr("Location") ; width: 400 }


		headerDelegate: Rectangle {
			implicitHeight: 15
			color: Qt.rgba(1, 1, 1, 0.7)
			Text {
				anchors { fill: parent; leftMargin: 5; rightMargin: 5 }
				color: "black"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
				text: styleData.value
				font.pixelSize: 11
			}
			Rectangle {
				anchors { right: parent.right; verticalCenter: parent.verticalCenter }
				width: 1; height: parent.height*0.6; color: "black"
			}
		}

		style: TableViewStyle {
			decrementControl: Item {} incrementControl: Item {} corner: Item {}
			scrollBarBackground: Item {
				implicitWidth:  table.scrollArea
				implicitHeight: table.scrollArea
				x: styleData.horizontal ? -2 :  2
				y: styleData.horizontal ?  2 : -2
				Rectangle {
					border { color: "white"; width: 1 } color: Qt.rgba(0, 0, 0, 0)
					x: styleData.horizontal ?  0 : -2
					y: styleData.horizontal ? -2 :  1
					width:  parent.width  + (styleData.horizontal ?  3 : 2)
					height: parent.height + (styleData.horizontal ?  2 : 2)
				}
			}
			handle: Item {
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				Rectangle {
					anchors {
						fill: parent
						leftMargin:   styleData.horizontal ? 1 : 3
						rightMargin:  styleData.horizontal ? 3 : 1
						topMargin:    styleData.horizontal ? 3 : 2
						bottomMargin: styleData.horizontal ? 1 : 3
					}
					radius: 3; smooth: true;
					color: styleData.pressed ? Qt.rgba(0, 0.93, 1, 0.5) : (styleData.hovered ? Qt.rgba(0, 0.73, 1, 0.5) : Qt.rgba(1, 1, 1, 0.5))
				}
			}
			itemDelegate: Item {
				Text {
					anchors { fill: parent; leftMargin: 5; rightMargin: 5 }
					color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
					text: styleData.value
				}
			}

			rowDelegate: Rectangle {
				function plain(alternate) { return styleData.alternate ? Qt.rgba(0.0, 0.0, 0.0, 0.5) : Qt.rgba(0.25, 0.25, 0.25, 0.5) }
				function selected(ok, fallback) { return ok ? Qt.rgba(0, 0.93, 1.0, 0.5) : fallback }
				function current(ok, fallback) { return ok ? Qt.rgba(0, 0.73, 1, .5) : fallback }
				function inside(row, fallback) { return (0 <= row && row < table.rowCount) ? fallback : Qt.rgba(0, 0, 0, 0) }
				color: inside(styleData.row, selected(styleData.selected, plain(styleData.alternate)))
			}
		}
	}

	MouseArea {
		anchors.fill: parent
		acceptedButtons: Qt.RightButton
		onClicked: Core.Util.execute("tool/history")
	}
}
