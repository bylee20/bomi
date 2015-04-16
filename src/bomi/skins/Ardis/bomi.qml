// CREDIT: designed by Ivan(Kotus works)
//         https://plus.google.com/u/1/117118228830713086299/posts

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithFloating {
    id: skin;
    name: "net.xylosper.bomi.Ardis"
    readonly property QtObject engine: B.App.engine
    readonly property size minimumSize: Qt.size(560, 130)

    Component {
        id: sliderstyle
        SliderStyle {
            groove: Rectangle {
                width: control.width; height: 5; color: "#282629"
                Rectangle {
                    width: control.width * control.arate; height: parent.height
                    color: Qt.rgba(1, 1, 1, 0.5)
                }
                Rectangle {
                    width: control.width * control.rate; height: parent.height
                    color: "#0078ad"
                }
            }
            handle: Item { }
        }
    }

    controls: Item {
        width: 560; height: panel.height + 40;
        opacity: 0.85
        Rectangle {
            id: panel
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            color: "#efefef"; height: 90; radius: 3

            Item {
                id: mbuttons
                anchors.verticalCenter: panel.top; width: parent.width
                readonly property real gap: 74
                MediaButton {
                    id: prev; offset: -2
                    icon.source: "prev.png"; action: "play/prev"
                }
                MediaButton {
                    id: backward; offset: -1
                    icon.source: "backward.png"; action: "play/seek/backward1"
                }
                MediaButton {
                    id: play; size: 82
                    icon.source: checked ? "pause.png" : "play.png"
                    action: "play/play-pause"; checked: engine.playing
                }
                MediaButton {
                    id: forward; offset: 1
                    icon.source: "forward.png"; action: "play/seek/forward1"
                }
                MediaButton {
                    id: next; offset: 2
                    icon.source: "next.png"; action: "play/next"
                }
            }

            Item {
                anchors {
                    fill: parent
                    leftMargin: 15; rightMargin:  anchors.leftMargin
                    topMargin:  8;  bottomMargin: anchors.topMargin
                }
                SmallButton {
                    id: shuffle
                    anchors { top: parent.top; left: parent.left }
                    icon.source: "shuffle.png"; action: "tool/playlist/shuffle"
                    checked: B.App.playlist.shuffled
                }

                SmallButton {
                    id: repeat
                    anchors { top: parent.top; left: shuffle.right; leftMargin: 7 }
                    icon.source: "repeat.png"; action: "tool/playlist/repeat"
                    checked: B.App.playlist.repetitive
                }

                SmallButton {
                    id: speaker
                    anchors { left: parent.left; bottom: parent.bottom }
                    icon.source: "volume.png"; action: "audio/volume/mute"
                    checked: B.App.engine.muted
                }

                SmallButton {
                    id: playlist
                    anchors { top: parent.top; right: fs.left; rightMargin: 7 }
                    icon.source: "playlist.png"; action: "tool/playlist/toggle"
                    checked: B.App.playlist.visible
                }

                SmallButton {
                    id: fs
                    anchors { right: parent.right; top: parent.top }
                    icon.source: "fullscreen.png"; action: "window/full"
                    checked: B.App.window.fullscreen
                }


                SmallButton {
                    id: audio
                    anchors { bottom: parent.bottom; right: fs.left; rightMargin: 7 }
                    icon.source: "audio.png";
                    action: "audio/track/next"; action2: "audio/track"
                    text.content: formatTrackNumber(engine.audio)
                }

                SmallButton {
                    id: sub
                    anchors { right: parent.right; bottom: parent.bottom }
                    icon.source: "sub.png"
                    action: "subtitle/track/next"; action2: "subtitle/track"
                    text.content: formatTrackNumber(engine.subtitle)
                }

                B.VolumeSlider {
                    width: 65; height: 10; style: sliderstyle
                    anchors {
                        left: speaker.right; leftMargin: 15
                        verticalCenter: speaker.verticalCenter
                    }
                }

                B.TimeSlider {
                    id: seeker; bind: td
                    width: 300; height: 10; style: sliderstyle
                    z: 1
                    anchors {
                        verticalCenter: speaker.verticalCenter
                        horizontalCenter: parent.horizontalCenter
                        horizontalCenterOffset: 22
                    }

                    markerStyle: B.Button {
                        readonly property bool emph: hovered || pressed
                        width: 5; height: 5; z: emph ? 1e10 : -1
                        y: control.height - 2*(pressed ? 1 : hovered ? -1 : 0) - 2.5
                        icon.source: emph ? "marker-filled.png" : "marker.png"
                        tooltip: chapter.name; delay: 0
                    }
                }

                B.TimeDuration {
                    id: td
                    height: 13; spacing: 2
                    textStyle {
                        font.pixelSize: height
                        color: "black"
                    }
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                        verticalCenterOffset: 7
                    }
                }
            }
        }
    }
}
