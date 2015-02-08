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

    B.ModelView {
        id: view
        model: B.App.history
        columns: [
            ItemColumn { width: 200; title: qsTr("Name"); role: "name" },
            ItemColumn { width: 150; title: qsTr("Latest Playback"); role: "latestplay" },
            ItemColumn { width: 400; title: qsTr("Location"); role: "location" }
        ]
        itemDelegate: Text { text: value; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
        onActivated: B.App.history.play(index)
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/history")
    }
}
