import QtQuick 2.0
import bomi 1.0 as Cp
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: player
    objectName: "player"
    property real dockZ: 0.0
    property real bottomPadding: 0.0
    readonly property QtObject engine: Cp.App.engine
    Logo { anchors.fill: parent }
    TextOsd { id: msgosd }
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

    ProgressOsd { id: timeline; value: engine.rate }
    PlayInfoView { objectName: "playinfo" }
    Item {
        anchors.fill: parent; z: dockZ
        PlaylistDock {
            id: right
            show: Cp.App.playlist.visible
            width: Math.min(widthHint, player.width-(left.x+left.width)-20)
            height: parent.height-2*y - bottomPadding
        }
        HistoryDock {
            id: left
            show: Cp.App.history.visible
            width: Math.min(widthHint, player.width*0.4)
            height: parent.height-2*y - bottomPadding
        }
    }
    Component.onCompleted: {
        engine.screen.parent = player
        engine.screen.width = width
        engine.screen.height = height
    }
    onWidthChanged: engine.screen.width = width
    onHeightChanged: engine.screen.height = height

    function showOSD(msg) { msgosd.text = msg; msgosd.show(); }
    function showMessageBox(msg) { msgbox.visible = !!msg; boxmsg.text = msg }
    function showTimeLine() { timeline.show(); }

    Cp.MessageBox {
        id: downloadMBox
        parent: Cp.App.topLevelItem
        width: 300
        height: 100
        anchors.centerIn: parent
        title.text: qsTr("Download")
        message.text: Cp.App.download.url
        message.elide: Text.ElideMiddle
        message.verticalAlignment: Text.AlignVCenter
        visible: false
        Connections {
            target: Cp.App.download
            onRunningChanged: {
                downloadMBox.visible = Cp.App.download.running
                if (Cp.App.download.running)
                    Cp.App.topLevelItem.visible = true
                else
                    Cp.App.topLevelItem.check()
            }
        }
        buttonBox.buttons: [Cp.ButtonBox.Cancel]
        buttonBox.onClicked: {
            if (button == Cp.ButtonBox.Cancel)
                Cp.App.download.cancel()
        }

        readonly property QtObject download: Cp.App.download
        customItem: Cp.ProgressBar {
            id: prog
            value: Cp.App.download.rate
            property bool writing: Cp.App.download.writtenSize >= 0
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
                text: prog.sizeText(Cp.App.download.writtenSize)
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
                text: prog.sizeText(Cp.App.download.totalSize)
            }
        }

    }
}
