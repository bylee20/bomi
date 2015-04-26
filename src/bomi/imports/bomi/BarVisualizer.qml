import QtQuick 2.2
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
        width: (d.barWidth + d.gap) * vis.count - d.gap
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.07
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.1
        x: (parent.width - width) * 0.5 | 0

        Repeater {
            model: vis.count
            Rectangle {
                id: rect; parent: frame
                width: d.barWidth; anchors.bottom: parent ? parent.bottom : undefined
                x: index * (d.barWidth + d.gap)
                gradient: Gradient {
                    id: grad
                    readonly property real min: -rect.y / rect.height
                    GradientStop { position: grad.min; color: "red" }
                    GradientStop { position: grad.min * 0.5 + 0.5; color: "yellow" }
                    GradientStop { position: 1.0; color: "green" }
                }

                readonly property real value: vis.data[index] * frame.height

                onValueChanged: {
                    if (height < value) {
                        sani.stop()
                        sani.value = value
                        sani.start()
                    }
                }
                onHeightChanged: {
                    if (height > top.anchors.bottomMargin)
                        top.anchors.bottomMargin = height
                }

                SequentialAnimation {
                    id: sani
                    property real value: 0
                    running: App.engine.playing
                    NumberAnimation {
                        target: rect
                        property: "height"
                        easing.type: Easing.OutQuad
                        duration: 100
                        to: sani.value
                    }
                    NumberAnimation {
                        target: rect
                        property: "height"
                        easing.type: Easing.Linear
                        duration: 1000
                        to: 0
                    }
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

    ShaderEffectSource {
        id: source
        sourceItem: frame
        width: frame.width; height: frame.height
        visible: false
    }

    ShaderEffect {
        width: frame.width
        anchors.top: frame.bottom
        height: frame.height * 0.1
        x: frame.x
        property ShaderEffectSource src: source
        property real tilt: width * 0.15
        vertexShader: "
                   uniform highp mat4 qt_Matrix;
                   uniform lowp float tilt;
                   attribute highp vec4 qt_Vertex;
                   attribute highp vec2 qt_MultiTexCoord0;
                   varying highp vec2 coord;
                   void main() {
                       coord = qt_MultiTexCoord0;
                       vec4 vtx = qt_Vertex;
                       vtx.x -= tilt * coord.y;
                       coord.y = 1.0 - coord.y;
                       gl_Position = qt_Matrix * vtx;
                   }"
        fragmentShader: "
                   varying highp vec2 coord;
                   uniform sampler2D src;
                   uniform lowp float qt_Opacity;
                   void main() {
                       lowp vec4 tex = texture2D(src, coord);
                       gl_FragColor = tex * qt_Opacity * coord.y * coord.y * 0.6;
                   }"
    }
}
