import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    size: 30
    property string prefix
    background {
        radius: size * 0.5; border { color: Qt.rgba(1, 1, 1, 1); width: 2 }
        color: hovered && !pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0)
    }

    icon.source: prefix + (pressed ? "-filled.png" : ".png")
}

