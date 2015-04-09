import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

Item {
    id: dock
    x: -dock.width; y: 20; visible: false
    width: 300; height: parent.height-y*2
    readonly property real widthHint: view.contentWidth+view.margins*2
    property alias selectedIndex: view.selectedIndex
    property bool show: false
    readonly property QtObject history: B.App.history
    states: State {
        name: "show"; when: dock.show
        PropertyChanges { target: dock; explicit: true; x: 0 }
        PropertyChanges { target: dock; visible: true }
    }
    transitions: Transition {
        reversible: true; to: "show"
        PropertyAction { property: "visible" }
        NumberAnimation { property: "x" }
    }

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
    }

    Component {
        id: starComponent
        Image {
            width: 16; height: 16
            source: starArea.containsMouse || history.isStarred(row) ? "qrc:/img/fav-on.png" : "qrc:/img/fav-off.png"
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
        columns: [
            ItemColumn { width: 200; title: qsTr("Name"); role: "name"; index: 1 },
            ItemColumn { width: 150; title: qsTr("Latest Playback"); role: "latestplay" },
            ItemColumn { width: 400; title: qsTr("Location"); role: "location" }
        ]
        itemDelegate: Item {
            Loader {
                x: -3
                readonly property int row: index
                sourceComponent: column.index > 0 ? starComponent : undefined
            }

            Text {
                x: column.index > 0 ? 14 : 0; width: parent.width - x
                text: value; color: "white"; elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }
        onActivated: B.App.history.play(index)
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/history")
    }
}
