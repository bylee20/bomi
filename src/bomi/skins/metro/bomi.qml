// CREDIT: designed by Ivan(Kotus works)
//         https://plus.google.com/u/1/117118228830713086299/posts

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app

    readonly property int s_thickness: 4
    readonly property int s_shadow: 1
    property bool compact: false
    readonly property int gap: B.Alg.clamp(width * 0.01, 1, 20)
    property bool ms: false

    name: "net.xylosper.metro"
    minimumSize: Qt.size(500, 300)
    overlaps: true
    trackingMinY: height - Math.max(height * 0.35, bottomControls.height)

    Component {
        id: sliders
        SliderStyle {
            groove: Item {
                width: parent.width; height: 4
                Rectangle {
                    x: parent.width * control.rate + 5
                    width: parent.width - x; height: parent.height; color: Qt.rgba(1, 1, 1, 0.3)
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    width: parent.width * control.rate - 5; height: parent.height
                    color: Qt.rgba(1, 1, 1, 1)
                }
            }
            handle: Rectangle {
                width: 12; height: width; radius: width * 0.5
                border { color: "white"; width: 2 }
                color: "transparent"
//                width: 6; height: width;
                x: (control.rate - 0.5) * width; y: 0
//                radius: 1; border { width: 1; color: "gray" } color: "white"
            }
        }
    }


    bottomControls: Rectangle {
        width: parent.width; height: 100
        color: Qt.rgba(0, 0, 0, 0.6)
        Item {
            anchors { fill: parent; leftMargin: 20; rightMargin: 20; topMargin: 20 }

            Row {
                spacing: gap; height: 30
                MetroButton {
                    prefix: engine.muted ? "mute" : "volume"
                    action: "audio/volume/mute"
                }
                B.VolumeSlider {
                    style: sliders;
                    width: B.Alg.clamp(app.width * 0.1, 20, 70); height: 4
                    anchors.verticalCenter: parent.verticalCenter
                }
                MetroButton { prefix: "playlist"; action: "tool/playlist/toggle" }
            }
            Row {
                spacing: gap; height: 30
                anchors.horizontalCenter: parent.horizontalCenter
                MetroButton { prefix: "prev"; action: "play/prev" }
                MetroButton { prefix: "backward"; action: "play/seek/backward1" }
                MetroButton { prefix: "pause"; action: engine.stopped ? "" : "play/pause" }
                MetroButton {
                    prefix: engine.running ? "stop" : "play"
                    action: engine.stopped ? "play/pause" : "play/stop"
                }
                MetroButton { prefix: "forward"; action: "play/seek/forward1" }
                MetroButton { prefix: "next"; action: "play/next" }
            }
            Row {
                spacing: gap; height: 30
                anchors.right: parent.right
                MetroButton { prefix: "sub"; action: "subtitle/track/cycle"; action2: "subtitle/track" }
                MetroButton { prefix: "audio"; action: "audio/track/cycle"; action2: "audio/track" }
                MetroButton {
                    prefix: B.App.window.fullscreen ? "exit-fs" : "enter-fs"
                    action: "window/toggle-fs"
                }
            }

            B.TimeSlider {
                id: timeSlider; style: sliders
                width: parent.width; height: 10
                anchors { bottom: parent.bottom; bottomMargin: 24 }
            }

            B.TextStyle { id: ts; color: "white" }
            B.TimeText {
                time: engine.time; textStyle: ts; msec: ms
                anchors { left: parent.left; top: timeSlider.bottom }
                MouseArea { anchors.fill: parent; onClicked: ms = !ms }
            }
            B.TimeText {
                id: end
                property bool remain: false;
                time: remain ? (engine.time - engine.end) : engine.end
                textStyle: ts; msec: ms
                anchors { top: timeSlider.bottom; right: parent.right }
                MouseArea { anchors.fill: parent; onClicked: end.remain = !end.remain }
            }
        }
    }

    Component.onCompleted: {
        B.Settings.open(app.name)
        compact = B.Settings.getBool("compact", false)
        B.Settings.close(app.name)
    }
    Component.onDestruction: {
        B.Settings.open(app.name)
        B.Settings.set("compact", compact)
        B.Settings.close(app.name)
    }
}
