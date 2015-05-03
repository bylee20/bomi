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
    readonly property int gap: {
        if (app.width < 400)
            return 1
        if (app.width < 600)
            return 5
        if (app.width < 800)
            return 10
        return 20
    }
    property bool ms: false

    name: "net.xylosper.metro"
    minimumSize: Qt.size(500, 200)
    overlaps: true
    trackingMinY: height - Math.max(height * 0.35, bottomControls.height)

    Component {
        id: rightButtons
        Row {
            spacing: gap; height: 30
            MetroButton { prefix: compact ? "to-normal" : "to-compact"; onClicked: compact = !compact }
            MetroButton { prefix: "sub"; action: "subtitle/track/cycle"; action2: "subtitle/track" }
            MetroButton { prefix: "audio"; action: "audio/track/cycle"; action2: "audio/track" }
            MetroButton { prefix: B.App.window.fullscreen ? "exit-fs" : "enter-fs"; action: "window/toggle-fs" }
        }
    }

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
                    x: parent.width * control.rate + 5
                    width: parent.width * control.arate - x; height: parent.height; color: Qt.rgba(1, 1, 1, 0.4)
                    anchors.verticalCenter: parent.verticalCenter
                    onWidthChanged: width
                }
                Rectangle {
                    width: parent.width * control.rate - 5; height: parent.height
                    color: Qt.rgba(1, 1, 1, 1)
                }
            }
            handle: Rectangle {
                width: 12; height: width; radius: width * 0.5
                x: (control.rate - 0.5) * width; y: 0
                border { color: "white"; width: 2 } color: "transparent"
            }
        }
    }

    Component {
        id: normalComponent
        Rectangle {
            implicitHeight: 100; color: Qt.rgba(0, 0, 0, 0.6)
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
                    MetroButton { prefix: engine.playing ? "pause" : "play"; action: "play/play-pause" }
                    MetroButton { prefix: "stop"; action: "play/stop"; enabled: !engine.stopped }
                    MetroButton { prefix: "forward"; action: "play/seek/forward1" }
                    MetroButton { prefix: "next"; action: "play/next" }
                }

                Loader { anchors.right: parent.right; sourceComponent: rightButtons }

                B.TimeSlider {
                    id: timeSlider; style: sliders; width: parent.width; height: 20
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
    }

    Component {
        id: compactComponent
        Rectangle {
            implicitHeight: 50; color: Qt.rgba(0, 0, 0, 0.6)
            RowLayout {
                spacing: gap
                anchors { fill: parent; leftMargin: 20; rightMargin: 20 }
                MetroButton { prefix: "prev"; action: "play/prev" }

                MetroButton { prefix: engine.playing ? "pause" : "play"; action: "play/play-pause" }
                MetroButton { prefix: "next"; action: "play/next" }
                Item {
                    height: 30; Layout.fillWidth: true
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        id: volumeArea
                        width: volume.width; height: 30
                        anchors.verticalCenter: parent.verticalCenter
                        clip: true; hoverEnabled: true
                        MetroButton {
                            id: volume
                            prefix: engine.muted ? "mute" : "volume"
                            action: "audio/volume/mute"
                        }
                        B.VolumeSlider {
                            id: vsCompact
                            style: sliders; width: 70; height: 4
                            anchors {
                                left: volume.right; leftMargin: gap
                                verticalCenter: parent.verticalCenter
                            }
                        }
                        states: State {
                            name: "expand"; when: volumeArea.containsMouse
                            PropertyChanges {
                                target: volumeArea
                                width: volume.width + vsCompact.width + gap + 7
                            }
                        }
                        transitions: Transition {
                            reversible: true; to: "expand"
                            NumberAnimation {
                                target: volumeArea
                                property: "width"; duration: 200
                                easing.type: Easing.InOutQuad
                            }
                        }
                    }
                    B.TimeSlider {
                        id: tsCompact; style: sliders; height: 20;
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right; left: volumeArea.right; leftMargin: gap
                        }
                    }
                }
                B.TimeDuration { width: contentWidth; textStyle.color: "white" }
                Loader { sourceComponent: rightButtons }
            }
        }
    }

    bottomControls: Loader {
        id: controlLoader
        width: parent.width; height: controlLoader.implicitHeight
        sourceComponent: compact ? compactComponent : normalComponent
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
