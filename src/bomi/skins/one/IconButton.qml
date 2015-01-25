import QtQuick 2.0
import bomi 1.0 as B

B.Circle {
    diameter: 28
    rotation: gradientAngle
    property alias icon: button.icon
    property real gradientAngle: 60
    property alias action: button.action
    property alias action2: button.action2
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#444" }
        GradientStop { position: 1.0; color:"#111" }
    }

    B.Button {
        id: button
        width: 22; height: 22; opacity: 0.8
        scale: pressed ? 0.9 : hovered ? 1.1 : 1
        anchors.centerIn: parent
        rotation: -gradientAngle
    }
}
