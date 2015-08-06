// written by varlesh <varlesh@gmail.com>

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app

    readonly property QtObject engine: B.App.engine
    Component {
        id: sliders
        SliderStyle {
            groove: Item {
                implicitHeight: 2;
                implicitWidth: 100;
                Rectangle {
                    anchors.fill: parent
                    color: "#3daee9"
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#7d7d7d"}
                        GradientStop {position: 1.0; color: "#7d7d7d"}
                    }
                }
                Rectangle {
                    border { color: "#3daee9"; width: 1 }
                    anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
                    width: parent.width*control.rate
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#7d7d7d"}
                        GradientStop {position: 1.0; color: "#7d7d7d"}
                    }
                }
            }
            handle: Image {
                width: 22; height: 22
                source: control.pressed ? "handle-pressed.png" : control.hovered ? "handle-hovered.png" : "handle.png"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    bottomControls: Rectangle {
        width: parent.width; height: 32
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#eff0f1" }
            GradientStop { position: 0.1; color: "#eff0f1" }
            GradientStop { position: 1.0; color: "#eff0f1" }
        }
        RowLayout {
            anchors { fill: parent; margins: 4 } spacing: 3;

            B.Button {
                id: playPrev; width: 22; height: 22
                action: "play/prev"; icon.prefix: "previous"
            }

            B.Button {
                id: playPause; width: 22; height: 22
                action: "play/play-pause"; icon.prefix: engine.playing ? "pause" : "play"
            }
            B.Button {
                id: playStop; width: 22; height: 22
                action: "play/stop"; icon.prefix: "stop"
            }

            B.Button {
                id: playNext; width: 22; height: 22
                action: "play/next"; icon.prefix: "next"
            }

            B.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }

            B.TimeDuration {
                height: parent.height;
                textStyle {
                    color: "#1e1e1e"
                    font.pixelSize: 10
                }
            }

            Row {
                B.Button {
                    id: playlistIcon; width: 22; height: 22; icon.prefix: "playlist"
                    action: "tool/playlist/toggle"; action2: "tool/playlist"
                    tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
                }
                B.Button {
                    id: fullscreen; width: 22; height: 22
                    action: "window/full"; icon.prefix: "fullscreen"
                }
                B.Button {
                    id: mute; width: 22; height: 22
                    action: "audio/volume/mute"; icon.prefix: engine.muted ? "speaker-off" : "speaker-on"
                }
            }
            B.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
        }
    }
}
