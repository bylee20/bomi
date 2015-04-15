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
            readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
            groove: Item {
                implicitHeight: 4;
                implicitWidth: 100;
                Rectangle {
                    anchors.fill: parent
                    color: "#fff"; border { color: "#999"; width: 1 }
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#acacac"}
                        GradientStop {position: 1.0; color: "#acacac"}
                    }
                }
                Rectangle {
                    border { color: "#999"; width: 1 }
                    anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
                    width: parent.width*ratio
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#6699cc"}
                        GradientStop {position: 1.0; color: "#6699cc"}
                    }
                }
            }
            handle: Image {
                width: 16; height: 16
                source: control.pressed ? "handle-pressed.png" : control.hovered ? "handle-hovered.png" : "handle.png"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    bottomControls: Rectangle {
        width: parent.width; height: 32
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#aaa" }
            GradientStop { position: 0.1; color: "#eee" }
            GradientStop { position: 1.0; color: "#aaa" }
        }
        RowLayout {
            anchors { fill: parent; margins: 4 } spacing: 3;

            B.Button {
                id: playPrev; width: 24; height: 24
                action: "play/prev"; icon.prefix: "previous"
            }

            B.Button {
                id: playPause; width: 24; height: 24
                action: "play/play-pause"; icon.prefix: engine.playing ? "pause" : "play"
            }
            B.Button {
                id: playStop; width: 24; height: 24
                action: "play/stop"; icon.prefix: "stop"
            }

            B.Button {
                id: playNext; width: 24; height: 24
                action: "play/next"; icon.prefix: "next"
            }

            B.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }

            B.TimeDuration {
                height: parent.height
                textStyle { color: "#1e1e1e"; font.pixelSize: 10; monospace: true }
            }

            Row {
                B.Button {
                    id: playlistIcon; width: 24; height: 24
                    action: "tool/playlist/toggle"; icon.prefix: "playlist"; action2: "tool/playlist"
                    tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
                }
                B.Button {
                    id: fullscreen; width: 24; height: 24
                    action: "window/full"; icon.prefix: "fullscreen"
                }
                B.Button {
                    id: openFolder; width: 24; height: 24
                    action: "open/folder"; icon.prefix: "open-folder"
                }
                B.Button {
                    id: mute; width: 24; height: 24
                    action: "audio/volume/mute"; icon.prefix: engine.muted ? "speaker-off" : "speaker-on"
                }
            }
            B.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
        }
    }
}
