import QtQuick 2.0
import bomi 1.0 as B
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: dock
    readonly property alias blockHiding: view.blockHiding
    readonly property real widthHint: {
        var w = 30
        if (_location.show)
            w += Math.min(widthHintForName, widthHintForLocation)
        else
            w += widthHintForName
        return Math.max(200, w);
    }
    readonly property alias widthHintForName: view.nameWidth
    readonly property alias widthHintForLocation: view.locationWidth
    readonly property QtObject playlist: B.App.playlist
    property alias selectedIndex: view.selectedIndex
    property int status: __ToolHidden

    width: widthHint; height: parent.height - 2*y; anchors.left: parent.right
    visible: anchors.leftMargin < 0

    states: [
        State {
            name: "hidden"; when: dock.status == __ToolHidden
            PropertyChanges { target: dock; anchors.leftMargin: 0 }
        }, State {
            name: "visible"; when: dock.status == __ToolVisible
            PropertyChanges { target: dock; anchors.leftMargin: -dock.width }
        }, State {
            name: "edge"; when: dock.status == __ToolEdge
            PropertyChanges { target: dock; anchors.leftMargin: -15 }
        }
    ]

    transitions: Transition {
        NumberAnimation { target: dock; property: "anchors.leftMargin" }
    }

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
    }

    Rectangle {
        width: 1; height: parent.height
        anchors.left: parent.left
    }

    B.ModelView {
        id: view
        model: B.App.playlist
        titlePadding: title.height
        anchors {
            leftMargin: 1
            bottomMargin: parent.height - bottomSeparator.y
        }
        Text {
            id: _name
            font { pixelSize: 15 }
            visible: false
            text: "DUMMY TEXT FOR HEIGHT"
            readonly property real h: contentHeight
            function getWidth(text) {
                return B.App.textWidth(text, font.pixelSize, font.family)
            }
        }
        Text {
            id: _location
            readonly property bool show: B.App.theme.controls.showLocationsInPlaylist
            readonly property real h: contentHeight
            font { pixelSize: 10; family: B.App.theme.monospace }
            visible: false
            text: "DUMMY TEXT FOR HEIGHT"
            function getWidth(text) {
                return B.App.textWidth(text, font.pixelSize, font.family)
            }
        }

        headerVisible: false
        rowHeight: _name.contentHeight + (_location.show ? _location.contentHeight : 0) + 14;

        property real nameWidth: 0
        property real locationWidth: 0
        currentIndex: model.loaded
        function updateWidthHints() {
            var nameMax = 0, locMax = 0;
            for (var i=0; i<view.count; ++i) {
                var number = _name.getWidth(model.number(i))
                var name = _name.getWidth(model.name(i))
                nameMax = Math.max(nameMax, number + name)
                if (_location.show)
                    locMax = Math.max(locMax, _location.getWidth(model.location(i)))
            }
            nameWidth = nameMax
            locationWidth = locMax
        }

        Connections {
            target: view.model;
            onSelectedChanged: view.selectedIndex = target.selected
        }

        columns: B.ItemColumn { title: "Name"; role: "name"; width: 200; id: column}

        onSelectedIndexChanged: model.selected = view.selectedIndex
        onCountChanged: {
            updateWidthHints()
            column.width = Math.max(nameWidth, locationWidth)
        }
        onActivated: model.play(index)

        itemDelegate: Item {
            Column {
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors { margins: 5; left: parent.left; right: parent.right }
                    font: _name.font
                    verticalAlignment: Text.AlignVCenter; height: _name.h
                    color: "white"; text: value; elide: Text.ElideRight
                }
                Text {
                    visible: _location.show
                    anchors { margins: 5; left: parent.left; right: parent.right }
                    font: _location.font
                    width: parent.width; height: _location.h; verticalAlignment: Text.AlignTop
                    color: "white"; text: view.model.location(index); elide: Text.ElideRight
                }
            }
        }
    }


    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: B.App.execute("tool/playlist")
    }

    Item {
        id: title
        height: 61; width: parent.width - 2 * 20
        anchors.horizontalCenter: parent.horizontalCenter
        B.Button {
            size: 16
            icon.source: "qrc:/img/tool-close.png"
            y: 7
            anchors {
                left: parent.left
            }
            emphasize: 0.05
            onClicked: B.App.playlist.visible = false
        }

        Text {
            text: qsTr("Playlist")
            color: "white"; font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            width: parent.width
            height: 30
        }
        Item {
            width: Math.min(parent.width, number.width + marqueeItem.width); height: 30
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            B.Text {
                id: number
                readonly property QtObject pl: B.App.playlist
                textStyle {
                    color: "white"; font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
                content: '[' + B.Format.listNumber(pl.currentNumber, pl.length) + ']'
                width: contentWidth + 2; height: parent.height
            }
            Item {
                id: marqueeItem
                clip: true
                x: number.width; height: parent.height
                width: Math.min(title.width - x, mediaName.width)
                anchors.bottom: parent.bottom
                B.Text {
                    id: mediaName
                    readonly property real xTo: Math.min(0, -(mediaName.width - parent.width))
                    anchors.bottom: parent.bottom
                    width: contentWidth; height: parent.height
                    textStyle: number.textStyle
                    content: B.App.engine.media.name// + "asdasdasdasdasdasdasdasdasd1"
                }

                SequentialAnimation {
                    id: marquee
                    NumberAnimation {
                        target: mediaName
                        property: "x"
                        duration: 1500
                        easing.type: Easing.Linear
                        from: 0; to: 0
                    }
                    NumberAnimation {
                        target: mediaName
                        property: "x"
                        duration: 1000
                        easing.type: Easing.InOutQuad
                        from: 0; to: mediaName.xTo
                    }
                }
                Timer {
                    id: marqueeTimer
                    interval: 4000
                    onTriggered: {
                        marquee.stop()
                        mediaName.x = 0
                        if (mediaName.xTo < 0)
                            marquee.start()
                    }
                    repeat: true
                    running: true
                    triggeredOnStart: true
                }
            }
        }
    }

    Rectangle {
        id: bottomSeparator
        width: parent.width; height: 1; anchors.bottom: bbox.top
    }

    Component {
        id: checkableButton
        B.Button {
            size: 26; emphasize: 0.05
            icon { source: src; width: 22; height: 22 }
            background.color: ch ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0);
            background.radius: 3
            checked: ch; action: act
        }
    }

    Rectangle {
        id: bbox
        anchors {
            bottom: parent.bottom
            right: parent.right
            left: parent.left
            leftMargin: 1
        }
        height: 50
        color: Qt.rgba(0, 0, 0, 0.8)
        Row {
            anchors.centerIn: parent
            spacing: 40
            Loader {
                sourceComponent: checkableButton
                readonly property url src: "qrc:/img/tool-repeat.png"
                readonly property bool ch: B.App.playlist.repetitive
                readonly property string act: "tool/playlist/repeat"
            }
            Loader {
                sourceComponent: checkableButton
                readonly property url src: "qrc:/img/tool-shuffle.png"
                readonly property bool ch: B.App.playlist.shuffled
                readonly property string act: "tool/playlist/shuffle"
            }
        }
    }
}
