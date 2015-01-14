import QtQuick 2.0

Osd {
    id: osd
    property real value: 0.0
    height: parent.height*0.06
    width: parent.width*0.8
    x: (parent.width - width)*0.5
    y: (parent.height - height)*0.5
    Rectangle {
        id: border
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0)
        border.color: Qt.rgba(0.0, 0.0, 0.0, 0.5)
        border.width: height*0.06
        Rectangle {
            id: inner
            anchors.fill: parent
            anchors.margins: border.border.width
            color: Qt.rgba(0, 0, 0, 0)
            border.color: Qt.rgba(1.0, 1.0, 1.0, 0.5)
            border.width: border.border.width
            Rectangle {
                id: filled
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: border.border.width*2
                anchors.rightMargin: 0
                color: Qt.rgba(1.0, 1.0, 1.0, 0.5)
                width: (parent.width-border.border.width*4)*value
            }
        }
    }
}
