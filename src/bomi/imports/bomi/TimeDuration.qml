import QtQuick 2.0
import bomi 1.0 as B

Item {
    id: item
    property alias color: slash.color
    property alias font: slash.font
    property alias monospace: slash.monospace
    property real spacing: 0
    property bool msec: false
    property int time: s.engine.time
    property int duration: s.engine.end
    implicitWidth: timeText.paintedWidth + slash.paintedWidth + endText.paintedWidth
    implicitHeight: Math.max(timeText.paintedHeight, slash.paintedHeight, endText.paintedHeight)
    QtObject {
        id: s
        readonly property QtObject engine: B.App.engine
        property int time: item.time/1000
        property int duration: item.duration/1000
    }

    B.Text {
        id: timeText; height: parent.height
        anchors {
            right: slash.left; rightMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        font: slash.font; color: slash.color
        text: formatTime(msec ? item.time : s.time * 1000, msec)
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
    B.Text {
        id: slash;
        height: parent.height; anchors.centerIn: parent
        text: "/"; verticalAlignment: Text.AlignVCenter
    }
    B.Text {
        id: endText; height: parent.height
        anchors {
            left: slash.right; leftMargin: spacing
            verticalCenter: slash.verticalCenter
        }
        text: formatTime(msec ? item.duration : s.duration * 1000, msec)
        font: slash.font; color: slash.color
        monospace: slash.monospace
        verticalAlignment: slash.verticalAlignment
    }
}
