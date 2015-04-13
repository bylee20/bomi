import QtQuick 2.0
import bomi 1.0 as B

TextButton {
    width: text.contentWidth
    height: text.contentHeight
    adjustIconSize: false
    icon { width: 0; height: 0 }
    layout: centerIcon

    property int time: 0
    property bool msec: false
    text.content: B.Format.time(time, msec, true)
    text.textStyle.monospace: true
    onClicked: checked = !checked
}
