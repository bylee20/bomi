import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

Item {
    id: dock
    width: 300; height: parent.height
    readonly property alias blockHiding: view.blockHiding
    readonly property int widthHint: view.contentWidth+view.margins*2
    property alias selectedIndex: view.selectedIndex
    readonly property QtObject history: B.App.history
    property int status: __ToolHidden
    anchors.right: parent.left
    visible: anchors.rightMargin < 0

    states: [
        State {
            name: "hidden"; when: dock.status == __ToolHidden
            PropertyChanges { target: dock; anchors.rightMargin: 0 }
        }, State {
            name: "visible"; when: dock.status == __ToolVisible
            PropertyChanges { target: dock; anchors.rightMargin: -dock.width }
        }, State {
            name: "edge"; when: dock.status == __ToolEdge
            PropertyChanges { target: dock; anchors.rightMargin: -15 }
        }
    ]

    transitions: Transition {
        NumberAnimation { target: dock; property: "anchors.rightMargin" }
    }

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
    }

    Component {
        id: starComponent
        Image {
            width: 16; height: 16
            source: starArea.containsMouse || history.isStarred(row)
                    ? "qrc:/img/fav-on.png" : "qrc:/img/fav-off.png"
            MouseArea {
                id: starArea
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    history.setStarred(row, !history.isStarred(row))
                }
            }
        }
    }

    B.ModelView {
        id: view
        model: B.App.history
        titlePadding: title.height
        anchors.rightMargin: 1
        rowHeight: 26
        columns: [
            ItemColumn { width: 200; title: qsTr("Name"); role: "name"; index: 1 },
            ItemColumn { width: 150; title: qsTr("Latest Playback"); role: "latestplay" },
            ItemColumn { width: 400; title: qsTr("Location"); role: "location" }
        ]
        itemDelegate: Item {
            Loader {
                readonly property int row: index
                sourceComponent: column.index > 0 ? starComponent : undefined
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                anchors { fill: parent; leftMargin: column.index > 0 ? 18 : 0 }
                text: value; color: "white"; elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        onActivated: B.App.history.play(index)
    }

    Rectangle {
        width: 1
        height: parent.height
        anchors.left: view.right
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/history")
    }

    Text {
        id: title
        text: width < 200 ? qsTr("History"): qsTr("Playback History")
        height: 30
        width: parent.width - 2 * 20
        color: "white"
        font.pixelSize: 16
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.horizontalCenter: parent.horizontalCenter
        B.Button {
            size: 16
            icon.source: "qrc:/img/tool-clear.png"
            anchors {
                verticalCenter: title.verticalCenter
                left: parent.left
            }
            emphasize: 0.05
            onClicked: B.App.execute("tool/history/clear")
        }

        B.Button {
            size: 16
            icon.source: "qrc:/img/tool-close.png"
            anchors {
                verticalCenter: title.verticalCenter
                right: parent.right
            }
            emphasize: 0.05
            onClicked: B.App.history.visible = false
        }
    }
}
