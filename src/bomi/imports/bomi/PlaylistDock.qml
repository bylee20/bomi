import QtQuick 2.0
import bomi 1.0 as B
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: dock
    readonly property real widthHint: 500
    readonly property QtObject playlist: B.App.playlist
    property alias selectedIndex: table.selectedIndex
    property real dest: 0
    property bool show: false
    y: 20; width: widthHint; height: parent.height-2*y; visible: false

    SequentialAnimation {
        id: pull
        PropertyAction { target: dock; property: "visible"; value: true }
        NumberAnimation { target: dock; property: "x"; to: dock.dest }
    }

    SequentialAnimation {
        id: push
        NumberAnimation { target: dock; property: "x"; to: dock.parent.width }
        PropertyAction { target: dock; property: "visible"; value: false }
    }
    function updateDestination() {
        dock.dest = dock.parent.width - dock.width
        push.running = pull.running = false
        if (show)
            dock.x = dest
        else
            dock.x = parent.width
        dock.visible = show
    }
    Connections {
        target: parent
        onWidthChanged: {
            updateDestination()

        }
    }
    onWidthChanged: { updateDestination() }
    onShowChanged: {
        if (show) {
            push.running = false
            pull.running = true
        } else {
            pull.running = false
            push.running = true
        }
    }

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
    }

    B.PlaylistView {
        id: table
        margins: 15
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/playlist")
    }
}
