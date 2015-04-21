import QtQuick 2.0
import bomi 1.0 as B

QtObject {
    property font font
    property color color: Qt.rgba(0, 0, 0, 1)
    property color styleColor: Qt.rgba(0, 0, 0, 1)
    property int style: Text.Normal
    property int format: Text.PlainText
    property int elide: Text.ElideNone
    property int verticalAlignment: Text.AlignTop
    property int horizontalAlignment: Text.AlignLeft
    property bool monospace: false
    property int wrapMode: Text.NoWrap
    property QtObject copy: null
    onMonospaceChanged: {
        font.family = monospace ? B.App.theme.monospace
                                : B.App.theme.font.family
    }
    onCopyChanged: {
        font = copy.font
        color = copy.color
        styleColor = copy.styleColor
        style = copy.style
        elide = copy.elide
        verticalAlignment = copy.verticalAlignment
        horizontalAlignment = copy.horizontalAlignment
        monospace = copy.monospace
    }
}

