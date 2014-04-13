import QtQuick 2.2
import QtQuick.Layouts 1.1

Rectangle {
	id: box
	anchors.centerIn: parent
	radius: 5
	color: Qt.rgba(0, 0, 0, 1);
	opacity: 0.75
	visible: false
	property alias title: titletxt
	property alias message: messagetxt
	property bool dismissable: true
	property Item customItem: Item { }
	function dismiss() {
		if (dismissable) {
			visible = false
			dismissed()
		}
	}
	signal dismissed

	onCustomItemChanged: customItem.parent = custom

	ColumnLayout {
		anchors.margins: 5
		anchors.fill: parent
		Text {
			id: titletxt
			Layout.fillWidth: true
			color: "white"
			horizontalAlignment: Text.AlignHCenter
			height: 20
			Item {
				id: closebtn
				anchors.verticalCenter: parent.verticalCenter
				anchors.right: parent.right
				width: 10
				height: 10

				visible: box.dismissable
				property color color: crossarea.containsMouse ? "white" : "gray"
				onColorChanged: canvas.requestPaint()
				Canvas {
					id: canvas
					anchors.fill: parent
					readonly property real thickness: 3
					onPaint: {
						var ctx = canvas.getContext('2d');
						ctx.save();
						ctx.clearRect(0, 0, canvas.width, canvas.height);
						ctx.fillStyle = closebtn.color
						ctx.translate(canvas.width*0.5, canvas.height*0.5)
						ctx.rotate(Math.PI/4)
						ctx.strokeStyle = Qt.rgba(0, 0, 0, 0);
						ctx.fillRect(-canvas.width*0.5, -canvas.thickness*0.5, canvas.width, canvas.thickness)
						ctx.fillRect(-canvas.thickness*0.5, -canvas.height*0.5, canvas.thickness, canvas.height)
						ctx.restore();
					}
				}
				MouseArea {
					anchors.fill: parent
					id: crossarea
					onClicked: box.dismiss()
				}
			}
		}
		Text {
			id: messagetxt
			Layout.fillWidth: true
			Layout.fillHeight: true
			color: "white"
			anchors.margins: 20
		}
		Item {
			id: custom
			width: parent.width
			height: childrenRect.height
		}
	}
}
