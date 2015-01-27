import QtQuick 2.0
import bomi 1.0 as B

Item {
    property alias color: slash.color
    property alias font: slash.font
    property alias monospace: slash.monospace
    property real spacing: 0
    property bool msec: false
    readonly property QtObject engine: B.App.engine
    implicitWidth: time.paintedWidth + slash.paintedWidth + end.paintedWidth
    implicitHeight: Math.max(time.paintedHeight, slash.paintedHeight, end.paintedHeight)
    B.Text {
        id: time; height: parent.height
        anchors {
            right: slash.left; rightMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        font: slash.font; color: slash.color
        text: formatTime(msec ? engine.time : engine.time_s*1000)
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
    B.Text {
        id: slash;
        height: parent.height; anchors.centerIn: parent
        text: "/"; verticalAlignment: Text.AlignVCenter
    }
    B.Text {
        id: end; height: parent.height
        anchors {
            left: slash.right; leftMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        text: formatTime(msec ? engine.end : engine.end_s*1000)
        font: slash.font; color: slash.color
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
}
