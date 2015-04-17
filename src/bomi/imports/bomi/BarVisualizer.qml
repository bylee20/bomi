import QtQuick 2.0
import bomi 1.0

Item {
    id: item
    readonly property Visualizer vis: root.engine.visualizer

    QtObject {
        id: d
        readonly property real widthHint: Math.min(10 * 100 + 2 * 99, item.width)
        readonly property int barWidth: (widthHint - (vis.count - 1) * gap) / vis.count
        property int gap: widthHint < 800 ? 1 : 2
    }

    Component.onCompleted: { vis.count = 100 }

    Item {
        id: frame
        height: parent.height - 10
        width: (d.barWidth + d.gap) * vis.count - d.gap
        anchors.bottom: parent.bottom
        x: (parent.width - width) * 0.5 | 0

        Repeater {
            model: vis.count
            Rectangle {
                id: rect; parent: frame
                width: d.barWidth; anchors.bottom: parent.bottom
                x: index * (d.barWidth + d.gap)
                gradient: Gradient {
                    id: grad
                    readonly property real min: -rect.y / rect.height
                    GradientStop { position: grad.min; color: "red" }
                    GradientStop { position: grad.min * 0.5 + 0.5; color: "yellow" }
                    GradientStop { position: 1.0; color: "green" }
                }

                readonly property real value: vis.data[index] * frame.height
                property bool run: false

                onValueChanged: {
                    if (height < value) {
                        run = false
                        an.to = value
                        an.duration = 100
                        an.easing.type = Easing.OutQuad
                        run = true
                    }
                }
                onHeightChanged: {
                    if (height >= value)
                    {
                        run = false
                        an.to = 0
                        an.duration = 1500
                        an.easing.type = Easing.Linear
                        run = true
                    }
                    if (height > top.anchors.bottomMargin)
                        top.anchors.bottomMargin = height
                }

                NumberAnimation {
                    id: an
                    target: rect
                    property: "height"
                    easing.type: Easing.Linear
                    running: rect.run && App.engine.playing
                }

                Rectangle {
                    id: top; parent: frame
                    width: d.barWidth; height: Math.max(1, Math.min(5, width * 0.5)) | 0
                    x: rect.x; anchors.bottom: parent.bottom

                    NumberAnimation {
                        id: anTop
                        target: top; property: "anchors.bottomMargin"; to: 0
                        easing.type: Easing.InQuad; duration: 3000
                        running: top.anchors.bottomMargin > rect.height
                    }

                }
            }
        }
    }
}
