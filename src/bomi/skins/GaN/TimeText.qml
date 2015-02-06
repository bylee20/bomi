import QtQuick 2.0
import bomi 1.0 as B

TextButton {
    width: Util.textWidth(msec ? "00:00:00.000" : "00:00:00", text.font.pixelSize, text.font.family);
    height: text.contentHeight
    adjustIconSize: false
    icon { width: 0; height: 0 }
    layout: centerIcon

    property int time: 0
    property bool msec: false
    text.content: text.formatTime(time, msec)
    onClicked: checked = !checked
}
