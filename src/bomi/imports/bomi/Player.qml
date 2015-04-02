import QtQuick 2.0
import bomi 1.0 as B
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: player
    objectName: "player"
    property real dockZ: 0.0
    property real bottomPadding: 0.0
    property real topPadding: 0.0
    readonly property QtObject engine: B.App.engine
    property Item screen

    Logo { anchors.fill: parent }

    TextOsd {
        id: msgosd;
        duration: B.App.theme.osd.message.duration
    }

    Rectangle {
        id: msgbox
        color: Qt.rgba(0.86, 0.86, 0.86, 0.8)
        visible: false
        anchors.centerIn: parent
        width: boxmsg.width + pad*2
        height: boxmsg.height + pad*2
        readonly property real pad: 5
        Text {
            id: boxmsg
            anchors.centerIn: parent
            width: contentWidth
            height: contentHeight
            font.bold: true
        }
        radius: 5
        border.color: "black"
        border.width: 1
    }

    ProgressOsd {
        id: timeline; value: engine.rate
        duration: B.App.theme.osd.timeline.duration
        x: (parent.width - width)*0.5
        y: {
            var m = B.App.theme.osd.timeline.margin
            switch (B.App.theme.osd.timeline.position) {
            case B.TimelineTheme.Top:
                return parent.height * m;
            case B.TimelineTheme.Bottom:
                return parent.height * (1.0 - m) - height;
            default:
                return (parent.height - height)*0.5;
            }
        }
    }

    Component {
        id: playinfo
        PlayInfoView { }
    }

    Loader {
        objectName: "playinfo"
        property bool show: false
        readonly property int fontSize: parent.height*0.022;
        sourceComponent: show ? playinfo : undefined
        width: parent.width-fontSize*2;
        height: parent.height-fontSize*2;
        anchors.centerIn: parent
    }

    Item {
        anchors.fill: parent; z: dockZ
        PlaylistDock {
            id: right
            y: topPadding
            show: B.App.playlist.visible
            width: Math.min(widthHint, player.width-(left.x+left.width)-20)
            height: parent.height - bottomPadding - topPadding
        }
        HistoryDock {
            id: left
            y: topPadding
            show: B.App.history.visible
            width: Math.min(widthHint, player.width*0.4)
            height: parent.height - bottomPadding - topPadding
        }
    }

    function updateScreenSize() {
        if (!screen)
            return
        screen.width = width * engine.zoom
        screen.height = height * engine.zoom
        screen.x = (width - screen.width) * 0.5
        screen.y = (height - screen.height) * 0.5
    }

    Connections { target: engine; onZoomChanged: updateScreenSize() }
    onWidthChanged: updateScreenSize()
    onHeightChanged: updateScreenSize()

    property var showOsdFunc: function(msg){ msgosd.text = msg; msgosd.show(); }
    function showOSD(msg) { showOsdFunc(msg) }
    function showMessageBox(msg) { msgbox.visible = !!msg; boxmsg.text = msg }
    function showTimeLine() { timeline.show(); }

    B.MessageBox {
        id: downloadMBox
        parent: B.App.topLevelItem
        width: 300; height: 100
        anchors.centerIn: parent
        title.text: qsTr("Download")
        message.text: B.App.download.url
        message.elide: Text.ElideMiddle
        message.verticalAlignment: Text.AlignVCenter
        visible: false
        Connections {
            target: B.App.download
            onRunningChanged: {
                downloadMBox.visible = B.App.download.running
                if (B.App.download.running)
                    B.App.topLevelItem.visible = true
                else
                    B.App.topLevelItem.check()
            }
        }
        buttonBox.buttons: [B.ButtonBox.Cancel]
        buttonBox.onClicked: {
            if (button == B.ButtonBox.Cancel)
                B.App.download.cancel()
        }

        readonly property QtObject download: B.App.download
        customItem: B.ProgressBar {
            id: prog
            value: B.App.download.rate
            property bool writing: B.App.download.writtenSize >= 0
            function sizeText(size) {
                if (size < 0)
                    return "??"
                if (size < 1024*1024)
                    return (size/1024).toFixed(3) + "KiB"
                return (size/(1024*1024)).toFixed(3) + "MiB"
            }
            Text {
                height: parent.height
                anchors.right: progSlash.left
                anchors.rightMargin: 2
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                color: "black"
                visible: prog.writing
                text: prog.sizeText(B.App.download.writtenSize)
            }
            Text {
                id: progSlash
                anchors.horizontalCenter: parent.horizontalCenter
                width: 5; height: parent.height
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                color: "black"
                text: prog.writing ? "/" : qsTr("Connecting...")
            }
            Text {
                height: parent.height
                anchors.left: progSlash.right
                anchors.leftMargin: 2
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                color: "black"
                visible: prog.writing
                text: prog.sizeText(B.App.download.totalSize)
            }
        }
    }

    B.AutoDisplayZone {
        id: rightEdge
        y: right.y
        width: 15; height: right.height; anchors.right: parent.right
        visible: B.App.theme.controls.showPlaylistOnMouseOverEdge
        target: B.App.playlist; box: right;
    }

    B.AutoDisplayZone {
        id: leftEdge
        y: left.y
        width: 15; height: left.height; anchors.left: parent.left
        visible: B.App.theme.controls.showHistoryOnMouseOverEdge
        target: B.App.history; box: left;
    }

}
