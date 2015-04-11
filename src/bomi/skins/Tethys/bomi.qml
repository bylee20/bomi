// CREDIT: designed by Ivan(Kotus works)
//         https://plus.google.com/u/1/117118228830713086299/posts

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    name: "net.xylosper.Tethys"
    overlaps: true
    excludeMouseArea: Qt.rect(0, 0, width, height * 0.2)

    readonly property int s_thickness: 4
    readonly property int s_shadow: 1

    Component {
        id: sliders
        SliderStyle {
            groove: Item {
                Rectangle {
                    color: Qt.rgba(0, 0, 0, 0.4)
                    width: parent.width; height: s_thickness; y: s_shadow * 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: s_shadow
                }
                Rectangle {
                    width: parent.width; height: s_thickness; y: s_shadow
                    radius: height * 0.5; color: "#929292"
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        width: parent.width * control.rate; height: parent.height
                        radius: parent.radius; color: "#d73d48"
                    }
                }
            }
            handle: Image {
                width: control.height; height: width
                source: control.hpressed ? "handle-pressed.png"
                      : control.hhovered ? "handle-hovered.png" : "handle.png"
                x: (control.rate - 0.5) * width
            }
        }
    }

    Component {
        id: large
        Item {
            anchors {
                fill: parent
                leftMargin: 24; rightMargin: anchors.leftMargin
                topMargin: 10
            }

            B.TimeSlider {
                id: timeslider; style: sliders;
                width: parent.width; height: 24; bind: time
            }

            Row {
                id: mediaButtons; height: 44
                anchors { bottom: parent.bottom; bottomMargin: 6; left: parent.left }
                B.Button {
                    width: 32; height: 24
                    icon.prefix: "backward"; action: "play/seek/backward1"
                    anchors.verticalCenter: parent.verticalCenter
                }
                B.Button {
                    size: 44; action: "play/pause"
                    icon.prefix: engine.playing ? "pause" : "play"
                    anchors.verticalCenter: parent.verticalCenter
                }
                B.Button {
                    width: 32; height: 24
                    icon.prefix: "forward"; action: "play/seek/forward1"
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    id: volumeArea
                    width: volume.width + 40; height: 40
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: true
                    Item {
                        width: 24; height: 24
                        anchors.verticalCenter: parent.verticalCenter
                        Image { source: engine.muted ? "volume-muted-shadow.png" : "volume-shadow.png" }
                        Image { source: "volume-blur.png"; visible: volumeButton.hovered }
                        B.Button {
                            id: volumeButton
                            size: 24; action: "audio/volume/mute"
                            icon.source: engine.muted ? "volume-mute.png"
                                       : engine.volume < 0.05 ? "volume-0.png"
                                       : engine.volume < 0.4 ? "volume-low.png"
                                       : engine.volume < 0.8 ? "volume-high.png" : "volume-100.png"
                        }
                        Image {
                            visible: volumeButton.pressed
                            source: engine.muted ? "volume-muted-pressed.png" : "volume-pressed.png"
                        }
                    }

                    Item {
                        id: volumeBox; width: 26; height: parent.height; clip:true
                        B.VolumeSlider {
                            id: volume; style: sliders
                            x: 26; width: Math.min(app.width * 0.09, 80); height: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    states: State {
                        name: "show"; when: volumeArea.containsMouse
                        PropertyChanges { target: volumeBox; width: volume.width + 40 }
                    }

                    transitions: Transition {
                        reversible: true; to: "show"
                        NumberAnimation { target: volumeBox; property: "width"; duration: 200 }
                    }
                }
            }

            B.TimeDuration {
                id: time; spacing: 4
                anchors.verticalCenter: mediaButtons.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                textStyle { color: "white"; style: Text.Raised; styleColor: "black" }
            }

            Row {
                height: 44
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 6
                    right: parent.right
                }
                spacing: 5
                B.Button {
                    size: 24; icon.prefix: "playlist"
                    anchors.verticalCenter: parent.verticalCenter
                    action: "tool/playlist/toggle"; action2: "tool/playlist"
                }
                B.Button {
                    size: 24; icon.prefix: "subscale"
                    anchors.verticalCenter: parent.verticalCenter
                    action: "subtitle/scale/increase"; action2: "subtitle/scale/decrease"
                }
                B.Button {
                    size: 24; icon.prefix: "sub"
                    anchors.verticalCenter: parent.verticalCenter
                    action: "subtitle/track/next"; action2: "subtitle/track"
                }
                B.Button {
                    size: 24; icon.prefix: "audio"
                    anchors.verticalCenter: parent.verticalCenter
                    action: "audio/track/next"; action2: "audio/track"
                }
                B.Button {
                    size: 24; icon.prefix: "fs"; action: "window/full"
                    checked: B.App.window.fullscreen
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

        }
//        RowLayout {
//            anchors { fill: parent; margins: 4 } spacing: 3;

////            B.Button {
////                id: playPrev; width: 24; height: 24
////                action: "play/prev"; icon.prefix: "previous"
////            }

////            B.Button {
////                id: playPause; width: 24; height: 24
////                action: "play/pause"; icon.prefix: engine.playing ? "pause" : "play"
////            }
////            B.Button {
////                id: playStop; width: 24; height: 24
////                action: "play/stop"; icon.prefix: "stop"
////            }

////            B.Button {
////                id: playNext; width: 24; height: 24
////                action: "play/next"; icon.prefix: "next"
////            }


////            B.TimeDuration {
////                height: parent.height;
////                textStyle {
////                    color: "#1e1e1e"
////                    monospace: true
////                    font.pixelSize: 10
////                }
////            }

////            Row {
////                B.Button {
////                    id: playlistIcon; width: 24; height: 24; icon.prefix: "playlist"
////                    action: "tool/playlist/toggle"; action2: "tool/playlist"
////                    tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
////                }
////                B.Button {
////                    id: fullscreen; width: 24; height: 24
////                    action: "window/full"; icon.prefix: "fullscreen"
////                }
////                B.Button {
////                    id: mute; width: 24; height: 24
////                    action: "audio/volume/mute"; icon.prefix: engine.muted ? "speaker-off" : "speaker-on"
////                }
////            }
////            B.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
//        }

    }

    bottomControls: Rectangle {
        width: parent.width; height: 80
        gradient: Gradient {
            GradientStop {
                position: 0.0; color: Qt.rgba(0, 0, 0, 0)
            }
            GradientStop {
                position: 0.9; color: Qt.rgba(0, 0, 0, 0.82)
            }
        }

        Loader {
            sourceComponent: large
            anchors.fill: parent
        }
    }
}
