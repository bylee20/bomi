import QtQuick 2.0
import bomi 1.0 as B

Item {
    id: item
    property alias textStyle: slash.textStyle
    property real spacing: 0
    property bool msec: false
    property bool remaining: false
    property alias time: timeText.time
    property alias duration: endText.time
    property bool interactive: true
    implicitWidth: timeText.contentWidth + slash.contentWidth + endText.contentWidth
    implicitHeight: Math.max(timeText.contentHeight, slash.contentHeight, endText.contentHeight)

    QtObject {
        id: d
        readonly property QtObject engine: B.App.engine
        function prefix() {
            var ret = "TimeDuration-"
            if (objectName.length > 0)
                ret += objectName + "-"
            return ret
        }
    }

    B.TimeText {
        id: timeText; height: parent.height
        anchors {
            right: slash.left; rightMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        time: d.engine.time
        textStyle: item.textStyle
        msec: item.msec

        MouseArea {
            anchors.fill: parent
            visible: interactive
            acceptedButtons: Qt.LeftButton
            onClicked: item.msec = !item.msec
        }
    }
    B.Text {
        id: slash;
        height: parent.height; anchors.centerIn: parent
        content: "/";
        textStyle {
            verticalAlignment: Text.AlignVCenter
        }
    }
    B.TimeText {
        id: endText; height: parent.height
        anchors {
            left: slash.right; leftMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        time: remaining ? (d.engine.time - d.engine.end) : d.engine.end
        textStyle: item.textStyle
        msec: item.msec

        MouseArea {
            anchors.fill: parent
            visible: interactive
            acceptedButtons: Qt.LeftButton
            onClicked: remaining = !remaining
        }
    }

    Component.onCompleted: {
        if (name && name.length > 0) {
            B.Settings.open(name)
            msec = B.Settings.getBool(d.prefix() + "msec", false)
            remaining = B.Settings.getBool(d.prefix() + "remaining", true)
            B.Settings.close()
        }
    }
    Component.onDestruction: {
        if (name && name.length > 0) {
            B.Settings.open(name)
            B.Settings.set(d.prefix() + "msec", msec)
            B.Settings.set(d.prefix() + "remaining", remaining )
            B.Settings.close()
        }
    }
}
