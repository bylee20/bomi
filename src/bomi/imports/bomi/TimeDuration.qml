import QtQuick 2.0
import bomi 1.0 as B

Item {
    id: item
    property alias color: slash.color
    property alias font: slash.font
    property alias monospace: slash.monospace
    property real spacing: 0
    property bool msec: false
    property alias time: timeText.time
    property alias duration: endText.time
    implicitWidth: timeText.paintedWidth + slash.paintedWidth + endText.paintedWidth
    implicitHeight: Math.max(timeText.paintedHeight, slash.paintedHeight, endText.paintedHeight)

    QtObject {
        id: d
        readonly property QtObject engine: B.App.engine
    }

    B.TimeText {
        id: timeText; height: parent.height
        anchors {
            right: slash.left; rightMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        time: d.engine.time
        font: slash.font; color: slash.color
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
    B.Text {
        id: slash;
        height: parent.height; anchors.centerIn: parent
        text: "/"; verticalAlignment: Text.AlignVCenter
    }
    B.TimeText {
        id: endText; height: parent.height
        anchors {
            left: slash.right; leftMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        time: d.engine.end
        font: slash.font; color: slash.color
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
}
