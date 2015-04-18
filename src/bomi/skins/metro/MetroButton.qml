import QtQuick 2.0
import bomi 1.0 as B

B.StateButton {
    size: 30
    normalIcon: prefix + ".png"
    hoveredIcon: prefix + "-filled.png"
    background {
        radius: size * 0.5; border { color: Qt.rgba(1, 1, 1, 1); width: 2 }
        color: hovered && !pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0)
    }
}
