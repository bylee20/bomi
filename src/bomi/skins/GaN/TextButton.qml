import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    width: icon.width + text.contentWidth + spacing; height: 12;
    adjustIconSize: false
    spacing: 2; icon { width: 20; height: 12 }
    textStyle {
        color: pressed ? "#0ef" : (hovered ? "#0cf" : "white")
        font.pixelSize: 12
    }
    text.width: text.contentWidth
    layout: leftIcon
}
