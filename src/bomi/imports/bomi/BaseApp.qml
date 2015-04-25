import QtQuick 2.0
import QtQuick 2.0 as Q
import bomi 1.0

Item {
    id: root
    property string name
    property size minimumSize: Qt.size(300, 200)
    readonly property Engine engine: App.engine
    property ToolPlaneStyle toolStyle: ToolPlaneStyle { }
    readonly property real toolMinX: left.status == __ToolEdge ? 0 : (left.x, mapFromItem(left, left.width, 0).x)
    readonly property real toolMaxX: right.status == __ToolEdge ? width : (width, right.x, mapFromItem(right, 0, 0).x)
    readonly property int __ToolVisible: 2
    readonly property int __ToolHidden: 0
    readonly property int __ToolEdge: 1

    property bool titleBarVisible: false
    property real trackingMinX: toolMinX
    property real trackingMaxX: toolMaxX
    property real trackingMinY: 0
    property real trackingMaxY: height

    function dismissTools() {
        App.playlist.visible = App.history.visible = false
    }

    MouseArea {
        width: parent.width; height: 24
        z: toolStyle.z - 1
        onPressed: mouse.accepted = false
        Item {
            id: titleItem
            anchors.fill: parent
            visible: App.theme.controls.titleBarEnabled && titleBarVisible
                        && !App.window.fullscreen && App.window.frameless
            readonly property real h: visible ? height : 0
            Rectangle {
                width: parent.width; height: parent.height
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.4); }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.0); }
                }
            }
            Image {
                id: logo
                x: y; width: 16; height: 16; smooth: true
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/img/window-logo-filled.png"
            }
            Text {
                anchors.centerIn: parent
                content: (engine.media.name ? (engine.media.name + " - ") : "")
                            + App.displayName
                width: parent.width - 16 * 4 * 2
                textStyle {
                    horizontalAlignment: Q.Text.AlignHCenter
                    color: "white"; styleColor: "black"; style: Q.Text.Raised
                    font.pixelSize: 12; elide: Q.Text.ElideMiddle
                }
            }
            Row {
                anchors.right:parent.right; anchors.rightMargin: logo.x
                anchors.verticalCenter: parent.verticalCenter
                StateButton {
                    size: 16; tooltip: ""
                    prefix: "qrc:/img/window-minimize"; action: "window/minimize"
                }
                StateButton {
                    size: 16; tooltip: ""
                    prefix: App.window.maximized ? "qrc:/img/window-normal" : "qrc:/img/window-maximize"
                    onClicked: App.window.maximized ? App.window.showNormal() : App.execute("window/maximize")
                }
                StateButton {
                    size: 16; tooltip: ""
                    action: "window/close"; prefix: "qrc:/img/window-close"
                }
            }
        }
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
            width: Math.min(widthHint, player.width-(left.x+left.width)-20, player.width * 0.7) | 0
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
            id: rightEdge; hideDelay: 2500
            y: right.y + titleItem.h; anchors.right: parent.right
            width: 15; height: parent.height - titleItem.h
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.playlist; box: right;
            blockHiding: right.blockHiding
        }

        AutoDisplayZone {
            id: leftEdge; hideDelay: 2500
            y: left.y + titleItem.h; anchors.left: parent.left
            width: 15; height: parent.height - titleItem.h
            auto: App.theme.controls.showToolOnMouseOverEdge
            target: App.history; box: left;
            blockHiding: left.blockHiding
        }
    }
}
