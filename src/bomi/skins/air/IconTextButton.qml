import QtQuick 2.0
import bomi 1.0 as B

ImageButton {
    width: icon.width + text.contentWidth + spacing;
    textStyle {
        color: "white"
        style: Text.Outline
        styleColor: Qt.rgba(0, 0, 0, 0.5)
        monospace: false
        elide: Text.ElideNone
        font.pixelSize: 14
        verticalAlignment: Text.AlignVCenter
    }
    opacity: 0.8
    height: icon.height
    adjustIconSize: false
    spacing: 2
    icon { width: 32*0.7; height: 22*0.7 }
    layout: leftIcon
    Component.onCompleted: textStyle.elide = Text.ElideNone
}

