import QtQuick 2.0
import bomi 1.0 as B

B.Text {
    id: item
    property int time: 0
    property bool msec: false

    QtObject { id: s; property int time: (item.time/1000) | 0 }
    width: text.contentWidth; height: parent.height; color: "white"
    font.pixelSize: 10; monospace: true
    content: formatTime(msec ? time : s.time * 1000, msec)
}
