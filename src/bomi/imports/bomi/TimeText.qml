import QtQuick 2.0
import bomi 1.0 as B

B.Text {
    id: item
    property int time: 0
    property bool msec: false
    property bool hour: false

    QtObject { id: s; property int time: item.time/1000 }
    width: contentWidth; height: parent.height;
    content: B.Format.time(msec ? time : s.time * 1000, msec, hour)
}
