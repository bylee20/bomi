import QtQuick 2.0
import bomi 1.0 as B
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
    id: player
    objectName: "player"
    property real bottomPadding: 0.0
    property real topPadding: 0.0
    property bool logoVisible: logo.visible
    readonly property QtObject engine: B.App.engine
    readonly property Item video: B.App.engine.video.screen
    property Item screen: !logo.visible ? video : logo

    Component { id: barVis; BarVisualizer { } }

    Item {
        id: cropbox
        objectName: "cropbox"
        visible: false
        anchors.fill: parent
        property int cx: 0
        property int cy: 0
        property int cw: 0
        property int ch: 0
        property color shade: Qt.rgba(1, 0, 0, 0.3)
        function updateArea() {
            var rect = video.mapFromVideo(Qt.rect(cx, cy, cw, ch))
            cropArea.x = rect.x
            cropArea.y = rect.y
            cropArea.width = rect.width
            cropArea.height = rect.height
        }
        Connections {
            target: B.App.engine.video.screen
            onFrameRectChanged: cropbox.updateArea()
            onWidthChanged: cropbox.updateArea()
            onHeightChanged: cropbox.updateArea()
        }

        Item {
            id: cropArea
//            MouseArea {
//                anchors.fill: parent
//                drag.target: cropArea; drag.axis: Drag.XAndYAxis
//                drag.minimumX: 0; drag.maximumX: player.width-width
//                drag.minimumY: 0; drag.maximumY: player.height-height
//            }
        }
        Rectangle {
            anchors {
                top: parent.top; bottom: cropArea.top
                left: parent.left; right: parent.right;
            }
            color: parent.shade
        }
        Rectangle {
            anchors {
                top: cropArea.top; bottom: cropArea.bottom
                left: parent.left; right: cropArea.left
            }
            color: parent.shade
        }
        Rectangle {
            anchors {
                top: cropArea.top; bottom: cropArea.bottom
                left: cropArea.right; right: parent.right
            }
            color: parent.shade
        }
        Rectangle {
            anchors {
                top: cropArea.bottom; bottom: parent.bottom
                left: parent.left; right: parent.right
            }
            color: parent.shade
        }
    }

    Logo { id: logo; anchors.fill: parent }

    Rectangle {
        anchors.fill: parent
        width: 300
        height: 100
        visible: engine.visualizer.enabled
        color: "black"
        Loader {
            anchors.fill: parent
            sourceComponent: {
                if (!engine.visualizer.enabled)
                    return undefined
                switch (engine.visualizer.type) {
                case B.Visualizer.Bar:
                    return barVis;
                default:
                    return undefined
                }
            }
        }
    }

    TextOsd {
        id: msgosd;
        anchors.top: parent.top
        anchors.topMargin: topPadding
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
        anchors {
            fill: parent; leftMargin: fontSize; rightMargin: fontSize
            topMargin: fontSize + topPadding
            bottomMargin: fontSize + bottomPadding
        }
    }

    function updateScreenSize() {
        if (!video)
            return
        var w = width, h = height;
        if (B.App.window.fullscreen && Qt.platform.os == "windows") {
            w = B.App.window.width + 2;
            h = B.App.window.height + 2;
        }
        video.width = w * engine.zoom
        video.height = h * engine.zoom
        video.x = (w - video.width) * 0.5
        video.y = (h - video.height) * 0.5
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

    Component.onCompleted: {
        video.z = -1
    }
}
