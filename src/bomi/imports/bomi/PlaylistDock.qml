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
    x: parent.width; y: 20; width: widthHint; height: parent.height-2*y; visible: false
    states: State {
        name: "show"; when: dock.show
        PropertyChanges { target: dock; visible: true }
        PropertyChanges { target: dock; x: dock.dest; explicit: true }
    }
    transitions: Transition {
        reversible: true; to: "show"
        SequentialAnimation {
            PropertyAction { property: "visible" }
            NumberAnimation { property: "x" }
        }
    }
    function updateDestination() { dock.dest = dock.parent.width-dock.width }
    Component.onCompleted: updateDestination()
    Connections { target: parent; onWidthChanged: { updateDestination() } }
    onWidthChanged: { updateDestination() }
    onDestChanged: { if (dock.show) dock.x = dock.dest }

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
    }

    B.ModelView {
        id: table

        model: playlist
        headerVisible: false
        rowHeight: showLocation ? 40 : 25
        readonly property int nameFontSize: 15
        readonly property bool showLocation: B.App.theme.playlist.showLocation
        readonly property int locationFontSize: 10
        readonly property string nameFontFamily: B.App.theme.monospace
        readonly property string locationFontFamily: B.App.theme.monospace
        currentIndex: playlist.loaded
        function contentWidth() {
            var max = 0;
            for (var i=0; i<table.count; ++i) {
                var number = B.App.textWidth(playlist.number(i), table.nameFontSize, table.nameFontFamily);
                var name = B.App.textWidth(playlist.name(i), table.nameFontSize, table.nameFontFamily);
                if (showLocation) {
                    var loc = B.App.textWidth(playlist.location(i), table.locationFontSize, table.locationFontFamily);
                    max = Math.max(number + name, loc, max);
                } else
                    max = Math.max(number + name, max);
            }
            return max+30
        }

        Connections {
            target:playlist;
            onSelectedChanged: table.selectedIndex = playlist.selected
        }
        onSelectedIndexChanged: playlist.selected = table.selectedIndex

        onCountChanged: column.width = contentWidth()
        columns: B.ItemColumn { title: "Name"; role: "name"; width: 200; id: column}

        onActivated: playlist.play(index)
        itemDelegate: Item {
            Column {
                width: parent.width
                Text {
                    anchors { margins: 5; left: parent.left; right: parent.right }
                    font {
                        family: table.nameFontFamily; pixelSize: table.nameFontSize
                        italic: current
                        bold: current
                    }
                    verticalAlignment: Text.AlignVCenter; height: 25
                    color: "white"; text: value; elide: Text.ElideRight
                }
                Text {
                    visible: table.showLocation
                    anchors { margins: 5; left: parent.left; right: parent.right }
                    font { family: table.locationFontFamily; pixelSize: table.locationFontSize }
                    width: parent.width; height: table.locationFontSize; verticalAlignment: Text.AlignTop
                    color: "white"; text: playlist.location(index); elide: Text.ElideRight
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/playlist")
    }
}
