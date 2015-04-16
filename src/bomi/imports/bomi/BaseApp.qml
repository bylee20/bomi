import QtQuick 2.0
import bomi 1.0

Item {
    id: root
    property Player player
    property string name
    property size minimumSize: Qt.size(400, 300)
    readonly property Engine engine: App.engine
    property ToolPlaneStyle toolStyle: ToolPlaneStyle { }
    readonly property real toolMinX: left.status == __ToolEdge ? 0 : (left.x, mapFromItem(left, left.width, 0).x)
    readonly property real toolMaxX: right.status == __ToolEdge ? width : (width, right.x, mapFromItem(right, 0, 0).x)
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
            hideDelay: 2500
            width: 15; height: parent.height; anchors.right: parent.right
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.playlist; box: right;
            blockHiding: right.blockHiding
        }

        AutoDisplayZone {
            id: leftEdge
            y: left.y
            hideDelay: 2500
            width: 15; height: parent.height; anchors.left: parent.left
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.history; box: left;
            blockHiding: left.blockHiding
        }

        Component {
            id: bar
            Rectangle {
                anchors.bottom: parent.bottom
                width: 10;
                height: engine.audio.spectrum[0]
            }
        }

//        Rectangle {
//            id: fft
//            width: 200
//            height: 100
//            anchors.centerIn: parent

//            clip: true

//            VisualizationHelper {
//                id: vh
//                count: 100
//                audio: engine.audio
//                active: false
//            }

//            Row {
//                Repeater {
//                    model: vh.count
//                    Rectangle {
//                        id: rect
//                        width: 2
//                        height: vh.spectrum[index]
//                        anchors.bottom: parent.bottom
//                        color: "red";
//                        onHeightChanged: {
//                            ha.stop(); ha.start()
//                        }

//                        NumberAnimation {
//                            id: ha
//                            target: rect
//                            property: "height"
//                            to: 0
//                            duration: 1000
//                        }
//                    }
//                }
//                height: parent.height
//            }
//        }
    }
}
