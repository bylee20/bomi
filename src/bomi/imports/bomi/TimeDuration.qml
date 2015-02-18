import QtQuick 2.0
import bomi 1.0 as B

Item {
    id: item
    property alias textStyle: slash.textStyle
    property real spacing: 0
    property bool msec: false
    property alias time: timeText.time
    property alias duration: endText.time
    implicitWidth: timeText.contentWidth + slash.contentWidth + endText.contentWidth
    implicitHeight: Math.max(timeText.contentHeight, slash.contentHeight, endText.contentHeight)

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
        textStyle: item.textStyle
        msec: item.msec
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
        time: d.engine.end
        textStyle: item.textStyle
        msec: item.msec
    }
}
