import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    icon.smooth: true
    icon.scale: pressed ? 0.95 : hovered ? 1.1 : 1
    anchors.verticalCenter: parent.verticalCenter
}
