import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    readonly property QtObject engine: B.App.engine
    Component {
        id: sliders
        SliderStyle {
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
                    width: parent.width*control.rate
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#fff"}
                        GradientStop {position: 1.0; color: "#ccc"}
                    }
                }
            }
            handle: Item {}
        }
    }

    bottomControls: Rectangle {
        width: parent.width; height: 20
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#aaa" }
            GradientStop { position: 0.1; color: "#eee" }
            GradientStop { position: 1.0; color: "#aaa" }
        }
        RowLayout {
            anchors { fill: parent; margins: 4 } spacing: 3;
            B.Button {
                id: playPause; size: parent.height
                action: "play/play-pause";
                adjustIconSize: true
                icon.source: engine.playing ? "pause.png" : "play.png"
                paddings: pressed ? 2 : (hovered ? 0 : 1)
            }
            B.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }
            B.TimeDuration { height: parent.height }
            B.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
        }
    }
}
