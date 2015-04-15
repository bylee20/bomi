import QtQuick 2.0
import bomi 1.0

Item {
    id: root
    property Player player
    property string name
    property size minimumSize: Qt.size(400, 300)
    readonly property Engine engine: App.engine
    property ToolPlaneStyle toolStyle: ToolPlaneStyle { }
    readonly property real toolMinX: (left.x, mapFromItem(left, left.width, 0).x)
    readonly property real toolMaxX: (width, right.x, mapFromItem(right, 0, 0).x)
    readonly property int __ToolVisible: 2
    readonly property int __ToolHidden: 0
    readonly property int __ToolEdge: 1

    property real trackingMinX: toolMinX
    property real trackingMaxX: toolMaxX
    property real trackingMinY: 0
    property real trackingMaxY: height

    function dismissTools() {
        App.playlist.visible = App.history.visible = false
    }

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
            width: Math.min(widthHint, player.width-(left.x+left.width)-20) | 0
            height: parent.height; status: rightEdge.status
        }
        HistoryView {
            id: left
            width: (Math.min(widthHint, player.width*0.4) | 0)
            height: parent.height; status: leftEdge.status
        }
    }
    Item {
        anchors {
            fill: parent
            topMargin: toolStyle.topMarginForInteraction
            bottomMargin: toolStyle.bottomMarginForInteraction
            leftMargin: toolStyle.leftMargin
            rightMargin: toolStyle.rightMargin
        }
        z: toolStyle.z

        AutoDisplayZone {
            id: rightEdge
            y: right.y
            width: 15; height: parent.height; anchors.right: parent.right
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.playlist; box: right;
        }

        AutoDisplayZone {
            id: leftEdge
            y: left.y
            width: 15; height: parent.height; anchors.left: parent.left
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.history; box: left;
        }
    }
}
