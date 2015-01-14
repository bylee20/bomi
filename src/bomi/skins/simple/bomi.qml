import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as Cp

Cp.AppWithDock {
    id: app
    readonly property QtObject engine: Cp.App.engine
    Component {
        id: sliders
        SliderStyle {
            readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
            groove: Item {
                implicitHeight: 12;
                implicitWidth: 100;
                Rectangle {
                    anchors.fill: parent
                    color: "#fff"; border { color: "#999"; width: 1 }
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#555"}
                        GradientStop {position: 1.0; color: "#bbb"}
                    }
                }
                Rectangle {
                    border { color: "#999"; width: 1 }
                    anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
                    width: parent.width*ratio
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#fff"}
                        GradientStop {position: 1.0; color: "#ccc"}
                    }
                }
            }
            handle: Item {}
        }
    }

    controls: Rectangle {
        width: parent.width; height: 20
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#aaa" }
            GradientStop { position: 0.1; color: "#eee" }
            GradientStop { position: 1.0; color: "#aaa" }
        }
        RowLayout {
            anchors { fill: parent; margins: 4 } spacing: 3;
            Cp.Button {
                id: playPause; width: height; height: parent.height
                action: "play/pause"; icon: (player.state === Cp.Engine.Playing) ? "pause.png" : "play.png"
                paddings: pressed ? 2 : (hovered ? 0 : 1)
            }
            Cp.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }
            Row {
                width: childrenRect.width; height: parent.height
                Cp.TimeText { textColor: "black"; msecs: engine.time }
                Cp.TimeText { textColor: "black"; text: "/" }
                Cp.TimeText { textColor: "black"; msecs: engine.end }
            }
            Cp.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
        }
    }
}
