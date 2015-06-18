// Designed by varlesh (varlesh@gmail.com)
import QtQuick 2.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    name: "net.xylosper.Freya"
    minimumSize: Qt.size(450, 200)
    overlaps: true; blurBackground: true

    readonly property QtObject engine: B.App.engine
    Component {
        id: sliderstyle
        SliderStyle {
            groove: Item {
                implicitHeight: 5;
                implicitWidth: 100;
                Rectangle {
                    anchors.fill: parent
                    radius: 4; border { color: "#6e6e6e"; width: 1 }
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#b9b9b9"}
                        GradientStop {position: 1.0; color: "#c4c4c4"}
                    }
                }
                Rectangle {
                    radius: 4; border { color: "#6e6e6e"; width: 1 }
                    anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
                    width: parent.width*control.rate
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#818181"}
                        GradientStop {position: 1.0; color: "#8f8f8f"}
                    }
                }
            }
            handle: Item { }
        }
    }

    topControls: Item {
        id: controls
        width: parent.width; height: 56
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#dedede" }
                GradientStop { position: 0.5; color: "#d1d1d1" }
                GradientStop { position: 1.0; color: "#c0c0c0" }
            }

            Row {
                id: mediaButtons
                opacity: 0.8; spacing: 5
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left; leftMargin: 15
                }

                B.Button {
                    id: prev; width: 32; height: 32
                    action: "play/prev"; icon.prefix: "prev";
                }

                B.Button {
                    id: play; width: 32; height: 32
                    action: "play/play-pause"; icon.prefix: engine.playing ? "pause" : "play"
                }

                B.Button {
                    id: next; width: 32; height: 32
                    action: "play/next"; icon.prefix: "next"
                }
            }
            B.TimeSlider {
                id: seeker; style: sliderstyle
                anchors { fill: parent; leftMargin: 130; rightMargin: 130 }
            }
            Row {
                id: toolButtons; spacing: -1;
                anchors {
                    right: parent.right; rightMargin: 45;
                    verticalCenter: parent.verticalCenter
                }

                B.Button {
                    id: subtitles; width: 24; height: 25
                    action: "subtitle/track"; icon.prefix: "sub"
                }
                B.Button {
                    id: playlist; width: 25; height: 25
                    action: "tool/playlist/toggle"; icon.prefix: "pl"
                }
                B.Button {
                    id: fullscreen; width: 25; height: 25
                    action: "window/full"; icon.prefix: "fullscreen"
                }
            }
            Row {
                id: toolButtons2;
                anchors {
                    right: parent.right; rightMargin: 10;
                    verticalCenter: parent.verticalCenter
                }
                B.Button {
                    id: preferences; width: 32; height: 32
                    action: "tool/pref"; icon.prefix: "pref"
                }
            }
        }
    }
}
