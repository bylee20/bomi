import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    property int offset: 0
    size: 62
    anchors.centerIn: parent
    anchors.horizontalCenterOffset: offset * 74
    scale: pressed ? 0.95 : hovered ? 1.05 : 1
    adjustIconSize: true
    icon.smooth: true
}
