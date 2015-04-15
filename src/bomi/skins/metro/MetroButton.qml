import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    size: 30
    property string prefix
    background {
        radius: size * 0.5; border { color: Qt.rgba(1, 1, 1, 1); width: 2 }
        color: hovered && !pressed ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0)
    }
    states: [
        State {
            name: "normal"; when: !hovered && !pressed
            PropertyChanges { target: filled; opacity: 0.0 }
            PropertyChanges { target: icon; opacity: 1.0 }
        },
        State {
            name: "hovered"; when: hovered && !pressed
            PropertyChanges { target: filled; opacity: 1.0 }
            PropertyChanges { target: icon; opacity: 0.0 }
        },
        State {
            name: "pressed"; when: pressed
            PropertyChanges { target: filled; opacity: 0.7 }
            PropertyChanges { target: icon; opacity: 0.0 }
        }
    ]
    transitions: [
        Transition {
            from: "normal"; to: "hovered"; reversible: true
            ParallelAnimation {
                NumberAnimation { target: filled; property: "opacity" }
                NumberAnimation { target: icon; property: "opacity" }
            }
        }
    ]
    icon.source: prefix + ".png"

    Image {
        id: filled
        anchors.fill: parent
        source: prefix + "-filled.png"
        opacity: 0.0
    }
}

