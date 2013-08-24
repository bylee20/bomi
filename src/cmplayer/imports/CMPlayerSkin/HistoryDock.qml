import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayerCore 1.0 as Core

Item {
	id: dock
	x: -dock.width; y: 20; visible: false
	width: 300; height: parent.height-y*2
	readonly property real widthHint: table.contentWidth+rect.radius+table.margins*2
	property bool show: false
	Rectangle { id: rect; anchors.fill: parent; color: "gray"; opacity: 0.8; radius: 5 }

	states: State {
		name: "show"; when: dock.show
		PropertyChanges { target: dock; explicit: true; x: -rect.radius }
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
		border { color: "black"; width: 1 }
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#ccc"}
			GradientStop {position: 1.0; color: "#333"}
		}
	}

	TableView {
		id: table
		readonly property real scrollArea: 10
		readonly property real margins: 15
		readonly property real contentWidth: name.width + latest.width + location.width
		anchors {
			fill: parent; topMargin: margins; leftMargin: margins+rect.radius;
			rightMargin: margins
			bottomMargin: margins
		}
		model: history
		frameVisible: false
		onDoubleClicked: history.play(row)

		TableViewColumn { id: name; role: "name"  ; title: qsTr("Name") ; width: 200 }
		TableViewColumn { id: latest; role: "latestplay"  ; title: qsTr("Latest Playback") ; width: 150 }
		TableViewColumn { id: location; role: "location"  ; title: qsTr("Location") ; width: 400 }

		itemDelegate: Item {
			Text {
				anchors { fill: parent; leftMargin: 5; rightMargin: 5 }
				color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
				text: styleData.value
			}
		}

		headerDelegate: Rectangle {
			implicitHeight: 20
			gradient: Gradient {
				GradientStop {position: 0.0; color: "#ddd"}
				GradientStop {position: 0.9; color: "#aaa"}
				GradientStop {position: 1.0; color: "#333"}
			}
			Text {
				anchors { fill: parent; leftMargin: 5; rightMargin: 5 }
				color: "black"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
				text: styleData.value
			}
			Rectangle {
				anchors { right: parent.right; verticalCenter: parent.verticalCenter }
				width: 1; height: parent.height*0.6; color: "gray"
			}
		}

		style: TableViewStyle {
			backgroundColor: "#555"
			alternateBackgroundColor: "#333"
			decrementControl: Item {} incrementControl: Item {}
			corner: Item {}
			scrollBarBackground: Rectangle {
				color: "#ddd"
				x: styleData.horizontal ? -1 : 1
				y: styleData.horizontal ? 1 : 1
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				Rectangle {
					color: parent.color
					x: parent.width
					visible: styleData.horizontal
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
						topMargin: styleData.horizontal ? 3 : 2
					}
					opacity: 0.8; radius: 3; smooth: true; color: "black"
				}
			}
		}
	}

	MouseArea {
		anchors.fill: parent
		acceptedButtons: Qt.RightButton
		onClicked: Core.Util.execute("tool/history")
	}
}
