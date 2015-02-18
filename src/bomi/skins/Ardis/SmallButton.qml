import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    width: 34; height: 24
    adjustIconSize: false
    icon.smooth: false
    background { color: Qt.rgba(0, 0, 0, checked ? 0.2 : 0); radius: 2 }

    icon {
        width: 32; height: 22
        anchors.verticalCenterOffset: pressed ? 1 : hovered ? -1 : 0
        anchors.horizontalCenterOffset: icon.anchors.verticalCenterOffset
    }

    textStyle { color: "white"; font.pixelSize: 9 }
}
