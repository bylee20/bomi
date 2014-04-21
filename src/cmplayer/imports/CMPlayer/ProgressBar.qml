import QtQuick 2.2

Rectangle {
    id: frame
    color: "transparent"
    border.width: 1
    border.color: "white"
    width: parent.width
    height: 20
    property real value: -1
    readonly property real pad: border.width + 1
    Item {
        id: progbar
        anchors.fill: parent
        visible: frame.value >= 0
        Rectangle {
            id: filled
            x: frame.pad; y: frame.pad
            width: (parent.width - 2*frame.pad)*frame.value
            height: parent.height - 2*frame.pad
            color: "white"
        }

        Rectangle {
            anchors.left: filled.right
            y: frame.pad
            width: (parent.width - 2*frame.pad)*(1 - frame.value)
            height: parent.height - 2*frame.pad
            color: "gray"
        }
    }
    Rectangle {
        visible: frame.value < 0
        anchors.fill: parent
        anchors.margins: parent.pad
        color: "gray"
    }
    Rectangle {
        id: busybar
        visible: frame.value < 0
        width: 20
        height: parent.height - 2*frame.pad
        y: frame.pad
    }
    SequentialAnimation {
        id: mover
        loops: -1
        running: frame.value < 0 && frame.visible
        NumberAnimation {
            target: busybar
            property: "x"
            from: frame.pad
            to: frame.width - frame.pad - busybar.width
            easing.type: Easing.InOutQuad
            duration: 1000
        }
        NumberAnimation {
            target: busybar
            property: "x"
            from: frame.width - frame.pad - busybar.width
            to: frame.pad
            easing.type: Easing.InOutQuad
            duration: 1000
        }
    }
}
