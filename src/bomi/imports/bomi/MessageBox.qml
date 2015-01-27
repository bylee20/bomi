import QtQuick 2.2
import QtQuick.Layouts 1.1
import bomi 1.0 as B

Rectangle {
    id: box
    anchors.centerIn: parent
    radius: 5
    color: Qt.rgba(0, 0, 0, 1);
    opacity: 0.75
    visible: false
    property alias title: titleItem
    property alias message: messageItem
    property bool dismissable: true
    property alias buttonBox: bboxItem
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
            id: titleItem
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
            id: messageItem
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
        B.ButtonBox {
            id: bboxItem
            buttons: [B.ButtonBox.Cancel, B.ButtonBox.Ok]
            visible: buttons.length > 0
            width: 150; height: 25
            anchors.horizontalCenter: parent.horizontalCenter
            source: Rectangle {
                color: mouseArea.pressed ? Qt.rgba(1.0, 1.0, 1.0, 1.0)
                                         : Qt.rgba(0.3, 0.3, 0.3, 1.0)
                property alias text: textItem.text
                signal clicked
                Text {
                    id: textItem
                    color: mouseArea.pressed ? Qt.rgba(0.0, 0.0, 0.0, 1.0)
                                             : Qt.rgba(1.0, 1.0, 1.0, 1.0)
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: parent.clicked()
                }
            }
        }
    }
}
