import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    id: item
    background.radius: 3
    background.border.width: pressed ? 2 : (hovered ? 1 : 0)
    background.border.color: "#6ad"
    paddings: 2
}
