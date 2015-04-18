import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    property string prefix
    property url normalIcon: prefix + ".png"
    property url hoveredIcon: prefix + "-filled.png"
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
    icon.source: normalIcon

    Image {
        id: filled
        anchors.fill: parent
        source: hoveredIcon
        opacity: 0.0
    }
}

