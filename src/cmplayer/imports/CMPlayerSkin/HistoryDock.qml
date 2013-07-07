import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
	id: dock
//	visible: false
	x: -dock.width
	y: 20;
	width: 300; height: parent.height-y*2
	onVisibleChanged: if (visible) { sliding.start(); }
	readonly property real widthHint: table.contentWidth+rect.radius+table.margins*2
	property bool show: false
	Rectangle { id: rect; anchors.fill: parent; color: "gray"; opacity: 0.8; radius: 5 }

//	Component.onCompleted: x = -dock.width

	states: State {
		name: "show"; when: dock.show
		PropertyChanges { target: dock; explicit: true; x: -rect.radius }
	}
	transitions: Transition {
		reversible: true; to: "show"
		NumberAnimation { property: "x" }
	}

//	NumberAnimation {
//		id: sliding;		target: dock;		property: "x"
//		from: -dock.width;	to: -rect.radius;	easing.type: Easing.OutCubic
//	}

	Rectangle {
		id: frame
		x: table.x-1; y: table.y-1
		width: table.width+2-(true? table.scrollArea : 0);
		height: table.height+2-(table.hScrollVisible ? table.scrollArea : 0);
		border { color: "black"; width: 1 } color: "transparent"
	}

	TableView {
		id: table
		readonly property real scrollArea: 12
		readonly property real margins: 15
		readonly property real contentWidth: name.width + latest.width + location.width
		readonly property bool hScrollVisible: contentWidth > viewport.width
		anchors {
			fill: parent; topMargin: margins; leftMargin: margins+rect.radius;
			rightMargin: margins - (true ? table.scrollArea : 0);
			bottomMargin: margins - (table.hScrollVisible ? table.scrollArea : 0)
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
			decrementControl: Item {} incrementControl: Item {} corner: Item {}
			scrollBarBackground: Item { implicitWidth: table.scrollArea; implicitHeight: table.scrollArea }
			handle: Item {
				implicitWidth: table.scrollArea; implicitHeight: table.scrollArea
				Rectangle {
					anchors {
						fill: parent; rightMargin: 2; bottomMargin: 2
						leftMargin: styleData.horizontal ? 2 : 4
						topMargin: styleData.horizontal ? 4 : 2
					}
					opacity: 0.8; radius: 3; smooth: true; color: "white"
				}
			}
		}
	}
//	MouseArea { anchors.fill: parent; acceptedButtons: Qt.RightButton }
}
