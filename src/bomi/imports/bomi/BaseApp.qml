import QtQuick 2.0
import bomi 1.0

Item {
    id: root
    property Player player
    property string name
    property size minimumSize: Qt.size(400, 300)
    readonly property Engine engine: App.engine
    property ToolPlaneStyle toolStyle: ToolPlaneStyle { }
    readonly property real toolMinX: mapFromItem(left, left.x + left.width, 0).x
    readonly property real toolMaxX: mapFromItem(right, right.x, 0).x

    property real trackingMinX: toolMinX
    property real trackingMaxX: toolMaxX
    property real trackingMinY: 0
    property real trackingMaxY: height

    Item {
        anchors {
            fill: parent
            topMargin: toolStyle.topMargin
            bottomMargin: toolStyle.bottomMargin
            leftMargin: toolStyle.leftMargin
            rightMargin: toolStyle.rightMargin
        }
        z: toolStyle.z

        PlaylistView {
            id: right
            y: 0
            show: App.playlist.visible
            width: Math.min(widthHint, player.width-(left.x+left.width)-20)
            height: parent.height
        }
        HistoryView {
            id: left
            y: 0
            show: App.history.visible
            width: (Math.min(widthHint, player.width*0.4) | 0)
            height: parent.height
        }
        AutoDisplayZone {
            id: rightEdge
            y: right.y
            width: 15; height: right.height; anchors.right: parent.right
            visible: App.theme.controls.showPlaylistOnMouseOverEdge
            target: App.playlist; box: right;
        }
        AutoDisplayZone {
            id: leftEdge
            y: left.y
            width: 15; height: left.height; anchors.left: parent.left
            visible: App.theme.controls.showHistoryOnMouseOverEdge
            target: App.history; box: left;
        }
    }
}
